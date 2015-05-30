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

#ifndef GROUPCACHE_H
#define GROUPCACHE_H

#include <time.h>

#include "layer3.h"

typedef struct
{
  /** Layer 4 data */
  CArray data;
  /** source address */
  eibaddr_t src;
  /** destination address */
  eibaddr_t dst;
  /** receive time */
  time_t recvtime;
} GroupCacheEntry;

class GroupCache:public L_Data_CallBack
{
  /** Layer 3 interface */
  Layer3 *layer3;
  /** debug output */
  Trace *t;
  /** output queue */
  Array < GroupCacheEntry * >cache;
  /** controlled by .Start/Stop; if false, the whole code does nothing */
  bool enable;
  /** signal for .Read and .LastUpdates that the cache has been updated */
  pth_cond_t cond;
  /** serialize access to .cond */
  pth_mutex_t mutex;
  /** current position in 'updates' array */
  uint16_t pos;
  /** circular buffer of last-updated group addresses */
  eibaddr_t updates[0x100];

  /** find this group */
  GroupCacheEntry *find (eibaddr_t dst);
  /** add this entry */
  void add (GroupCacheEntry * entry);

public:
  /** constructor */
  GroupCache (Layer3 * l3, Trace * t);
  /** destructor */
  virtual ~GroupCache ();
  /** Feed data to the cache */
  void Send_L_Data (L_Data_PDU * l);

  /** Turn on caching, calls l3.registerGroupCallBack(ANY) */
  bool Start ();
  /** drop the whole cache */
  void Clear ();
  /** Turn off caching, deregisters */
  void Stop ();

  /** read, and optionally wait for, a cache entry for this address */
  GroupCacheEntry Read (eibaddr_t addr, unsigned timeout, uint16_t age);
  /** incrementally monitor group cache updates */
  Array < eibaddr_t > LastUpdates (uint16_t start, uint8_t timeout,
                                   uint16_t & end, pth_event_t stop);
  // remove this group
  void remove (eibaddr_t addr);
};

#endif
