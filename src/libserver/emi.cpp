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

#include "emi.h"

CArray
L_Data_ToCEMI (uchar code, const L_Data_PDU & l1)
{
  uchar c;
  CArray pdu;
  assert (l1.data () >= 1);
  assert (l1.data () < 0xff);
  assert ((l1.hopcount & 0xf8) == 0);

  switch (l1.prio)
    {
    case PRIO_LOW:
      c = 0x3;
      break;
    case PRIO_NORMAL:
      c = 0x1;
      break;
    case PRIO_URGENT:
      c = 0x02;
      break;
    case PRIO_SYSTEM:
      c = 0x00;
      break;
    }
  pdu.resize (l1.data () + 9);
  pdu[0] = code;
  pdu[1] = 0x00;
  pdu[2] = 0x10 | (c << 2) | (l1.data () - 1 <= 0x0f ? 0x80 : 0x00);
  if (code == 0x29)
    pdu[2] |= (l1.repeated ? 0 : 0x20);
  else
    pdu[2] |= 0x20;
  pdu[3] =
    (l1.AddrType ==
     GroupAddress ? 0x80 : 0x00) | ((l1.hopcount & 0x7) << 4) | 0x0;
  pdu[4] = (l1.source >> 8) & 0xff;
  pdu[5] = (l1.source) & 0xff;
  pdu[6] = (l1.dest >> 8) & 0xff;
  pdu[7] = (l1.dest) & 0xff;
  pdu[8] = l1.data () - 1;
  pdu.setpart (l1.data.array (), 9, l1.data ());
  return pdu;
}

L_Data_PDU *
CEMI_to_L_Data (const CArray & data)
{
  L_Data_PDU c;
  if (data () < 2)
    return 0;
  unsigned start = data[1] + 2;
  if (data () < 7 + start)
    return 0;
  if (data () < 7 + start + data[6 + start] + 1)
    return 0;
  c.source = (data[start + 2] << 8) | (data[start + 3]);
  c.dest = (data[start + 4] << 8) | (data[start + 5]);
  c.data.set (data.array () + start + 7, data[6 + start] + 1);
  if (data[0] == 0x29)
    c.repeated = (data[start] & 0x20) ? 0 : 1;
  else
    c.repeated = 0;
  switch ((data[start] >> 2) & 0x3)
    {
    case 0:
      c.prio = PRIO_SYSTEM;
      break;
    case 1:
      c.prio = PRIO_URGENT;
      break;
    case 2:
      c.prio = PRIO_NORMAL;
      break;
    case 3:
      c.prio = PRIO_LOW;
      break;
    }
  c.hopcount = (data[start + 1] >> 4) & 0x07;
  c.AddrType = (data[start + 1] & 0x80) ? GroupAddress : IndividualAddress;
  if (!data[start] & 0x80 && data[start + 1] & 0x0f)
    return 0;
  return new L_Data_PDU (c);
}

L_Busmonitor_PDU *
CEMI_to_Busmonitor (const CArray & data)
{
  L_Busmonitor_PDU c;
  if (data () < 2)
    return 0;
  unsigned start = data[1] + 2;
  if (data () < 1 + start)
    return 0;
  c.pdu.set (data.array () + start, data () - start);
  return new L_Busmonitor_PDU (c);
}

CArray
Busmonitor_to_CEMI (uchar code, const L_Busmonitor_PDU & p, int no)
{
  CArray pdu;
  pdu.resize (p.pdu () + 6);
  pdu[0] = code;
  pdu[1] = 4;
  pdu[2] = 3;
  pdu[3] = 1;
  pdu[4] = 1;
  pdu[5] = no & 0x7;
  pdu.setpart (p.pdu, 6);
  return pdu;
}

CArray
L_Data_ToEMI (uchar code, const L_Data_PDU & l1)
{
  CArray pdu;
  uchar c;
  switch (l1.prio)
    {
    case PRIO_LOW:
      c = 0x3;
      break;
    case PRIO_NORMAL:
      c = 0x1;
      break;
    case PRIO_URGENT:
      c = 0x02;
      break;
    case PRIO_SYSTEM:
      c = 0x00;
      break;
    }
  pdu.resize (l1.data () + 7);
  pdu[0] = code;
  pdu[1] = c << 2;
  pdu[2] = 0;
  pdu[3] = 0;
  pdu[4] = (l1.dest >> 8) & 0xff;
  pdu[5] = (l1.dest) & 0xff;
  pdu[6] =
    (l1.hopcount & 0x07) << 4 | ((l1.data () - 1) & 0x0f) | (l1.AddrType ==
							     GroupAddress ?
							     0x80 : 0x00);
  pdu.setpart (l1.data.array (), 7, l1.data ());
  return pdu;
}

L_Data_PDU *
EMI_to_L_Data (const CArray & data)
{
  L_Data_PDU c;
  unsigned len;

  if (data () < 8)
    return 0;

  c.source = (data[2] << 8) | (data[3]);
  c.dest = (data[4] << 8) | (data[5]);
  switch ((data[1] >> 2) & 0x3)
    {
    case 0:
      c.prio = PRIO_SYSTEM;
      break;
    case 1:
      c.prio = PRIO_URGENT;
      break;
    case 2:
      c.prio = PRIO_NORMAL;
      break;
    case 3:
      c.prio = PRIO_LOW;
      break;
    }
  c.AddrType = (data[6] & 0x80) ? GroupAddress : IndividualAddress;
  len = (data[6] & 0x0f) + 1;
  if (len > data.len () - 7)
    len = data.len () - 7;
  c.data.set (data.array () + 7, len);
  c.hopcount = (data[6] >> 4) & 0x07;
  return new L_Data_PDU (c);
}
