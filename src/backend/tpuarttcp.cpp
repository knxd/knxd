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
#include <errno.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include "tpuarttcp.h"
#include "layer3.h"

TPUARTTCPLayer2Driver::TPUARTTCPLayer2Driver (const char *dest, int port,
						    L2options *opt)
	: TPUART_Base(opt)
{
  int reuse = 1;
  int nodelay = 1;
  struct sockaddr_in addr;

  TRACEPRINTF (t, 2, this, "Open");

  if (!GetHostIP (t, &addr, dest))
    return;
  addr.sin_port = htons (port);

  fd = socket (AF_INET, SOCK_STREAM, 0);
  if (fd == -1)
    {
      ERRORPRINTF (t, E_ERROR | 52, this, "Opening %s:%d failed: %s", dest,port, strerror(errno));
      return;
    }
  setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse));

  if (connect (fd, (struct sockaddr *) &addr, sizeof (addr)) == -1)
    {
      ERRORPRINTF (t, E_ERROR | 53, this, "Connect %s:%d: connect: %s", dest,port, strerror(errno));
      close (fd);
      fd = -1;
      return;
    }
  setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof (nodelay));

  TRACEPRINTF (t, 2, this, "Openend");
}

bool
TPUARTTCPLayer2Driver::init(Layer3 *l3)
{
  if (!TPUART_Base::init(l3))
    return false;
  setup_buffers();
  return true;
}

void
TPUARTTCPLayer2Driver::setstate(enum TSTATE new_state)
{
  if (new_state == T_dev_start)
    new_state = T_is_online;

  TPUART_Base::setstate(new_state);
}

void
TPUARTTCPLayer2Driver::dev_timer()
{
  ERRORPRINTF (t, E_ERROR | 61, this, "bad timeout in state %d",state);
}
