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

#ifndef C_TPUARTs_TCP_H
#define C_TPUARTs_TCP_H

#include "tpuarttcp.h"

#define TPUARTs_URL "tpuarttcp:CUNX_IP_ADDR:2324\n"

#define TPUARTs_DOC "tpuarttcp connects to the EIB bus over a TPUART (using a TCP connection)\n\n"

#define TPUARTs_PREFIX "tpuarttcp"

#define TPUARTs_CREATE tpuarts_tcp_Create

inline Layer2Ptr 
tpuarts_tcp_Create (const char *dev, L2options *opt)
{
  char *a = strdup (dev);
  char *b;
  int port;
  Layer2Ptr c;
  if (!a)
    die ("out of memory");
  for (b = a; *b; b++)
    if (*b == ':')
      break;
  if (*b == ':')
    {
      *b = 0;
      port = atoi (b + 1);
    }
  else
    port = 2324;
  c = std::shared_ptr<TPUARTTCPLayer2Driver>(new TPUARTTCPLayer2Driver (a, port, opt));
  free (a);
  return c;
}

#endif
