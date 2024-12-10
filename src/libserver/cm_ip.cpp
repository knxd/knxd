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

#include "cm_ip.h"
#include "config.h"

#include <net/if.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

CArray
IPtoEIBNetIP (const struct sockaddr_in * a, bool nat, uint8_t protocol)
{
  CArray buf;
  buf.resize (8);
  buf[0] = 0x08;
  buf[1] = protocol;
  if (nat)
    {
      buf[2] = 0;
      buf[3] = 0;
      buf[4] = 0;
      buf[5] = 0;
      buf[6] = 0;
      buf[7] = 0;
    }
  else
    {
      buf[2] = (ntohl (a->sin_addr.s_addr) >> 24) & 0xff;
      buf[3] = (ntohl (a->sin_addr.s_addr) >> 16) & 0xff;
      buf[4] = (ntohl (a->sin_addr.s_addr) >> 8) & 0xff;
      buf[5] = (ntohl (a->sin_addr.s_addr) >> 0) & 0xff;
      buf[6] = (ntohs (a->sin_port) >> 8) & 0xff;
      buf[7] = (ntohs (a->sin_port) >> 0) & 0xff;
    }
  return buf;
}

bool
EIBnettoIP (const CArray & buf, struct sockaddr_in *a,
            const struct sockaddr_in *src, bool & nat, uint8_t protocol)
{
  int ip, port;
  memset (a, 0, sizeof (*a));
  if (buf[0] != 0x8 || buf[1] != protocol)
    return true;
  ip = (buf[2] << 24) | (buf[3] << 16) | (buf[4] << 8) | (buf[5]);
  port = (buf[6] << 8) | (buf[7]);
#ifdef HAVE_SOCKADDR_IN_LEN
  a->sin_len = sizeof (*a);
#endif
  a->sin_family = AF_INET;
  if (port == 0)
    a->sin_port = src->sin_port;
  else
    a->sin_port = htons (port);
  if (ip == 0)
    {
      nat = true;
      a->sin_addr.s_addr = src->sin_addr.s_addr;
    }
  else
    a->sin_addr.s_addr = htonl (ip);

  return false;
}
