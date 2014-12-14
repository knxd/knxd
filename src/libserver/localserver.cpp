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

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "localserver.h"

LocalServer::LocalServer (Layer3 * la3, Trace * tr, const char *path):
Server (la3, tr)
{
  struct sockaddr_un addr;
  TRACEPRINTF (tr, 8, this, "OpenLocalSocket");
  addr.sun_family = AF_LOCAL;
  strncpy (addr.sun_path, path, sizeof (addr.sun_path));

  fd = socket (AF_LOCAL, SOCK_STREAM, 0);
  if (fd == -1)
    return;

  unlink (path);
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

  TRACEPRINTF (tr, 8, this, "LocalSocket opened");
  Start ();
}

bool
LocalServer::init ()
{
  return fd != -1;
}
