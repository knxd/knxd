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

#include "config.h"

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

#include "ipsupport.h"
#include "lltcp.h"

LLtcp::~LLtcp () {}

bool
LLtcp::setup()
{
  if(!FDdriver::setup())
    return false;

  dest = cfg->value("ip-address","");
  port = cfg->value("dest-port",0);
  if (dest.size() == 0)
    {
      ERRORPRINTF (t, E_ERROR | 52, "%s: 'ip-address=<host>' required", cfg->name);
      return false;
    }
  if (port == 0)
    {
      ERRORPRINTF (t, E_ERROR | 52, "%s: 'port=<num>' required", cfg->name);
      return false;
    }

  return true;
}

void
LLtcp::start()
{
  int reuse = 1;
  int nodelay = 1;
  struct sockaddr_in addr;

  if (!GetHostIP (t, &addr, dest.c_str()))
    {
      ERRORPRINTF (t, E_ERROR | 52, "Lookup of %s failed: %s", dest, strerror(errno));
      goto ex1;
    }
  addr.sin_port = htons (port);

  fd = socket (AF_INET, SOCK_STREAM, 0);
  if (fd == -1)
    {
      ERRORPRINTF (t, E_ERROR | 52, "Opening %s:%d failed: %s", dest,port, strerror(errno));
      goto ex1;
    }

  if (connect (fd, (struct sockaddr *) &addr, sizeof (addr)) == -1)
    {
      ERRORPRINTF (t, E_ERROR | 53, "Connect %s:%d: connect: %s", dest,port, strerror(errno));
      goto ex2;
    }
  setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse));
  setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof (nodelay));

  TRACEPRINTF (t, 2, "Opened");
  FDdriver::start();
  return;

ex2:
  close (fd);
  fd = -1;
ex1:
  stopped();
}

