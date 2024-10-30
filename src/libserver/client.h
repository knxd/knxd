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

/**
 * @file
 * @addtogroup Client
 * @{
 */

#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include "eibtypes.h"
#include "iobuf.h"
#include "router.h"
#include "server.h"

/** reads the type of a eibd packet */
#define EIBTYPE(buf) (((buf)[0]<<8)|((buf)[1]))
/** sets the type of a eibd packet*/
#define EIBSETTYPE(buf,type) do{(buf)[0]=((type)>>8)&0xff;(buf)[1]=(type)&0xff;}while(0)

class A__Base;

/** a client connection, either for the knxd protocol or for tcp tunnelling */
class ClientConnectionBase
{
public:
  virtual ~ClientConnectionBase ();

  virtual bool setup() = 0;
  virtual void start() = 0;
  virtual void stop(bool err) = 0;

  bool running = false;
};

/** implements a knxd protocol client connection */
class ClientConnection : public ClientConnectionBase, public std::enable_shared_from_this<ClientConnection>
{
public:
  /** Layer 3 interface */
  Router &router;
  /** my address */
  eibaddr_t addr;
  /** debug output */
  TracePtr t;
  /** server creating this connection */
  NetServerPtr server;

  ClientConnection (NetServerPtr s, int fd);
  virtual ~ClientConnection ();
  bool setup();
  void start();
  void stop(bool err);

  size_t read_cb(uint8_t *buf, size_t len);
  void error_cb();

  /** send a message */
  void sendmessage (int size, const uint8_t * msg);
  /** send a reject */
  void sendreject ();
  /** sends a reject with code @code */
  void sendreject (int code);

protected:
  /** sending */
  SendBuf sendbuf;
  RecvBuf recvbuf;
  A__Base *a_conn = nullptr;

  void exit_conn();

private:
  /** client connection */
  int fd;
};

using ClientConnPtr = std::shared_ptr<ClientConnection>;

#endif

/** @} */
