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

#include "emi1.h"
#include "emi.h"
#include "layer3.h"

EMI1Layer2::~EMI1Layer2()
{
}

void
EMI1Layer2::cmdEnterMonitor()
{
  const uchar t[] = { 0x46, 0x01, 0x00, 0x60, 0x90 };
  // pth_usleep (1000000);
  iface->Send_Packet (CArray (t, sizeof (t)));
}

void
EMI1Layer2::cmdLeaveMonitor()
{
  uchar t[] = { 0x46, 0x01, 0x00, 0x60, 0xc0 };
  iface->Send_Packet (CArray (t, sizeof (t)));
  // pth_usleep (1000000);
}

void
EMI1Layer2::cmdOpen ()
{
  const uchar ta[] = { 0x46, 0x01, 0x01, 0x16, 0x00 }; // clear addr tab
  const uchar t[] = { 0x46, 0x01, 0x00, 0x60, 0x12 };
  iface->Send_Packet (CArray (ta, sizeof (t)));
  iface->Send_Packet (CArray (t, sizeof (t)));
}

void
EMI1Layer2::cmdClose ()
{
  uchar t[] = { 0x46, 0x01, 0x00, 0x60, 0xc0 };
  iface->Send_Packet (CArray (t, sizeof (t)));
}

const uint8_t *
EMI1Layer2::getIndTypes()
{
    static const uint8_t indTypes[] = { 0x4E, 0x49, 0x49 };
    return indTypes;
}
