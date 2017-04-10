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
#include <sys/socket.h>
#include "systemdserver.h"

/*
 * systemd services are not controlled by the "usual" server logic,
 * so no SERVER macro here.
 */
SystemdServer::SystemdServer (BaseRouter& r, IniSectionPtr& s, int systemd_fd)
    : NetServer(r,s)
{
  t->setAuxName("systemd");
  fd = systemd_fd;
}

void
SystemdServer::start()
{
  TRACEPRINTF (t, 8, "OpenSystemdSocket %d", fd);
  if (fd < 0)
    {
      stopped();
      return;
    }

  if (listen (fd, 10) == -1)
    {
      ERRORPRINTF (t, E_ERROR | 19, "OpenSystemdSocket: listen: %s", strerror(errno));
      NetServer::stop();
      return;
    }

  TRACEPRINTF (t, 8, "SystemdSocket %d opened", fd);
  NetServer::start();
}

void
SystemdServer::stop()
{
//** actually, we can't, because we can't re-open. So fake it.
//  if (fd > -1)
//    {
//      close(fd);
//      fd = -1;
//    }
  NetServer::stop();
}

SystemdServer::~SystemdServer()
{
  if (fd >= 0)
    close(fd);
}

