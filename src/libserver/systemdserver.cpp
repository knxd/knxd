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
#include <unistd.h>
#include <stdio.h>
#include <systemd/sd-daemon.h>
#include "systemdserver.h"

SystemdServer::SystemdServer (Layer3 * la3, Trace * tr):
Server (la3, tr)
{
  const int num_fds = sd_listen_fds(0);

  TRACEPRINTF (tr, 8, this, "OpenSystemdSocket");

  if( num_fds < 0 ) {
      fprintf(stderr, "Error: getting fds from systemd.");
      return;
  }
  else if( num_fds == 0 ) {
      fprintf(stderr, "Error: no sockets specified");
      return;
  }
  else if( num_fds > 1 )
      fprintf(stderr, "Warning: too many sockets specified, only using first.");

  fd = SD_LISTEN_FDS_START;
  if (listen (fd, 10) == -1)
    {
      close (fd);
      fd = -1;
      return;
    }

  TRACEPRINTF (tr, 8, this, "SystemdSocket opened");
  Start ();
}

bool
SystemdServer::init ()
{
  return fd != -1;
}
