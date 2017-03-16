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

#include <stdio.h>
#include "lpdu.h"
#include "tpdu.h"

LPDUPtr
LPDU::fromPacket (const CArray & c, TracePtr t UNUSED)
{
  LPDUPtr l = nullptr;
  if (c.size() >= 1)
    {
      if (c[0] == 0xCC)
	l = LPDUPtr(new L_ACK_PDU ());
      else if (c[0] == 0xC0)
	l = LPDUPtr(new L_BUSY_PDU ());
      else if (c[0] == 0x0C)
	l = LPDUPtr(new L_NACK_PDU ());
      else if ((c[0] & 0x53) == 0x10)
	l = LPDUPtr(new L_Data_PDU ());
    }
  if (l && l->init (c))
    return l;
  l = LPDUPtr(new L_Unknown_PDU ());
  l->init (c);
  return l;
}

/* L_NACK */

L_NACK_PDU::L_NACK_PDU () : LPDU()
{
}

bool
L_NACK_PDU::init (const CArray & c)
{
  if (c.size() != 1)
    return false;
  return true;
}

CArray L_NACK_PDU::ToPacket ()
{
  uchar
    c = 0x0C;
  return CArray (&c, 1);
}

String L_NACK_PDU::Decode (TracePtr t UNUSED)
{
  return "NACK";
}

/* L_ACK */

L_ACK_PDU::L_ACK_PDU () : LPDU()
{
}

bool
L_ACK_PDU::init (const CArray & c)
{
  if (c.size() != 1)
    return false;
  return true;
}

CArray L_ACK_PDU::ToPacket ()
{
  uchar c = 0xCC;
  return CArray (&c, 1);
}

String L_ACK_PDU::Decode (TracePtr t UNUSED)
{
  return "ACK";
}

/* L_BUSY */

L_BUSY_PDU::L_BUSY_PDU () : LPDU()
{
}

bool
L_BUSY_PDU::init (const CArray & c)
{
  if (c.size() != 1)
    return false;
  return true;
}

CArray L_BUSY_PDU::ToPacket ()
{
  uchar c = 0xC0;
  return CArray (&c, 1);
}

String L_BUSY_PDU::Decode (TracePtr t UNUSED)
{
  return "BUSY";
}

/* L_Unknown  */

L_Unknown_PDU::L_Unknown_PDU () : LPDU()
{
}

bool
L_Unknown_PDU::init (const CArray & c)
{
  pdu = c;
  return true;
}

CArray
L_Unknown_PDU::ToPacket ()
{
  return pdu;
}

String
L_Unknown_PDU::Decode (TracePtr t UNUSED)
{
  String s ("Unknown LPDU: ");

  if (pdu.size() == 0)
    return "empty LPDU";

  ITER (i,pdu)
    addHex (s, *i);

  return s;
}

/* L_Busmonitor  */

L_Busmonitor_PDU::L_Busmonitor_PDU () : LPDU()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  timestamp = tv.tv_sec*65536 + tv.tv_usec/(1000000/65536+1);
  status = 0;
}

bool
L_Busmonitor_PDU::init (const CArray & c)
{
  pdu = c;
  return true;
}

CArray
L_Busmonitor_PDU::ToPacket ()
{
  return pdu;
}

String
L_Busmonitor_PDU::Decode (TracePtr t)
{
  String s ("LPDU: ");

  if (pdu.size() == 0)
    return "empty LPDU";

  ITER (i,pdu)
    addHex (s, *i);
  s += ":";
  LPDUPtr l = LPDU::fromPacket (pdu, t);
  s += l->Decode (t);
  return s;
}

/* L_Data */

L_Data_PDU::L_Data_PDU () : LPDU()
{
  prio = PRIO_LOW;
  repeated = 0;
  valid_checksum = 1;
  valid_length = 1;
  AddrType = IndividualAddress;
  source = 0;
  dest = 0;
  hopcount = 0x06;
}

