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
#include <sys/types.h>
#include <sys/socket.h>
#include <ev++.h>
#include "server.h"
#include "client.h"

void
NetServer::stop_()
{
  TRACEPRINTF (t, 8, "StopServer");

  io.stop();
  cleanup.stop();
  while(!cleanup_q.empty())
    cleanup_q.pop();

  ITER(i,connections)
    (*i)->stop();
  connections.resize(0);

  if (fd > -1)
    {
      close (fd);
      fd = -1;
    }
}

void
NetServer::stop()
{
  stop_();
  stopped();
}

NetServer::~NetServer ()
{
  // stopped() may not be called from a destructor
  stop_();
}

void
NetServer::deregister (ClientConnPtr con)
{
  cleanup_q.push(con);
  cleanup.send();
}

void
NetServer::cleanup_cb (ev::async &w, int revents)
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

NetServer::NetServer (BaseRouter& l3, IniSection& s) : Server (l3,s)
{
  fd = -1;
}

void
NetServer::start()
{
  if (fd == -1)
    {
      stopped();
      return;
    }
  set_non_blocking(fd);
  io.set<NetServer, &NetServer::io_cb>(this);
  io.start(fd,ev::READ);
  cleanup.set<NetServer, &NetServer::cleanup_cb>(this);
  cleanup.start();

  started();
}

void
NetServer::io_cb (ev::io &w, int revents)
{
  int cfd;
  cfd = accept (fd, NULL,NULL);
  if (cfd != -1)
    {
      TRACEPRINTF (t, 8, "New Connection");
      setupConnection (cfd);
      ClientConnPtr c = std::shared_ptr<ClientConnection>(new ClientConnection (std::static_pointer_cast<NetServer>(shared_from_this()), cfd));
      if (!c->setup())
        return;
      c->start();
      if (c->running)
        connections.push_back(c);
    }
  else if (errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
    ERRORPRINTF (t, E_ERROR | 51, "Accept %s: %s", name(), strerror(errno));
}

void
NetServer::setupConnection (int cfd UNUSED)
{
  ignore_when_systemd = cfg.value("systemd-ignore",ignore_when_systemd);
}
