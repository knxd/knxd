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

#include <ev++.h>
#include "callbacks.h"
#include "eibnetip.h"
#include "layer3.h"
#include "layer2.h"
#include "server.h"
#include "lpdu.h"


class EIBnetServer;

typedef enum {
	CT_STANDARD = 0,
	CT_BUSMONITOR,
	CT_CONFIG,
} ConnType;

class ConnState: public Layer2mixin, public L_Busmonitor_CallBack
{
public:
  ConnState (EIBnetServer *p, eibaddr_t addr);
  ~ConnState ();
  void stop(); // false when called from destructor

  EIBnetServer *parent;
  bool init();

  uchar channel;
  uchar sno;
  uchar rno;
  int retries;
  ConnType type;
  int no;
  bool nat;

  ev::timer timeout; void timeout_cb(ev::timer &w, int revents);
  ev::timer sendtimeout; void sendtimeout_cb(ev::timer &w, int revents);
  ev::async send_trigger; void send_trigger_cb(ev::async &w, int revents);
  Queue < CArray > out;
  void reset_timer();

  struct sockaddr_in daddr;
  struct sockaddr_in caddr;

  // handle various packets from the connection
  void tunnel_request(EIBnet_TunnelRequest &r1, EIBNetIPSocket *isock);
  void tunnel_response(EIBnet_TunnelACK &r1);
  void config_request(EIBnet_ConfigRequest &r1, EIBNetIPSocket *isock);
  void config_response (EIBnet_ConfigACK &r1);

  void send_L_Data (L_Data_PDU * l);
  void send_L_Busmonitor (L_Busmonitor_PDU * l);
  const char * Name () { return "EIBnetConn"; } // TODO add a sequence number
};
typedef std::shared_ptr<ConnState> ConnStatePtr;

class EIBnetDiscover
{
  EIBnetServer *parent;
  EIBNetIPSocket *sock; // receive only

  void on_recv_cb(EIBNetIPPacket *p);
  p_recv_cb on_recv;

public:
  EIBnetDiscover (EIBnetServer *parent, const char *multicastaddr, int port, const char *intf);
  virtual ~EIBnetDiscover ();
  struct sockaddr_in maddr;

  bool init (void);
  void stop();
  const char * Name () { return "EIBnetD"; }

  void Send (EIBNetIPPacket p, struct sockaddr_in addr);
};

class EIBnetServer: public Layer2mixin
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
  bool single_port;

  Array < ConnStatePtr > connections;
  Queue < ConnStatePtr > drop_q;
  String name;

  void send_L_Data (L_Data_PDU * l);
  void send_L_Busmonitor (L_Busmonitor_PDU * l);
  int addClient (ConnType type, const EIBnet_ConnectRequest & r1,
                 eibaddr_t addr = 0);
  void addNAT (const L_Data_PDU & l);

  void on_recv_cb(EIBNetIPPacket *p);

public:
  EIBnetServer (TracePtr tr, const String serverName);
  virtual ~EIBnetServer ();
  bool setup (const char *multicastaddr, const int port, const char *intf,
              const bool tunnel, const bool route, const bool discover,
              const bool single_port);
  void stop();
  void handle_packet (EIBNetIPPacket *p1, EIBNetIPSocket *isock);

  const char * Name () { return "EIBnet:"; }
  void drop_connection (ConnStatePtr s);
  ev::async drop_trigger; void drop_trigger_cb(ev::async &w, int revents);

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
