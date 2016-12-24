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

bool
CreateGroupCache (Layer3 * l3, TracePtr t, bool enable)
{
  GroupCachePtr cache;
  if (l3->cache)
    return false;
  cache = GroupCachePtr(new GroupCache (t));
  if (!cache->init (l3))
    return false;
  if (enable)
    if (!cache->Start ())
      return false;
  l3->cache = cache;
  return true;
}

void
DeleteGroupCache (Layer3 * l3)
{
  l3->cache = 0;
}

void
GroupCacheRequest (ClientConnPtr c, uint8_t *buf, size_t len)
{
  GroupCacheEntry gc;
  CArray erg;
  eibaddr_t dst;
  uint16_t age = 0;
  GroupCachePtr cache = c->l3 ? c->l3->cache : 0;

  if (!cache)
    {
      c->sendreject ();
      return;
    }
  switch (EIBTYPE (buf))
    {
    case EIB_CACHE_ENABLE:
      if (cache->Start ())
	c->sendreject (EIB_CACHE_ENABLE);
      else
	c->sendreject (EIB_CONNECTION_INUSE);
      break;
    case EIB_CACHE_DISABLE:
      cache->Stop ();
      c->sendreject (EIB_CACHE_DISABLE);
      break;
    case EIB_CACHE_CLEAR:
      cache->Clear ();
      c->sendreject (EIB_CACHE_CLEAR);
      break;
    case EIB_CACHE_REMOVE:
      if (len < 4)
	{
	  c->sendreject ();
	  return;
	}
      dst = (buf[2] << 8) | (buf[3]);
      cache->remove (dst);
      c->sendreject (EIB_CACHE_REMOVE);
      break;

    case EIB_CACHE_READ:
    case EIB_CACHE_READ_NOWAIT:
      if (len < 4)
	{
	  c->sendreject ();
	  return;
	}
      dst = (buf[2] << 8) | (buf[3]);
      if (EIBTYPE (buf) == EIB_CACHE_READ)
	{
	  if (len < 6)
	    {
	      c->sendreject ();
	      return;
	    }
	  age = (buf[4] << 8) | (buf[5]);
	}
      gc =
	cache->Read (dst, EIBTYPE (buf) == EIB_CACHE_READ_NOWAIT ? 0 : 1,
		     age);
      erg.resize (6 + gc.data.size());
      EIBSETTYPE (erg, EIBTYPE (buf));
      erg[2] = (gc.src >> 8) & 0xff;
      erg[3] = (gc.src >> 0) & 0xff;
      erg[4] = (gc.dst >> 8) & 0xff;
      erg[5] = (gc.dst >> 0) & 0xff;
      erg.setpart (gc.data, 6);
      c->sendmessage (erg.size(), erg.data());
      break;

    case EIB_CACHE_LAST_UPDATES:
      if (len < 5)
	{
	  c->sendreject ();
	  return;
	}
      {
	uint16_t end, start = (buf[2] << 8) | buf[3];
	uint8_t timeout = buf[4];
	Array < eibaddr_t > addrs =
	  cache->LastUpdates (start, timeout, end);
	erg.resize (addrs.size() * 2 + 4);
	EIBSETTYPE (erg, EIBTYPE (buf));
	erg[2] = (end >> 8) & 0xff;
	erg[3] = (end >> 0) & 0xff;
	for (unsigned int i = 0; i < addrs.size(); i++)
	  {
	    erg[4 + i * 2] = (addrs[i] >> 8) & 0xff;
	    erg[4 + i * 2 + 1] = (addrs[i]) & 0xff;
	  }
	c->sendmessage (erg.size(), erg.data());
      }
      break;

    default:
      c->sendreject ();
    }
}
