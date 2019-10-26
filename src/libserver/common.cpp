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

#include "common.h"

#include <cstdio>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

timestamp_t
getTime ()
{
  struct timeval t;
  gettimeofday (&t, 0);
  return ((timestamp_t) t.tv_sec) * 1000000 + ((timestamp_t) t.tv_usec);
}

std::string
FormatEIBAddr (eibaddr_t addr)
{
  char buf[255];
  sprintf (buf, "%d.%d.%d", (addr >> 12) & 0xf, (addr >> 8) & 0xf,
           (addr) & 0xff);
  return buf;
}

std::string
FormatGroupAddr (eibaddr_t addr)
{
  char buf[255];
  sprintf (buf, "%d/%d/%d", (addr >> 11) & 0x1f, (addr >> 8) & 0x7,
           (addr) & 0xff);
  return buf;
}

std::string
FormatDomainAddr (domainaddr_t addr)
{
  char buf[255];
  sprintf (buf, "%04X", addr);
  return buf;
}

std::string
FormatEIBKey (eibkey_type key)
{
  char buf[255];
  sprintf (buf, "%08X", key);
  return buf;
}

void
addHex (std::string & s, const uint8_t c)
{
  char buf[4];
  sprintf (buf, "%02X ", c);
  s += buf;
}

void
add16Hex (std::string & s, const uint16_t c)
{
  char buf[6];
  sprintf (buf, "%04X ", c);
  s += buf;
}

