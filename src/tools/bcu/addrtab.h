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

#ifndef ADDR_TAB_H
#define ADDR_TAB_H

#include "lowlevel.h"

int readAddrTabSize (LowLevelDriver * iface, uint8_t & result);
int writeAddrTabSize (LowLevelDriver * iface, uint8_t size);

int readEMIMem (LowLevelDriver * iface, memaddr_t addr, uint8_t len,
                CArray & result);
int writeEMIMem (LowLevelDriver * iface, memaddr_t addr,
                 CArray data);

#endif
