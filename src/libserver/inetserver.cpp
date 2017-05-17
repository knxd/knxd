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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "inetserver.h"

InetServer::InetServer (BaseRouter& r, IniSectionPtr& s)
  : NetServer(r,s)
{
  t->setAuxName("inet");
}

bool
InetServer::setup()
{
  if (!NetServer::setup())
    return false;
  port = cfg->value("port",6720);
  ignore_when_systemd = cfg->value("systemd-ignore",(port == 6720));
  return true;
}

void
InetServer::start()
{
  struct sockaddr_in addr;
  int reuse = 1;

  if (ignore_when_systemd && static_cast<Router &>(router).using_systemd)
    {
      may_fail = true;
      stopped();
      return;
    }

  TRACEPRINTF (t, 8, "OpenInetSocket %d", port);
  memset (&addr, 0, sizeof (addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);
  addr.sin_addr.s_addr = htonl (INADDR_ANY);

  fd = socket (AF_INET, SOCK_STREAM, 0);
  if (fd == -1)
    {
      ERRORPRINTF (t, E_ERROR | 12, "OpenInetSocket %d: socket: %s", port, strerror(errno));
      goto ex1;
    }

  setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse));

  if (bind (fd, (struct sockaddr *) &addr, sizeof (addr)) == -1)
    {
      ERRORPRINTF (t, E_ERROR | 13, "OpenInetSocket %d: bind: %s", port, strerror(errno));
      goto ex2;
    }

  if (listen (fd, 10) == -1)
    {
      ERRORPRINTF (t, E_ERROR | 14, "OpenInetSocket %d: listen: %s", port, strerror(errno));
      goto ex2;
    }

  TRACEPRINTF (t, 8, "InetSocket opened");
  NetServer::start();
  return;

ex2:
  close (fd);
  fd = -1;
ex1:
  stop();
  return;
}

void
InetServer::stop()
{
  if (fd >= 0)
    {
      close(fd);
      fd = -1;
    }
  NetServer::stop();
}

InetServer::~InetServer()
{
  if (fd >= 0)
    close(fd);
}

void
InetServer::setupConnection (int cfd)
{
  int val = 1;
  setsockopt (cfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof (val));
}
