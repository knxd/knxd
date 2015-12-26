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

#ifndef C_EIBNETIPTUNNEL_H
#define C_EIBNETIPTUNNEL_H

#include <stdlib.h>
#include "eibnettunnel.h"

#define EIBNETIPTUNNEL_URL "ipt:router-ip[:dest-port[:src-port[:nat-ip[:data-port]]]]]\n"
#define EIBNETIPTUNNEL_DOC "ipt directly connects to an EIBnet/IP gateway. The gateway must be configured to route the necessary addresses\n\n"

#define EIBNETIPTUNNEL_PREFIX "ipt"
#define EIBNETIPTUNNEL_CREATE eibnetiptunnel_Create

#define EIBNETIPTUNNELNAT_URL "iptn:router-ip[:dest-port[:src-port]]\n"
#define EIBNETIPTUNNELNAT_DOC "iptn connects to an EIBnet/IP gateway using NAT mode\n\n"

#define EIBNETIPTUNNELNAT_PREFIX "iptn"
#define EIBNETIPTUNNELNAT_CREATE eibnetiptunnelnat_Create


inline Layer2Ptr
eibnetiptunnel_Create (const char *dev, L2options *opt, Layer3 * l3)
{
  char *a = strdup (dev);
  char *b;
  char *c;
  char *d = 0;
  char *e;
  int dport = 3671;
  int dataport = -1;
  int sport = 3672;;
  Layer2Ptr iface;

  if (!a)
    die ("out of memory");
  c = d = e = NULL;
  b = strchr(a,':');
  if (b) {
    *b++ = 0;
    c = strchr(b,':');
    if (c) {
      *c++ = 0;
      d = strchr(c,':');
      if (d) {
        *d++ = 0;
        e = strchr(d,':');
        if (e)
          *e++ = 0;
      }
    }
  }
  if (b && *b)
    dport = atoi(b);
  if (c && *c)
    sport = atoi(c);
  if (e && *e)
    dataport = atoi(e);

  iface = std::shared_ptr<EIBNetIPTunnel>(new EIBNetIPTunnel (a, dport, sport, d, dataport, opt, l3));
  free (a);
  return iface;
}

inline Layer2Ptr
eibnetiptunnelnat_Create (const char *dev, L2options *opt, Layer3 * l3)
{
  char *a = strdup (dev);
  char *b;
  char *c;
  int dport = 3671;
  int sport = 3672;
  Layer2Ptr iface;
  if (!a)
    die ("out of memory");
  c = NULL;
  b = strchr(a,':');
  if (b) {
    *b++ = 0;
    c = strchr(b,':');
    if (c)
      *c++ = 0;
  }
  if (b && *b)
    dport = atoi(b);
  if (c && *c)
    sport = atoi(c);

  iface = std::shared_ptr<EIBNetIPTunnel>(new EIBNetIPTunnel (a, dport, sport, "0.0.0.0", -1, opt, l3));
  free (a);
  return iface;
}


#endif
