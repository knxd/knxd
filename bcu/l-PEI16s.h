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

#ifndef C_PEI16s_H
#define C_PEI16s_H

#include "bcu1serial.h"

#define PEI16s_URL "bcu1s:/dev/ttySx\n"
#define PEI16s_DOC "bcu1s connects using the PEI16 Protocoll over a BCU to the bus (using a experimental user mode driver, tracelevel % 2 must be 1 and the output must be displayed in a fast enough terminal)\n\n"

#define PEI16s_PREFIX "bcu1s"
#define PEI16s_CREATE PEI16s_ll_Create
#define PEI16s_CLEANUP NULL

inline LowLevelDriverInterface *
PEI16s_ll_Create (const char *dev, Trace * t)
{
  return new BCU1SerialLowLevelDriver (dev, t);
}

#endif
