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

#ifndef EIBNET_TUNNEL_H
#define EIBNET_TUNNEL_H

#include "layer2.h"
#include "eibnetip.h"

class EIBNetIPTunnel:public Layer2, private Thread
{
  EIBNetIPSocket *sock;
  struct sockaddr_in caddr;
  struct sockaddr_in daddr;
  struct sockaddr_in saddr;
  struct sockaddr_in raddr;
  pth_sem_t insignal;
  Queue < CArray > inqueue;
  int dataport;
  bool NAT;
  pth_time_t send_delay;
  int support_busmonitor;
  int connect_busmonitor;

  void Run (pth_sem_t * stop);
  const char *Name() { return "eibnettunnel"; }
public:
  EIBNetIPTunnel (const char *dest, int port, int sport, const char *srcip,
                  int dataport, L2options *opt);
  virtual ~EIBNetIPTunnel ();
  bool init (Layer3 *l3);

  bool enterBusmonitor ();
  bool leaveBusmonitor ();

  void Send_L_Data (LPDU * l);

  bool Send_Queue_Empty ();
};


#endif
