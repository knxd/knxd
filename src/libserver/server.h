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
 * @addtogroup Server
 * @{
 */

#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include "link.h"
#include "router.h"

class ClientConnectionBase;
using ClientConnBasePtr = std::shared_ptr<ClientConnectionBase>;

/** implements the frontend (but opens no connection) */
class NetServerBase: public Server
{
  friend class ClientConnection;

public:
  virtual ~NetServerBase ();
  bool ignore_when_systemd = false;

protected:
  NetServerBase (BaseRouter& l3, IniSectionPtr& s);

  /** server socket */
  int fd;

  virtual ClientConnBasePtr createConnection (int cfd) = 0;

  virtual void setupConnection (int cfd);

  bool setup();
  void start();
  void stop(bool err);

  /** deregister client connection */
  void deregister (ClientConnBasePtr con);

private:
  ev::io io;
  void io_cb (ev::io &w, int revents);

  /** open client connections*/
  std::vector < ClientConnBasePtr > connections;

  ev::async cleanup;
  void cleanup_cb (ev::async &w, int revents);

  /** to-be-closed client connections*/
  Queue < ClientConnBasePtr > cleanup_q;
  void stop_(bool err);
};

class NetServer: public NetServerBase
{
protected:
  NetServer (BaseRouter& l3, IniSectionPtr& s);
  virtual ~NetServer ();

  ClientConnBasePtr createConnection(int cfd);
};

using NetServerPtr = std::shared_ptr<NetServer>;

#endif

/** @} */
