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
#include "layer2.h"
#include "server.h"

class EIBnetServer;

typedef enum {
	CT_STANDARD = 0,
	CT_BUSMONITOR,
	CT_CONFIG,
} ConnType;

class ConnState: protected Thread, public Layer2mixin
{
public:
  ConnState (EIBnetServer *p, eibaddr_t addr);
  ~ConnState ();
  void Start () { Thread::Start(); }
  void Run(pth_sem_t*);

  EIBnetServer *parent;
  bool init();

  uchar channel;
  uchar sno;
  uchar rno;
  int state;
  ConnType type;
  int no;
  bool nat;
  pth_event_t timeout;
  Queue < CArray > out;

  struct sockaddr_in daddr;
  struct sockaddr_in caddr;
  pth_sem_t *outsignal;
  pth_event_t outwait;
  pth_event_t sendtimeout;

  // handle various packets from the connection
  void tunnel_request(EIBnet_TunnelRequest &r1, EIBNetIPSocket *isock);
  void tunnel_response(EIBnet_TunnelACK &r1);
  void config_request(EIBnet_ConfigRequest &r1, EIBNetIPSocket *isock);
  void config_response (EIBnet_ConfigACK &r1);

  void shutdown(void);
  void Send_L_Data (L_Data_PDU * l);
  const char * Name () { return "EIBnetConn"; } // TODO add a sequence number
};
typedef std::shared_ptr<ConnState> ConnStatePtr;

class EIBnetDiscover: protected Thread
{
  EIBnetServer *parent;
  EIBNetIPSocket *sock; // receive only

  void Run (pth_sem_t * stop);

public:
  EIBnetDiscover (EIBnetServer *parent, const char *multicastaddr, int port);
  virtual ~EIBnetDiscover ();
  struct sockaddr_in maddr;

  bool init (void);
  const char * Name () { return "EIBnetD"; }

  void Send (EIBNetIPPacket p, struct sockaddr_in addr);
};

class EIBnetServer: protected Thread, public L_Busmonitor_CallBack, public Layer2mixin
{
  friend class ConnState;
  friend class EIBnetDiscover;

  EIBnetDiscover *mcast; // used for multicast receiving
  EIBNetIPSocket *sock;  // used for normal dialog
  int sock_mac;          // used to query the list of interfaces
  int Port;              // copy of sock->port()

  /** Flags */
  bool tunnel;
  bool route;
  bool discover;

  int busmoncount;
  Array < ConnStatePtr > state;
  String name;

  void Run (pth_sem_t * stop);
  void Send_L_Data (L_Data_PDU * l);
  void Send_L_Busmonitor (L_Busmonitor_PDU * l);
  void addBusmonitor ();
  void delBusmonitor ();
  int addClient (ConnType type, const EIBnet_ConnectRequest & r1,
                 eibaddr_t addr = 0);
  void addNAT (const L_Data_PDU & l);
public:
  EIBnetServer (TracePtr tr, const String serverName);
  virtual ~EIBnetServer ();
  bool init (Layer3 *l3); // __attribute__((deprecated));
  bool init (Layer3 *l3,
             const char *multicastaddr, const int port,
             const bool tunnel, const bool route, const bool discover);
  void handle_packet (EIBNetIPPacket *p1, EIBNetIPSocket *isock);

  const char * Name () { return "EIBnet"; }
  void drop_state (ConnStatePtr s);
  void drop_state (uint8_t index);
  inline void Send (EIBNetIPPacket p) {
    Send (p, mcast->maddr);
  }
  inline void Send (EIBNetIPPacket p, struct sockaddr_in addr) {
    if (sock)
      sock->Send (p, addr);
  }

};
typedef std::shared_ptr<EIBnetServer> EIBnetServerPtr;

#endif
