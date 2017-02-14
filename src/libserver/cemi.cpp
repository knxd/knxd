/*
    EIBD eib bus access and management daemon
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>
 
    cEMI support for USB
    Copyright (C) 2013 Meik Felser <felser@cs.fau.de>

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

#include "cemi.h"
#include "emi.h"

unsigned int
CEMIDriver::maxPacketLen()
{
  return 50;
}

CEMIDriver::~CEMIDriver()
{
}

void CEMIDriver::cmdEnterMonitor() {}
void CEMIDriver::cmdLeaveMonitor() {}
void CEMIDriver::cmdOpen() {}
void CEMIDriver::cmdClose() {}

const uint8_t *
CEMIDriver::getIndTypes()
{ 
  static const uint8_t indTypes[] = { 0x2E, 0x29, 0x2B };
  return indTypes;
}   

