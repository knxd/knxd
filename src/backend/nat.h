/*
    EIBD eib bus access and management daemon
    Copyright (C) 2015 Matthias Urlichs <matthias@urlichs.de>

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

#ifndef NAT_H
#define NAT_H
#include "link.h"

/** NAT filter
 * outgoing packets: remember src/dest combination, zero src
 * incoming: restore dest
 */
typedef struct {
  eibaddr_t src;
  eibaddr_t dest;
} phys_comm;

FILTER(NatL2Filter,single)
{
  /** source addresses when the destination is my own */
  Array < phys_comm > revaddr; // TODO: replace with a map

public:
  eibaddr_t addr;

  NatL2Filter (const LinkConnectPtr_& c, IniSectionPtr& s) : Filter(c,s) {}
  virtual ~NatL2Filter ();

  DriverPtr clone(DriverPtr l2);
  bool setup();

  void recv_L_Data (LDataPtr l);
  void send_L_Data (LDataPtr l);

  void addReverseAddress (eibaddr_t src, eibaddr_t dest);
  eibaddr_t getDestinationAddress (eibaddr_t src);

  void setAddress(eibaddr_t addr) { this->addr = addr; }
};

#endif
