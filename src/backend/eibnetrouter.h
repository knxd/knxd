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

#ifndef EIBNET_ROUTER_H
#define EIBNET_ROUTER_H

#include "layer2.h"
#include "eibnetip.h"

/** EIBnet/IP routing backend */
class EIBNetIPRouter:public Layer2
{
  /** EIBnet/IP socket */
  EIBNetIPSocket *sock;

  const char *Name() { return "eibnetrouter"; }
  void on_recv_cb(EIBNetIPPacket *p);
public:
  EIBNetIPRouter (const char *multicastaddr, int port, L2options *opt);
  virtual ~EIBNetIPRouter ();
  bool init (Layer3 *l3);

  void send_L_Data (L_Data_PDU * l);

};

#endif
