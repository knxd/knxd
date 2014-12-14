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

#ifndef EIBNET_SERVER_H
#define EIBNET_SERVER_H

#include "eibnetip.h"
#include "layer3.h"

typedef struct
{
  uchar channel;
  uchar sno;
  uchar rno;
  int state;
  int type;
  int no;
  bool nat;
  pth_event_t timeout;
    Queue < CArray > out;
  struct sockaddr_in daddr;
  struct sockaddr_in caddr;
  pth_sem_t *outsignal;
  pth_event_t outwait;
  pth_event_t sendtimeout;
} ConnState;

typedef struct
{
  eibaddr_t src;
  eibaddr_t dest;
  pth_event_t timeout;
} NATState;

class EIBnetServer:public L_Data_CallBack, public L_Busmonitor_CallBack,
  private Thread
{
  Layer3 *l3;
  Trace *t;
  EIBNetIPSocket *sock;
  int Port;
  bool tunnel;
  bool route;
  bool discover;
  int busmoncount;
  struct sockaddr_in maddr;
    Array < ConnState > state;
    Array < NATState > natstate;

  void Run (pth_sem_t * stop);
  void Get_L_Data (L_Data_PDU * l);
  void Get_L_Busmonitor (L_Busmonitor_PDU * l);
  void addBusmonitor ();
  void delBusmonitor ();
  int addClient (int type, const EIBnet_ConnectRequest & r1);
  void addNAT (const L_Data_PDU & l);
public:
    EIBnetServer (const char *multicastaddr, int port, bool Tunnel,
		  bool Route, bool Discover, Layer3 * layer3, Trace * tr);
    virtual ~ EIBnetServer ();
  bool init ();

};

#endif
