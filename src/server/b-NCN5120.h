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

#ifndef C_NCN5120_H
#define C_NCN5120_H

#include "ncn5120.h"

#define NCN5120_URL "ncn5120:/dev/ttySx\n"
#define NCN5120_DOC "NCN5120 connects to the EIB bus over an NCN5120 with 38400 baud and 8-bit mode\n\n"
#define NCN5120_PREFIX "ncn5120"
#define NCN5120_CREATE ncn5120_Create

inline Layer2Ptr 
ncn5120_Create (const char *dev, L2options *opt)
{
  return std::shared_ptr<NCN5120SerialLayer2Driver>(new NCN5120SerialLayer2Driver(dev, opt));
}

#endif
