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

GroupCache::GroupCache (TracePtr t)
	: Layer2mixin(t)
{
  TRACEPRINTF (t, 4, this, "GroupCacheInit");
  enable = 0;
  pos = 0;
  memset (updates, 0, sizeof (updates));
}

GroupCache::~GroupCache ()
{
  TRACEPRINTF (t, 4, this, "GroupCacheDestroy");
  Clear ();
}

GroupCacheEntry *
GroupCache::find (eibaddr_t dst)
{
  int l = 0, r = cache.size() - 1;
  while (l <= r)
    {
      int p = (l + r) / 2;
      if (cache[p]->dst == dst)
	return cache[p];
      if (dst > cache[p]->dst)
	l = p + 1;
      else
	r = p - 1;
    }
  return 0;
}

void
GroupCache::remove (eibaddr_t addr)
{
  TRACEPRINTF (t, 4, this, "GroupCacheRemove %s", FormatGroupAddr (addr).c_str());

  int l = 0, r = cache.size() - 1;
  while (l <= r)
    {
      int p = (l + r) / 2;
      if (cache[p]->dst == addr)
	{
	  delete cache[p];
	  cache.erase (cache.begin()+p);
	  return;
	}
      if (addr > cache[p]->dst)
	l = p + 1;
      else
	r = p - 1;
    }
  return;
}

void
GroupCache::add (GroupCacheEntry * entry)
{
  unsigned p;
  cache.resize (cache.size() + 1);
  p = cache.size() - 1;
  while (p > 0 && cache[p - 1]->dst > entry->dst)
    {
      cache[p] = cache[p - 1];
      p--;
    }
  cache[p] = entry;
}

void
GroupCache::Send_L_Data (L_Data_PDU * l)
{
  GroupCacheEntry *c;
  if (enable)
    {
      TPDU *t = TPDU::fromPacket (l->data, this->t);
      if (t->getType () == T_DATA_XXX_REQ)
	{
	  T_DATA_XXX_REQ_PDU *t1 = (T_DATA_XXX_REQ_PDU *) t;
	  if (t1->data.size() >= 2 && !(t1->data[0] & 0x3) &&
	      ((t1->data[1] & 0xC0) == 0x40 || (t1->data[1] & 0xC0) == 0x80))
	    {
	      c = find (l->dest);
	      updates[pos & 0xff] = l->dest;
	      pos++;
	      if (! c)
		{
		  c = new GroupCacheEntry(l->dest);
		  add (c);
		}
              c->src = l->source;
              c->data = t1->data;
              c->recvtime = time (0);
              updated(c);
	    }
	}
      delete t;
    }
  delete l;
}

bool
GroupCache::Start ()
{
  TRACEPRINTF (t, 4, this, "GroupCacheEnable");
  if (!enable)
    if (!addGroupAddress (0))
      return false;
  enable = 1;
  return true;
}

void
GroupCache::Clear ()
{
  unsigned int i;
  TRACEPRINTF (t, 4, this, "GroupCacheClear");
  ITER(i,cache)
    delete *i;
  cache.resize (0);
}

