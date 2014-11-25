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
#include "inetserver.h"

InetServer::InetServer (Layer3 * la3, Trace * tr, int port):
Server (la3, tr)
{
  struct sockaddr_in addr;
  int reuse = 1;
  TRACEPRINTF (tr, 8, this, "OpenInetSocket %d", port);
  memset (&addr, 0, sizeof (addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);
  addr.sin_addr.s_addr = htonl (INADDR_ANY);

  fd = socket (AF_INET, SOCK_STREAM, 0);
  if (fd == -1)
    return;

  setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse));

  if (bind (fd, (struct sockaddr *) &addr, sizeof (addr)) == -1)
    {
      close (fd);
      fd = -1;
      return;
    }

  if (listen (fd, 10) == -1)
    {
      close (fd);
      fd = -1;
      return;
    }

  TRACEPRINTF (tr, 8, this, "InetSocket opened");
  Start ();
}

bool
InetServer::init ()
{
  return fd != -1;
}

void
InetServer::setupConnection (int cfd)
{
  int val = 1;
  setsockopt (cfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof (val));
}
