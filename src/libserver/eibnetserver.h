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
#include "link.h"
#include "server.h"
#include "lpdu.h"


class EIBnetServer;
typedef std::shared_ptr<EIBnetServer> EIBnetServerPtr;

typedef enum {
	CT_NONE = 0,
	CT_STANDARD,
	CT_BUSMONITOR,
	CT_CONFIG,
} ConnType;

class ConnState: public LineDriver, public L_Busmonitor_CallBack
{
public:
  ConnState (EIBnetServerPtr p, eibaddr_t addr);
  ~ConnState ();
  bool setup();
  void start();
  void stop(); // false when called from destructor

  EIBnetServer *parent;
  bool init();

  uchar channel;
  uchar sno;
  uchar rno;
  int retries;
  ConnType type = CT_NONE;
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

  void send_L_Data (LDataPtr l);
  void send_L_Busmonitor (LBusmonPtr l);
};
typedef std::shared_ptr<ConnState> ConnStatePtr;

class EIBnetDriver : public LineDriver
{
  EIBNetIPSocket *sock; // receive only

  void on_recv_cb(EIBNetIPPacket *p);
  p_recv_cb on_recv;

public:
  EIBnetDriver (EIBnetServerPtr parent, std::string& multicastaddr, int port, std::string& intf);
  virtual ~EIBnetDriver ();
  struct sockaddr_in maddr;

  bool setup();
  void start();
  void stop();

  void Send (EIBNetIPPacket p, struct sockaddr_in addr);

  void send_L_Data (LDataPtr l);
};

SERVER(EIBnetServer,router)
{
  friend class ConnState;
  friend class EIBnetDriver;

  EIBnetDriver *mcast;   // used for multicast receiving
  EIBNetIPSocket *sock;  // used for normal dialog
  int sock_mac;          // used to query the list of interfaces
  int Port;              // copy of sock->port()

  /** config */
  bool tunnel;
  bool route;
  bool discover;
  bool single_port;
  std::string multicastaddr;
  uint16_t port;
  std::string interface;
  std::string servername;

  Array < ConnStatePtr > connections;
  Queue < ConnStatePtr > drop_q;

  int addClient (ConnType type, const EIBnet_ConnectRequest & r1,
                 eibaddr_t addr = 0);
  void addNAT (const LDataPtr &&l);

  void on_recv_cb(EIBNetIPPacket *p);

public:
  EIBnetServer (BaseRouter& r, IniSection& s);
  virtual ~EIBnetServer ();
  bool setup ();
  void start();
  void stop();
  void handle_packet (EIBNetIPPacket *p1, EIBNetIPSocket *isock);

  void drop_connection (ConnStatePtr s);
  ev::async drop_trigger; void drop_trigger_cb(ev::async &w, int revents);

  inline void Send (EIBNetIPPacket p) {
    Send (p, mcast->maddr);
  }
  inline void Send (EIBNetIPPacket p, struct sockaddr_in addr) {
    if (sock)
      sock->Send (p, addr);
  }

  bool checkAddress(eibaddr_t addr) { return route; }
  bool checkGroupAddress(eibaddr_t addr) { return route; }
};
typedef std::shared_ptr<EIBnetServer> EIBnetServerPtr;

#endif
