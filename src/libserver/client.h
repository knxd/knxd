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

#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include "eibtypes.h"
#include "iobuf.h"

/** reads the type of a eibd packet */
#define EIBTYPE(buf) (((buf)[0]<<8)|((buf)[1]))
/** sets the type of a eibd packet*/
#define EIBSETTYPE(buf,type) do{(buf)[0]=((type)>>8)&0xff;(buf)[1]=(type)&0xff;}while(0)

class NetServer;
typedef std::shared_ptr<NetServer> NetServerPtr;

class Router;
class A__Base;

/** implements a client connection */
class ClientConnection : public std::enable_shared_from_this<ClientConnection>
{
  /** client connection */
  int fd;
public:
  bool running = false;

  /** Layer 3 interface */
  Router &router;
  /** my address */
  eibaddr_t addr;
  /** debug output */
  TracePtr t;
  /** server creating this connection */
  NetServerPtr server;

protected:
  /** sending */
  SendBuf sendbuf;
  RecvBuf recvbuf;
  A__Base *a_conn = 0;

  void exit_conn();

public:
  ClientConnection (NetServerPtr s, int fd);
  virtual ~ClientConnection ();
  bool setup();
  void start();
  void stop();

  size_t read_cb(uint8_t *buf, size_t len);
  void error_cb();

  /** send a message */
  void sendmessage (int size, const uchar * msg);
  /** send a reject */
  void sendreject ();
  /** sends a reject with code @code */
  void sendreject (int code);
};
typedef std::shared_ptr<ClientConnection> ClientConnPtr;

#endif
