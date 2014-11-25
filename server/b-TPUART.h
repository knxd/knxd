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

#ifndef C_TPUART_H
#define C_TPUART_H

#include "tpuart.h"

#define TPUART24_URL "tpuart24:/dev/tpuartX\n"
#define TPUART26_URL "tpuart:/dev/tpuartX\n"

#define TPUART24_DOC "tpuart24 connects to the EIB bus over an TPUART (using the TPUART driver, Linux Kernel 2.4)\n\n"
#define TPUART26_DOC "tpuart connects to the EIB bus over an TPUART (using the TPUART driver, Linux Kernel 2.6)\n\n"

#define TPUART24_PREFIX "tpuart24"
#define TPUART26_PREFIX "tpuart"

#define TPUART24_CREATE tpuart24_Create
#define TPUART26_CREATE tpuart26_Create

#define TPUART24_CLEANUP NULL
#define TPUART26_CLEANUP NULL

inline Layer2Interface *
tpuart24_Create (const char *dev, int flags, Trace * t)
{
  return new TPUARTLayer2Driver (0, dev, arg.addr, t);
}

inline Layer2Interface *
tpuart26_Create (const char *dev, int flags, Trace * t)
{
  return new TPUARTLayer2Driver (1, dev, arg.addr, t);
}

#endif
