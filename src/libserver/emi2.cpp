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

#include "emi2.h"
#include "emi.h"

EMI2Driver::~EMI2Driver()
{
}

void
EMI2Driver::cmdEnterMonitor ()
{
  const uchar t1[] = { 0xa9, 0x1E, 0x12, 0x34, 0x56, 0x78, 0x9a };
  const uchar t2[] = { 0xa9, 0x90, 0x18, 0x34, 0x45, 0x67, 0x8a };
  iface->Send_Packet (CArray (t1, sizeof (t1)));
  iface->Send_Packet (CArray (t2, sizeof (t2)));
}

void
EMI2Driver::cmdLeaveMonitor ()
{
  uchar t[] = { 0xa9, 0x1E, 0x12, 0x34, 0x56, 0x78, 0x9a };
  iface->Send_Packet (CArray (t, sizeof (t)));
}

void
EMI2Driver::cmdOpen ()
{
  const uchar t1[] = { 0xa9, 0x1E, 0x12, 0x34, 0x56, 0x78, 0x9a };
  const uchar t2[] = { 0xa9, 0x00, 0x18, 0x34, 0x56, 0x78, 0x0a };
  iface->Send_Packet (CArray (t1, sizeof (t1)));
  iface->Send_Packet (CArray (t2, sizeof (t2)));
}

void
EMI2Driver::cmdClose ()
{
  uchar t[] = { 0xa9, 0x1E, 0x12, 0x34, 0x56, 0x78, 0x9a };
  iface->Send_Packet (CArray (t, sizeof (t)));
}

const uint8_t *
EMI2Driver::getIndTypes()
{
    static const uint8_t indTypes[] = { 0x2E, 0x29, 0x2B };
    return indTypes;
}   

