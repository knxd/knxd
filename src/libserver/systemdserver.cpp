/*
    EIBD eib bus access and management daemon
    Copyright (C) 2015 Marc Joliet <marcec@gmx.de>

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
#include <errno.h>
#include "systemdserver.h"

SystemdServer::SystemdServer (Trace * tr, int systemd_fd):
Server (tr)
{
  TRACEPRINTF (tr, 8, this, "OpenSystemdSocket");

  fd = systemd_fd;
  if (listen (fd, 10) == -1)
    {
      ERRORPRINTF (tr, E_ERROR | 18, this, "OpenSystemdSocket: listen: %s", strerror(errno));
      close (fd);
      fd = -1;
      return;
    }

  TRACEPRINTF (tr, 8, this, "SystemdSocket opened");
  Start ();
}

