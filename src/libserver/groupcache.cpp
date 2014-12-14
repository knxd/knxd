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

GroupCache::GroupCache (Layer3 * l3, Trace * t)
{
  TRACEPRINTF (t, 4, this, "GroupCacheInit");
  this->t = t;
  this->layer3 = l3;
  this->enable = 0;
  pos = 0;
  memset (updates, 0, sizeof (updates));
  pth_mutex_init (&mutex);
  pth_cond_init (&cond);
}

GroupCache::~GroupCache ()
{
  TRACEPRINTF (t, 4, this, "GroupCacheDestroy");
  if (enable)
    layer3->deregisterGroupCallBack (this, 0);
  Clear ();
}

GroupCacheEntry *
GroupCache::find (eibaddr_t dst)
{
  int l = 0, r = cache () - 1;
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
  TRACEPRINTF (t, 4, this, "GroupCacheRemove %d/%d/%d", (addr >> 11) & 0x1f,
	       (addr >> 8) & 0x07, (addr) & 0xff);

  int l = 0, r = cache () - 1;
  while (l <= r)
    {
      int p = (l + r) / 2;
      if (cache[p]->dst == addr)
	{
	  delete cache[p];
	  cache.deletepart (p, 1);
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
  cache.resize (cache () + 1);
  p = cache () - 1;
  while (p > 0 && cache[p - 1]->dst > entry->dst)
    {
      cache[p] = cache[p - 1];
      p--;
    }
  cache[p] = entry;
}


void
GroupCache::Get_L_Data (L_Data_PDU * l)
{
  GroupCacheEntry *c;
  if (enable)
    {
      TPDU *t = TPDU::fromPacket (l->data);
      if (t->getType () == T_DATA_XXX_REQ)
	{
	  T_DATA_XXX_REQ_PDU *t1 = (T_DATA_XXX_REQ_PDU *) t;
	  if (t1->data () >= 2 && !(t1->data[0] & 0x3) &&
	      ((t1->data[1] & 0xC0) == 0x40 || (t1->data[1] & 0xC0) == 0x80))
	    {
	      c = find (l->dest);
	      updates[pos & 0xff] = l->dest;
	      pos++;
	      if (c)
		{
		  c->data = t1->data;
		  c->src = l->source;
		  c->dst = l->dest;
		  c->recvtime = time (0);
		  pth_cond_notify (&cond, 1);
		}
	      else
		{
		  c = new GroupCacheEntry;
		  c->data = t1->data;
		  c->src = l->source;
		  c->dst = l->dest;
		  c->recvtime = time (0);
		  add (c);
		  pth_cond_notify (&cond, 1);
		}
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
    if (!layer3->registerGroupCallBack (this, 0))
      return false;
  enable = 1;
  return true;
}

void
GroupCache::Clear ()
{
  int i;
  TRACEPRINTF (t, 4, this, "GroupCacheClear");
  for (i = 0; i < cache (); i++)
    delete cache[i];
  cache.resize (0);
}

void
GroupCache::Stop ()
{
  Clear ();
  TRACEPRINTF (t, 4, this, "GroupCacheStop");
  if (enable)
    layer3->deregisterGroupCallBack (this, 0);
  enable = 0;
}

GroupCacheEntry
  GroupCache::Read (eibaddr_t addr, unsigned Timeout, uint16_t age)
{
  TRACEPRINTF (t, 4, this, "GroupCacheRead %d/%d/%d %d %d",
	       (addr >> 11) & 0x1f, (addr >> 8) & 0x07, (addr) & 0xff,
	       Timeout, age);
  bool rm = false;
  GroupCacheEntry *c;
  if (!enable)
    {
      GroupCacheEntry f;
      f.src = 0;
      f.dst = 0;
      TRACEPRINTF (t, 4, this, "GroupCache not enabled");
      return f;
    }

  c = find (addr);
  if (c && age && c->recvtime + age < time (0))
    rm = true;

  if (c && !rm)
    {
      TRACEPRINTF (t, 4, this, "GroupCache found: %d.%d.%d",
		   (c->src >> 12) & 0xf, (c->src >> 8) & 0xf,
		   (c->src) & 0xff);
      return *c;
    }

  if (!Timeout)
    {
      GroupCacheEntry f;
      f.src = 0;
      f.dst = addr;
      TRACEPRINTF (t, 4, this, "GroupCache no entry");
      return f;
    }

  A_GroupValue_Read_PDU apdu;
  T_DATA_XXX_REQ_PDU tpdu;
  L_Data_PDU *l;
  pth_event_t timeout = pth_event (PTH_EVENT_RTIME, pth_time (Timeout, 0));

  tpdu.data = apdu.ToPacket ();
  l = new L_Data_PDU;
  l->data = tpdu.ToPacket ();
  l->source = 0;
  l->dest = addr;
  l->AddrType = GroupAddress;
  layer3->send_L_Data (l);

  do
    {
      c = find (addr);
      rm = false;
      if (c && age && c->recvtime + age < time (0))
	rm = true;

      if (c && !rm)
	{
	  TRACEPRINTF (t, 4, this, "GroupCache found: %d.%d.%d",
		       (c->src >> 12) & 0xf, (c->src >> 8) & 0xf,
		       (c->src) & 0xff);
	  pth_event_free (timeout, PTH_FREE_THIS);
	  return *c;
	}

      if (pth_event_status (timeout) == PTH_STATUS_OCCURRED && c)
	{
	  GroupCacheEntry gc;
	  gc.src = 0;
	  gc.dst = addr;
	  TRACEPRINTF (t, 4, this, "GroupCache reread timeout");
	  pth_event_free (timeout, PTH_FREE_THIS);
	  return gc;
	}

      if (pth_event_status (timeout) == PTH_STATUS_OCCURRED)
	{
	  c = new GroupCacheEntry;
	  c->src = 0;
	  c->dst = addr;
	  c->recvtime = time (0);
	  add (c);
	  TRACEPRINTF (t, 4, this, "GroupCache timeout");
	  pth_event_free (timeout, PTH_FREE_THIS);
	  return *c;
	}

      pth_mutex_acquire (&mutex, 0, 0);
      pth_cond_await (&cond, &mutex, timeout);
      pth_mutex_release (&mutex);
    }
  while (1);
}

Array < eibaddr_t > GroupCache::LastUpdates (uint16_t start, uint8_t Timeout,
					     uint16_t & end, pth_event_t stop)
{
  Array < eibaddr_t > a;
  pth_event_t timeout = pth_event (PTH_EVENT_RTIME, pth_time (Timeout, 0));

  do
    {
      if (pos < 0x100)
	{
	  if (pos < start && start < ((pos - 0x100) & 0xffff))
	    start = (pos - 0x100) & 0xffff;
	}
      else
	{
	  if (start < ((pos - 0x100) & 0xffff) || start > pos)
	    start = (pos - 0x100) & 0xffff;
	}
      TRACEPRINTF (t, 8, this, "LastUpdates start: %d pos: %d", start, pos);
      while (start != pos && !updates[start & 0xff])
	start++;
      if (start != pos)
	{
	  while (start != pos)
	    {
	      if (updates[start & 0xff])
		{
		  a.resize (a () + 1);
		  a[a () - 1] = updates[start & 0xff];
		}
	      start++;
	    }
	  end = pos;
	  pth_event_free (timeout, PTH_FREE_THIS);
	  return a;
	}
      if (pth_event_status (timeout) == PTH_STATUS_OCCURRED)
	{
	  end = pos;
	  pth_event_free (timeout, PTH_FREE_THIS);
	  return a;
	}
      pth_event_concat (timeout, stop, NULL);
      pth_mutex_acquire (&mutex, 0, 0);
      pth_cond_await (&cond, &mutex, timeout);
      pth_mutex_release (&mutex);
      pth_event_isolate (timeout);
    }
  while (1);
}
