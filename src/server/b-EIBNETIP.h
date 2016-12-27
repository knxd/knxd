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

#ifndef C_EIBNETIP_H
#define C_EIBNETIP_H

#include <stdlib.h>
#include "eibnetrouter.h"

#define EIBNETIP_URL "ip:[multicast_addr[:port]]\n"
#define EIBNETIP_DOC "ip uses multicast to talk to an EIBnet/IP gateway. The gateway must be configured to route your addresses to multicast.\n\n"

#define EIBNETIP_PREFIX "ip"
#define EIBNETIP_CREATE eibnetip_Create

inline Layer2Ptr
eibnetip_Create (const char *dev, L2options *opt)
{
  if (!*dev)
    return std::shared_ptr<EIBNetIPRouter>(new EIBNetIPRouter ("224.0.23.12", 3671, opt));
  char *a = strdup (dev);
  char *b;
  int port;
  Layer2Ptr c;
  for (b = a; *b; b++)
    if (*b == ':')
      break;
  if (*b == ':')
    {
      *b = 0;
      port = atoi (b + 1);
    }
  else
    port = 3671;
  c = std::shared_ptr<EIBNetIPRouter>(new EIBNetIPRouter (a, port, opt));
  free (a);
  return c;
}

#endif
