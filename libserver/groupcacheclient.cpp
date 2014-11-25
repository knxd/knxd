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

#include "groupcacheclient.h"
#include "groupcache.h"
#include "client.h"

static GroupCache *cache = 0;

bool
CreateGroupCache (Layer3 * l3, Trace * t, bool enable)
{
  cache = new GroupCache (l3, t);
  if (enable)
    if (!cache->Start ())
      return false;
  return true;
}

void
DeleteGroupCache ()
{
  if (cache)
    delete cache;
}

void
GroupCacheRequest (Layer3 * l3, Trace * t, ClientConnection * c,
		   pth_event_t stop)
{
  GroupCacheEntry gc;
  CArray erg;
  eibaddr_t dst;
  uint16_t age = 0;

  if (!cache)
    {
      c->sendreject (stop);
      return;
    }
  switch (EIBTYPE (c->buf))
    {
    case EIB_CACHE_ENABLE:
      if (cache->Start ())
	c->sendreject (stop, EIB_CACHE_ENABLE);
      else
	c->sendreject (stop, EIB_CONNECTION_INUSE);
      break;
    case EIB_CACHE_DISABLE:
      cache->Stop ();
      c->sendreject (stop, EIB_CACHE_DISABLE);
      break;
    case EIB_CACHE_CLEAR:
      cache->Clear ();
      c->sendreject (stop, EIB_CACHE_CLEAR);
      break;
    case EIB_CACHE_REMOVE:
      if (c->size < 4)
	{
	  c->sendreject (stop);
	  return;
	}
      dst = (c->buf[2] << 8) | (c->buf[3]);
      cache->remove (dst);
      c->sendreject (stop, EIB_CACHE_REMOVE);
      break;

    case EIB_CACHE_READ:
    case EIB_CACHE_READ_NOWAIT:
      if (c->size < 4)
	{
	  c->sendreject (stop);
	  return;
	}
      dst = (c->buf[2] << 8) | (c->buf[3]);
      if (EIBTYPE (c->buf) == EIB_CACHE_READ)
	{
	  if (c->size < 6)
	    {
	      c->sendreject (stop);
	      return;
	    }
	  age = (c->buf[4] << 8) | (c->buf[5]);
	}
      gc =
	cache->Read (dst, EIBTYPE (c->buf) == EIB_CACHE_READ_NOWAIT ? 0 : 1,
		     age);
      erg.resize (6 + gc.data ());
      EIBSETTYPE (erg, EIBTYPE (c->buf));
      erg[2] = (gc.src >> 8) & 0xff;
      erg[3] = (gc.src >> 0) & 0xff;
      erg[4] = (gc.dst >> 8) & 0xff;
      erg[5] = (gc.dst >> 0) & 0xff;
      erg.setpart (gc.data, 6);
      c->sendmessage (erg (), erg.array (), stop);
      break;

    case EIB_CACHE_LAST_UPDATES:
      if (c->size < 5)
	{
	  c->sendreject (stop);
	  return;
	}
      {
	uint16_t end, start = (c->buf[2] << 8) | c->buf[3];
	uint8_t timeout = c->buf[4];
	Array < eibaddr_t > addrs =
	  cache->LastUpdates (start, timeout, end, stop);
	erg.resize (addrs () * 2 + 4);
	EIBSETTYPE (erg, EIBTYPE (c->buf));
	erg[2] = (end >> 8) & 0xff;
	erg[3] = (end >> 0) & 0xff;
	for (int i = 0; i < addrs (); i++)
	  {
	    erg[4 + i * 2] = (addrs[i] >> 8) & 0xff;
	    erg[4 + i * 2 + 1] = (addrs[i]) & 0xff;
	  }
	c->sendmessage (erg (), erg.array (), stop);
      }
      break;

    default:
      c->sendreject (stop);
    }
}