bool
L_Data_PDU::init (const CArray & c)
{
  unsigned len, i;
  uchar c1;
  if (c.size() < 6)
    return false;
  if ((c[0] & 0x53) != 0x10)
    return false;
  repeated = (c[0] & 0x20) ? 0 : 1;
  valid_length = 1;
  switch ((c[0] >> 2) & 0x3)
    {
    case 0:
      prio = PRIO_SYSTEM;
      break;
    case 1:
      prio = PRIO_URGENT;
      break;
    case 2:
      prio = PRIO_NORMAL;
      break;
    case 3:
      prio = PRIO_LOW;
      break;
    }
  if (c[0] & 0x80)
    {
      /*Standard frame */
      source = (c[1] << 8) | (c[2]);
      dest = (c[3] << 8) | (c[4]);
      len = (c[5] & 0x0f) + 1;
      hopcount = (c[5] >> 4) & 0x07;
      AddrType = (c[5] & 0x80) ? GroupAddress : IndividualAddress;
      if (len + 7 != c.size())
	return false;
      data.set (c.data() + 6, len);
    }
  else
    {
      /*extended frame */
      if ((c[1] & 0x0f) != 0)
	return false;
      if (c.size() < 7)
	return false;
      hopcount = (c[1] >> 4) & 0x07;
      AddrType = (c[1] & 0x80) ? GroupAddress : IndividualAddress;
      source = (c[2] << 8) | (c[3]);
      dest = (c[4] << 8) | (c[5]);
      len = c[6] + 1;
      if (len + 8 != c.size())
	{
	  if (c.size() == 23)
	    {
	      valid_length = 0;
	      data.set (c.data() + 7, 8);
	    }
	  else
	    return false;
	}
      else
	data.set (c.data() + 7, len);
    }

  c1 = 0;
  for (i = 0; i < c.size () - 1; i++)
    c1 ^= c[i];
  c1 = ~c1;
  valid_checksum = (c[c.size () - 1] == c1);

  return true;
}

CArray L_Data_PDU::ToPacket ()
{
  assert (data.size() >= 1);
  assert (data.size() <= 0xff);
  assert ((hopcount & 0xf8) == 0);
  CArray pdu;
  uchar c = 0; // stupid compiler
  unsigned i;
  switch (prio)
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
  if (data.size() - 1 <= 0x0f)
    {
      pdu.resize (7 + data.size());
      pdu[0] = 0x90 | (c << 2) | (repeated ? 0x00 : 0x20);
      pdu[1] = (source >> 8) & 0xff;
      pdu[2] = (source) & 0xff;
      pdu[3] = (dest >> 8) & 0xff;
      pdu[4] = (dest) & 0xff;
      pdu[5] =
	(hopcount & 0x07) << 4 | ((data.size() - 1) & 0x0f) | (AddrType ==
							   GroupAddress ? 0x80
							   : 0x00);
      pdu.setpart (data.data(), 6, 1 + ((data.size() - 1) & 0x0f));
    }
  else
    {
      pdu.resize (8 + data.size());
      pdu[0] = 0x10 | (c << 2) | (repeated ? 0x00 : 0x20);
      pdu[1] =
	(hopcount & 0x07) << 4 | (AddrType == GroupAddress ? 0x80 : 0x00);
      pdu[2] = (source >> 8) & 0xff;
      pdu[3] = (source) & 0xff;
      pdu[4] = (dest >> 8) & 0xff;
      pdu[5] = (dest) & 0xff;
      pdu[6] = (data.size() - 1) & 0xff;
      pdu.setpart (data.data(), 7, 1 + ((data.size() - 1) & 0xff));
    }
  /* checksum */
  c = 0;
  for (i = 0; i < pdu.size() - 1; i++)
    c ^= pdu[i];
  pdu[pdu.size() - 1] = ~c;

  return pdu;
}

String L_Data_PDU::Decode (TracePtr t)
{
  assert (data.size() >= 1);
  assert (data.size() <= 0xff);
  assert ((hopcount & 0xf8) == 0);

  String s ("L_Data");
  if (!valid_length)
    s += " (incomplete)";
  if (repeated)
    s += " (repeated)";
  switch (prio)
    {
    case PRIO_LOW:
      s += " low";
      break;
    case PRIO_NORMAL:
      s += " normal";
      break;
    case PRIO_URGENT:
      s += " urgent";
      break;
    case PRIO_SYSTEM:
      s += " system";
      break;
    }
  if (!valid_checksum)
    s += " INVALID CHECKSUM";
  s += " from ";
  s += FormatEIBAddr (source);
  s += " to ";
  s += (AddrType == GroupAddress ?
                    FormatGroupAddr (dest) :
                    FormatEIBAddr (dest));
  s += " hops: ";
  addHex (s, hopcount);
  TPDUPtr d = TPDU::fromPacket (data, t);
  s += d->Decode (t);
  return s;
}
