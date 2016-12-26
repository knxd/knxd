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

#include <unistd.h>
#include <ev++.h>
#include "server.h"
#include "client.h"


BaseServer::BaseServer (TracePtr tr)
	: Layer2virtual (tr)
{
}

BaseServer::~BaseServer ()
{
  TRACEPRINTF (t, 8, this, "StopBaseServer");
  if (l3)
    l3->deregisterServer (this);
}

Server::~Server ()
{
  TRACEPRINTF (t, 8, this, "StopServer");
  io.stop();
  cleanup.stop();
  ITER(i,connections)
    (*i)->stop(true);

  if (fd != -1)
    close (fd);
}

void
Server::deregister (ClientConnPtr con)
{
  cleanup_q.put(con);
  cleanup.send();
}

void
Server::cleanup_cb (ev::async &w, int revents)
{
  while (!cleanup_q.isempty())
    {
      ClientConnPtr con = cleanup_q.get();

      ITER(i, connections)
        if (*i == con)
          {
	    connections.erase (i);
	    break;
          }
    }
}

Server::Server (TracePtr tr)
    : BaseServer (tr)
{
  fd = -1;
}

bool
Server::init (Layer3 *l3)
{
  if (fd == -1)
    return false;
  set_non_blocking(fd);
  io.set<Server, &Server::io_cb>(this);
  io.start(fd,ev::READ);
  cleanup.set<Server, &Server::cleanup_cb>(this);
  cleanup.start();

  return BaseServer::init(l3);
}

void
Server::io_cb (ev::io &w, int revents)
{
  int cfd;
  cfd = accept (fd, NULL,NULL);
  if (cfd != -1)
    {
      TRACEPRINTF (t, 8, this, "New Connection");
      setupConnection (cfd);
      ClientConnPtr c = std::shared_ptr<ClientConnection>(new ClientConnection (this, cfd));
      connections.push_back(c);
    }
  else if (errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
    ERRORPRINTF (t, E_ERROR | 51, this, "Accept %s: %s", Name(), strerror(errno));
}

void
Server::setupConnection (int cfd UNUSED)
{
}
