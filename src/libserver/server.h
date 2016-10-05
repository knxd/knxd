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

#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include "layer3.h"

class ClientConnection;

/** implements the frontend (but opens no connection) */
class BaseServer:protected Thread, public Layer2virtual
{
  virtual void Run (pth_sem_t * stop) = 0;
  const char *Name() { return "baseserver"; }
protected:
  BaseServer (Trace * tr);
public:
  virtual ~BaseServer ();
};
typedef std::shared_ptr<BaseServer> BaseServerPtr;

/** implements the frontend (but opens no connection) */
class Server:public BaseServer
{
  /** open client connections*/
  Array < ClientConnection * >connections;

  void Run (pth_sem_t * stop);
  const char *Name() { return "server"; }
protected:
  /** server socket */
  int fd;

  virtual void setupConnection (int cfd);

  Server (Trace * tr);
public:
  virtual ~Server ();

  virtual bool init (Layer3 *l3);

  /** deregister client connection */
  bool deregister (ClientConnection * con);
};

#endif
