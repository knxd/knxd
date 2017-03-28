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

#ifndef KNXD_LIBSERVER_SYSTEMDSERVER_H
#define KNXD_LIBSERVER_SYSTEMDSERVER_H

#include "server.h"

/** implements a server listening on a systemd provided file descriptor */
class SystemdServer:public NetServer
{
public:
  SystemdServer (BaseRouter& r, IniSectionPtr& s, int systemd_fd);
  virtual ~SystemdServer ();

  void start();
  void stop();
};

#endif
