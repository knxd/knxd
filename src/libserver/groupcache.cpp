/*
    EIBD eib bus access and management daemon
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "groupcache.h"
#include "tpdu.h"
#include "apdu.h"

GroupCache::GroupCache (const LinkConnectPtr& c, IniSectionPtr& s)
	: Driver(c,s)
{
  t->setAuxName("G");
  TRACEPRINTF (t, 4, "GroupCacheInit");
  enable = 0;
  remtrigger.set<GroupCache, &GroupCache::remtrigger_cb>(this);
  addr = c->router.addr;
  c->is_local = true;
}

GroupCache::~GroupCache ()
{
  remtrigger.stop();
  R_ITER(i,reader)
    {
      (*i)->stop();
      delete *i;
    }
  TRACEPRINTF (t, 4, "GroupCacheDestroy");
  Clear ();
}

bool
GroupCache::setup()
{
  if (!Driver::setup())
    return false;
  remtrigger.start();
  this->maxsize = cfg->value("max-size", 0xFFFF);
  return true;
}

void
GroupCache::start()
{
  enable = true;
  Driver::start();
}

void
GroupCache::stop()
{
  enable = false;
  Driver::stop();
}

void
GroupCache::send_L_Data (LDataPtr l)
{
  if (enable)
    {
      TPDUPtr t = TPDU::fromPacket (l->data, this->t);
      if (t->getType () == T_DATA_XXX_REQ)
	{
	  T_DATA_XXX_REQ_PDU *t1 = (T_DATA_XXX_REQ_PDU *) &*t;
	  if (t1->data.size() >= 2 && !(t1->data[0] & 0x3) &&
	      ((t1->data[1] & 0xC0) == 0x40 || (t1->data[1] & 0xC0) == 0x80)) // response or write
	    {
              CacheMap::iterator ci = cache.find (l->dest);
              CacheMap::value_type *c;
              if (ci == cache.end())
                {
                  while (cache_seq.size() >= maxsize) 
                    {
                      SeqMap::iterator si = cache_seq.begin();
                      cache.erase(si->second);
                      cache_seq.erase(si);
                    }
                  c = &(*cache.emplace(l->dest, GroupCacheEntry(l->dest)).first);
                }
              else
                {
                  c = &(*ci);
                  cache_seq.erase(c->second.seq);
                }
              c->second.src = l->source;
              c->second.data = t1->data;
              c->second.recvtime = time (0);
              c->second.seq = ++seq;
              cache_seq.emplace(c->second.seq,c->first);
              updated(c->second);
	    }
	}
    }
  send_Next();
}

bool
GroupCache::Start ()
{
  TRACEPRINTF (t, 4, "GroupCacheEnable");
  enable = 1;
  return true;
}

void
GroupCache::Clear ()
{
  TRACEPRINTF (t, 4, "GroupCacheClear");
  cache.clear();
}

void
GroupCache::Stop ()
{
  Clear ();
  TRACEPRINTF (t, 4, "GroupCacheStop");
  enable = 0;
}

void
GroupCache::remove (eibaddr_t ga)
{
  CacheMap::iterator f = cache.find(ga);
  if (f != cache.end()) 
    {
      cache_seq.erase(f->second.seq);
      cache.erase(f);
    }
}

GroupCacheReader::GroupCacheReader(GroupCache *gc)
{
  this->gc = gc;
  gc->add(this);
}

GroupCacheReader::~GroupCacheReader()
{
}

void
GroupCacheReader::stop()
{
  if (stopped)
    return;
  stopped = true;
  gc->remove(this);
}

void
GroupCache::add (GroupCacheReader * entry)
{
  reader.push_back(entry);
}

void
GroupCache::updated(GroupCacheEntry &c)
{
  // do this in reverse so that the update handler can safely remove itself
  R_ITER(i,reader)
    (*i)->updated(c);
}

void
GroupCache::remove (GroupCacheReader * entry UNUSED)
{
  remtrigger.send();
}

void
GroupCache::remtrigger_cb(ev::async &w UNUSED, int revents UNUSED)
{
  // erase() doesn't do reverse iterators
  //R_ITER(i,reader)
  unsigned int i = reader.size();
  while(i--)
    {
      GroupCacheReader *r = reader[i];
      if (!r->stopped)
        continue;
      delete r;
      reader.erase(reader.begin()+i);
    }
}

class GCReader : protected GroupCacheReader
{
  GCReadCallback cb;
  ClientConnPtr cc;
  eibaddr_t addr;
  uint16_t age;
  ev::timer timeout;
public:
  GCReader(GroupCache *gc, eibaddr_t addr, int Timeout, uint16_t age,
           GCReadCallback cb, ClientConnPtr cc) : GroupCacheReader(gc)
  {
    this->cb = cb;
    this->cc = cc;
    this->addr = addr;
    this->age = age;
    timeout.set<GCReader,&GCReader::timeout_cb>(this);
    timeout.start(Timeout,0);
  }
  virtual ~GCReader()
    {
      if (stopped)
        return;
      timeout.stop();
      GroupCacheReader::stop();
    }
private:
  void updated(GroupCacheEntry &c)
  {
    if (stopped)
      return;
    if (c.dst != addr)
      return;

    TRACEPRINTF (gc->t, 4, "GroupCache found: %s",
                  FormatEIBAddr (c.src).c_str());
    cb(c,false,cc);
    stop();
  }

  void timeout_cb(ev::timer &w UNUSED, int revents UNUSED)
  {
    if (stopped)
      return;

    GroupCacheEntry f(addr);
    TRACEPRINTF (gc->t, 4, "GroupCache reread timeout");
    cb(f,false,cc);
    stop();
    return;
  }
};

void
GroupCache::Read (eibaddr_t addr, unsigned Timeout, uint16_t age,
  GCReadCallback cb, ClientConnPtr cc)
{
  TRACEPRINTF (t, 4, "GroupCacheRead %s %d %d",
	       FormatGroupAddr (addr).c_str(), Timeout, age);

  if (!enable)
    {
      GroupCacheEntry f(0);
      TRACEPRINTF (t, 4, "GroupCache not enabled");
      cb(f, Timeout == 0, cc);
      return;
    }

  CacheMap::iterator c = cache.find (addr);
  if (c != cache.end() && age && c->second.recvtime + age < time (0))
    c = cache.end();
  if (c != cache.end())
    {
      TRACEPRINTF (t, 4, "GroupCache found: %s",
		   FormatEIBAddr (c->second.src).c_str());
      cb(c->second, Timeout == 0, cc);
      return;
    }

  if (!Timeout)
    {
      GroupCacheEntry f(addr);
      TRACEPRINTF (t, 4, "GroupCache no entry");
      cb(f, true, cc);
      return;
    }

  // No data fond. Send a Read request.
  A_GroupValue_Read_PDU apdu;
  T_DATA_XXX_REQ_PDU tpdu;
  LDataPtr l;

  new GCReader(this,addr,Timeout,age, cb,cc);

  tpdu.data = apdu.ToPacket ();
  l = LDataPtr(new L_Data_PDU ());
  l->data = tpdu.ToPacket ();
  l->source = 0;
  l->dest = addr;
  l->AddrType = GroupAddress;
  recv_L_Data (std::move(l));
}

class GCTracker : protected GroupCacheReader
{
  GCLastCallback cb;
  ClientConnPtr cc;
  ev::timer timeout;
  Array < eibaddr_t > a;
  uint32_t start;
public:
  bool stopped = false;

  GCTracker(GroupCache *gc, uint32_t start, int Timeout,
           GCLastCallback cb, ClientConnPtr cc) : GroupCacheReader(gc)
  {
    this->cb = cb;
    this->cc = cc;
    this->start = start;
    timeout.set<GCTracker,&GCTracker::timeout_cb>(this);
    timeout.start(Timeout,0);
  }
  virtual ~GCTracker() {
      a.clear();
  }
  void stop()
  {
      if (stopped)
        return;
      timeout.stop();
      GroupCacheReader::stop();
  }
private:
  void updated(GroupCacheEntry &c UNUSED)
  {
    if (stopped)
      return;

    handler();
  }

  void timeout_cb(ev::timer &w UNUSED, int revents UNUSED)
  {
    if (stopped)
      return;
    if (handler())
      return;
    cb(a,gc->seq,cc);
    stop();
  }

  bool handler()
  {
    TRACEPRINTF (gc->t, 8, "LastUpdates start: x%x pos: x%x", start, gc->seq);
    SeqMap::const_iterator sa = gc->cache_seq.begin();
    SeqMap::const_iterator sb = gc->cache_seq.end();
    while (sb != sa && (--sb)->first >= start)
      {
        a.push_back (sb->second);
        if (sb->first == start)
          break;
      }
    cb(a,gc->seq,cc);
    stop();
    return true;
  }
};

void
GroupCache::LastUpdates (uint16_t start, uint8_t Timeout,
    GCLastCallback cb, ClientConnPtr cc)
{
  uint32_t st;
  // counter wraparound
  st = (seq&~0xFFFF) + start;
  if (st > seq)
    st -= 0x10000;
  else
    new GCTracker(this, st, Timeout, cb,cc);
}

void
GroupCache::LastUpdates2 (uint32_t start, uint8_t Timeout,
    GCLastCallback cb, ClientConnPtr cc)
{
  new GCTracker(this, start, Timeout, cb,cc);
}

