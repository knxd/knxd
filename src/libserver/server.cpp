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

#include "server.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <ev++.h>

#include "client.h"

void
NetServerBase::stop_(bool err)
{
  TRACEPRINTF (t, 8, "StopServer");

  io.stop();
  cleanup.stop();
  while(!cleanup_q.empty())
    cleanup_q.pop();

  ITER(i,connections)
  (*i)->stop(err);
  connections.clear();

  if (fd > -1)
    {
      close (fd);
      fd = -1;
    }
}

void
NetServerBase::stop(bool err)
{
  stop_(err);
  stopped(err);
}

NetServerBase::~NetServerBase ()
{
  // stopped() may not be called from a destructor
  stop_(false);
}

void
NetServerBase::deregister (ClientConnBasePtr con)
{
  cleanup_q.push(con);
  cleanup.send();
}

void
NetServerBase::cleanup_cb (ev::async &, int)
{
  while (!cleanup_q.empty())
    {
      ClientConnBasePtr con = cleanup_q.get();

      ITER(i, connections)
      if (*i == con)
        {
          connections.erase (i);
          break;
        }
    }
}

NetServerBase::NetServerBase (BaseRouter& r, IniSectionPtr& s) : Server (r,s)
{
  t->setAuxName("NetServ");
  fd = -1;
}

void
NetServerBase::start()
{
  if (fd == -1)
    {
      stopped(true);
      return;
    }
  set_non_blocking(fd);
  io.set<NetServerBase, &NetServerBase::io_cb>(this);
  io.start(fd,ev::READ);
  cleanup.set<NetServerBase, &NetServerBase::cleanup_cb>(this);
  cleanup.start();

  started();
}

void
NetServerBase::io_cb (ev::io &, int)
{
  int cfd;
  cfd = accept (fd, NULL,NULL);
  if (cfd != -1)
    {
      TRACEPRINTF (t, 8, "New Connection");
      setupConnection (cfd);
      ClientConnBasePtr c = createConnection(cfd);
      if (!c)
        return;
      if (!c->setup())
        return;
      c->start();
      if (c->running)
        connections.push_back(c);
    }
  else if (errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
    ERRORPRINTF (t, E_ERROR | 97, "Accept %s: %s", name(), strerror(errno));
}

bool
NetServerBase::setup()
{
  if (!Server::setup())
    return false;
  if (!static_cast<Router&>(router).hasClientAddrs())
    return false;
  if (!static_cast<Router &>(router).checkStack(cfg))
    return false;
  return true;
}

void
NetServerBase::setupConnection (int)
{
  ignore_when_systemd = cfg->value("systemd-ignore",ignore_when_systemd);
}

NetServer::NetServer (BaseRouter& r, IniSectionPtr& s) : NetServerBase (r,s)
{
}

NetServer::~NetServer ()
{
}

ClientConnBasePtr
NetServer::createConnection(int cfd)
{
  return std::shared_ptr<ClientConnection>(new ClientConnection (std::static_pointer_cast<NetServer>(shared_from_this()), cfd));
}
