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

#ifndef C_TERA_H
#define C_TERA_H

#include "teraserial.h"

#define TERA_URL "tera:/dev/ttySx\n"

#define TERA_DOC "tera connects to the EIB bus over an the SystemTera TPUART\n\n"

#define TERA_PREFIX "tera"

#define TERA_CREATE tera_Create

#define TERA_CLEANUP NULL

inline Layer2Interface *
tera_Create (const char *dev, int flags, Trace * t)
{
  return new TERASerialLayer2Driver(dev, arg.addr, flags, t);
}

#endif
