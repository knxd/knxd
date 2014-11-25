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

class EIBNetIPTunnel:public Layer2Interface, private Thread
{
  Trace *t;
  eibaddr_t addr;
  EIBNetIPSocket *sock;
  struct sockaddr_in caddr;
  struct sockaddr_in daddr;
  struct sockaddr_in saddr;
  struct sockaddr_in raddr;
  pth_sem_t insignal;
  pth_sem_t outsignal;
  pth_event_t getwait;
    Queue < CArray > inqueue;
    Queue < LPDU * >outqueue;
  int mode;
  int vmode;
  int dataport;
  bool NAT;
  bool noqueue;
  int support_busmonitor;
  int connect_busmonitor;

  void Run (pth_sem_t * stop);
public:
    EIBNetIPTunnel (const char *dest, int port, int sport, const char *srcip,
		    int dataport, int flags, Trace * tr);
    virtual ~ EIBNetIPTunnel ();
  bool init ();

  void Send_L_Data (LPDU * l);
  LPDU *Get_L_Data (pth_event_t stop);

  bool addAddress (eibaddr_t addr);
  bool addGroupAddress (eibaddr_t addr);
  bool removeAddress (eibaddr_t addr);
  bool removeGroupAddress (eibaddr_t addr);

  bool enterBusmonitor ();
  bool leaveBusmonitor ();

  bool openVBusmonitor ();
  bool closeVBusmonitor ();

  bool Open ();
  bool Close ();
  eibaddr_t getDefaultAddr ();
  bool Connection_Lost ();
  bool Send_Queue_Empty ();
};


#endif