void
GroupCache::Stop ()
{
  Clear ();
  TRACEPRINTF (t, 4, this, "GroupCacheStop");
  if (enable)
    removeGroupAddress (0);
  enable = 0;
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
GroupCache::updated(GroupCacheEntry *c)
{
  // do this in reverse so that the update handler can safely remove itself
  unsigned int i = reader.size();
  while(i--)
    reader[i]->updated(c);
}

void
GroupCache::remove (GroupCacheReader * entry)
{
  ITER(i,reader)
    if (*i == entry)
      {
        reader.erase(i);
        return;
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
  bool stopped = false;

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
  ~GCReader() {}
  void stop() {
    stopped = true;
    GroupCacheReader::stop();
  }
private:
  void updated(GroupCacheEntry *c)
  {
    if (stopped)
      return;
    if (c->dst != addr)
      return;

    TRACEPRINTF (gc->t, 4, this, "GroupCache found: %s",
                  FormatEIBAddr (c->src).c_str());
    cb(*c,false,cc);
    stop();
  }

  void timeout_cb(ev::timer &w, int revents)
  {
    if (stopped)
      return;

    GroupCacheEntry f(addr);
    TRACEPRINTF (gc->t, 4, this, "GroupCache reread timeout");
    cb(f,false,cc);
    stop();
    return;
  }
};

void
GroupCache::Read (eibaddr_t addr, unsigned Timeout, uint16_t age,
  GCReadCallback cb, ClientConnPtr cc)
{
  TRACEPRINTF (t, 4, this, "GroupCacheRead %s %d %d",
	       FormatGroupAddr (addr).c_str(), Timeout, age);
  GroupCacheEntry *c;

  if (!enable)
    {
      GroupCacheEntry f(0);
      TRACEPRINTF (t, 4, this, "GroupCache not enabled");
      cb(f, Timeout == 0, cc);
      return;
    }

  c = find (addr);
  if (c && age && c->recvtime + age < time (0))
    c = nullptr;
  if (c)
    {
      TRACEPRINTF (t, 4, this, "GroupCache found: %s",
		   FormatEIBAddr (c->src).c_str());
      cb(*c, Timeout == 0, cc);
      return;
    }

  if (!Timeout)
    {
      GroupCacheEntry f(addr);
      TRACEPRINTF (t, 4, this, "GroupCache no entry");
      cb(f, true, cc);
      return;
    }

  // No data fond. Send a Read request.
  A_GroupValue_Read_PDU apdu;
  T_DATA_XXX_REQ_PDU tpdu;
  L_Data_PDU *l;

  GCReader *gcr = new GCReader(this,addr,Timeout,age, cb,cc);

  tpdu.data = apdu.ToPacket ();
  l = new L_Data_PDU (shared_from_this());
  l->data = tpdu.ToPacket ();
  l->source = 0;
  l->dest = addr;
  l->AddrType = GroupAddress;
  l3->recv_L_Data (l);
}

class GCTracker : protected GroupCacheReader
{
  GCLastCallback cb;
  ClientConnPtr cc;
  ev::timer timeout;
  Array < eibaddr_t > a;
  uint16_t start;
public:
  bool stopped = false;

  GCTracker(GroupCache *gc, uint16_t start, int Timeout,
           GCLastCallback cb, ClientConnPtr cc) : GroupCacheReader(gc)
  {
    this->cb = cb;
    this->cc = cc;
    this->start = start;
    timeout.set<GCTracker,&GCTracker::timeout_cb>(this);
    timeout.start(Timeout,0);
  }
  ~GCTracker() {}
  void stop() {
    stopped = true;
    GroupCacheReader::stop();
  }
private:
  void updated(GroupCacheEntry *c)
  {
    if (stopped)
      return;

    handler();
  }

  void timeout_cb(ev::timer &w, int revents)
  {
    if (stopped)
      return;

    handler();
    if (stopped)
      return;
    cb(a,gc->pos,cc);
    stop();
  }

  void handler()
  {
    if (gc->pos < 0x100)
      {
        if (gc->pos < start && start < ((gc->pos - 0x100) & 0xffff))
          start = (gc->pos - 0x100) & 0xffff;
      }
    else
      {
        if (start < ((gc->pos - 0x100) & 0xffff) || start > gc->pos)
          start = (gc->pos - 0x100) & 0xffff;
      }
    TRACEPRINTF (gc->t, 8, this, "LastUpdates start: %d pos: %d", start, gc->pos);
    while (start != gc->pos && !gc->updates[start & 0xff])
      start++;
    if (start != gc->pos)
      {
        while (start != gc->pos)
          {
            if (gc->updates[start & 0xff])
              a.push_back (gc->updates[start & 0xff]);
            start++;
          }
        cb(a,gc->pos,cc);
        stop();
      }
  }
};

void
GroupCache::LastUpdates (uint16_t start, uint8_t Timeout,
    GCLastCallback cb, ClientConnPtr cc)
{
  new GCTracker(this, start, Timeout, cb,cc);
}
