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
CreateGroupCache (Router& r, IniSectionPtr& s)
{
  LinkConnectPtr c = LinkConnectPtr(new LinkConnect(r,s,r.t));
  GroupCachePtr cache;
  if (r.getCache())
    return false;
  cache = GroupCachePtr(new GroupCache (c,s));
  c->set_driver(cache);
  if (!c->setup())
    return false;

  if (!r.registerLink(c))
    return false;
  r.setCache(cache);
  return true;
}

void
ReadCallback(const GroupCacheEntry &gce, bool nowait, ClientConnPtr c)
{
  CArray erg;

  erg.resize (6 + gce.data.size());
  EIBSETTYPE (erg, nowait ? EIB_CACHE_READ_NOWAIT : EIB_CACHE_READ);
  erg[2] = (gce.src >> 8) & 0xff;
  erg[3] = (gce.src >> 0) & 0xff;
  erg[4] = (gce.dst >> 8) & 0xff;
  erg[5] = (gce.dst >> 0) & 0xff;
  erg.setpart (gce.data, 6);
  c->sendmessage (erg.size(), erg.data());
}

void
LastUpdatesCallback(const Array<eibaddr_t> &addrs, uint32_t end, ClientConnPtr c)
{
  CArray erg;

  erg.resize (addrs.size() * 2 + 4);
  EIBSETTYPE (erg, EIB_CACHE_LAST_UPDATES);
  erg[2] = (end >> 8) & 0xff;
  erg[3] = (end >> 0) & 0xff;
  for (unsigned int i = 0; i < addrs.size(); i++)
    {
      erg[4 + i * 2] = (addrs[i] >> 8) & 0xff;
      erg[4 + i * 2 + 1] = (addrs[i]) & 0xff;
    }
  c->sendmessage (erg.size(), erg.data());
}

void
LastUpdates2Callback(const Array<eibaddr_t> &addrs, uint32_t end, ClientConnPtr c)
{
  CArray erg;

  erg.resize (addrs.size() * 2 + 6);
  EIBSETTYPE (erg, EIB_CACHE_LAST_UPDATES_2);
  erg[2] = (end >> 24) & 0xff;
  erg[3] = (end >> 16) & 0xff;
  erg[4] = (end >> 8) & 0xff;
  erg[5] = (end >> 0) & 0xff;
  for (unsigned int i = 0; i < addrs.size(); i++)
    {
      erg[6 + i * 2] = (addrs[i] >> 8) & 0xff;
      erg[6 + i * 2 + 1] = (addrs[i]) & 0xff;
    }
  c->sendmessage (erg.size(), erg.data());
}

void
GroupCacheRequest (ClientConnPtr c, uint8_t *buf, size_t len)
{
  eibaddr_t dst;
  uint16_t age = 0;
  Router& r = c->router;
  GroupCachePtr cache = r.getCache();

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
      cache->Read (dst, EIBTYPE (buf) == EIB_CACHE_READ_NOWAIT ? 0 : 1,
		     age, ReadCallback, c);
      break;

    case EIB_CACHE_LAST_UPDATES:
      {
        if (len < 5)
          {
            c->sendreject ();
            return;
          }
        uint16_t start = (buf[2] << 8) | buf[3];
        uint8_t timeout = buf[4];
        cache->LastUpdates (start, timeout, &LastUpdatesCallback, c);
        break;
      }

    case EIB_CACHE_LAST_UPDATES_2:
      {
        if (len < 7)
          {
            c->sendreject ();
            return;
          }
        uint32_t start = (buf[2] << 24) | (buf[3] << 16) | (buf[4] << 8) | buf[5];
        uint8_t timeout = buf[6];
        cache->LastUpdates2 (start, timeout, &LastUpdates2Callback, c);
        break;
      }

    default:
      c->sendreject ();
    }
}

