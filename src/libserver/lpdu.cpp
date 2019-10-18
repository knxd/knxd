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

#include <cstdio>
#include "lpdu.h"
#include "tpdu.h"

LPDUPtr
LPDU::fromPacket (const CArray & c, TracePtr)
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

std::string L_NACK_PDU::Decode (TracePtr)
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

std::string L_ACK_PDU::Decode (TracePtr)
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

std::string L_BUSY_PDU::Decode (TracePtr)
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

std::string
L_Unknown_PDU::Decode (TracePtr)
{
  std::string s ("Unknown LPDU: ");

  if (pdu.size() == 0)
    return "empty LPDU";

  ITER (i,pdu)
  addHex (s, *i);

  return s;
}

/* L_Data */

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
  priority = static_cast<EIB_Priority>((c[0] >> 2) & 0x3);
  if (c[0] & 0x80)
    {
      /* Standard frame */
      source_address = (c[1] << 8) | (c[2]);
      destination_address = (c[3] << 8) | (c[4]);
      len = (c[5] & 0x0f) + 1;
      hop_count = (c[5] >> 4) & 0x07;
      address_type = (c[5] & 0x80) ? GroupAddress : IndividualAddress;
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
      hop_count = (c[1] >> 4) & 0x07;
      address_type = (c[1] & 0x80) ? GroupAddress : IndividualAddress;
      source_address = (c[2] << 8) | (c[3]);
      destination_address = (c[4] << 8) | (c[5]);
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
  assert ((hop_count & 0xf8) == 0);
  CArray pdu;
  if (data.size() - 1 <= 0x0f)
    {
      pdu.resize (7 + data.size());
      pdu[0] = 0x90 | (priority << 2) | (repeated ? 0x00 : 0x20);
      pdu[1] = (source_address >> 8) & 0xff;
      pdu[2] = (source_address) & 0xff;
      pdu[3] = (destination_address >> 8) & 0xff;
      pdu[4] = (destination_address) & 0xff;
      pdu[5] =
        (hop_count & 0x07) << 4 |
        ((data.size() - 1) & 0x0f) |
        (address_type == GroupAddress ? 0x80 : 0x00);
      pdu.setpart (data.data(), 6, 1 + ((data.size() - 1) & 0x0f));
    }
  else
    {
      pdu.resize (8 + data.size());
      pdu[0] = 0x10 | (priority << 2) | (repeated ? 0x00 : 0x20);
      pdu[1] =
        (hop_count & 0x07) << 4 | (address_type == GroupAddress ? 0x80 : 0x00);
      pdu[2] = source_address >> 8;
      pdu[3] = source_address & 0xff;
      pdu[4] = destination_address >> 8;
      pdu[5] = destination_address & 0xff;
      pdu[6] = (data.size() - 1) & 0xff;
      pdu.setpart (data.data(), 7, 1 + ((data.size() - 1) & 0xff));
    }
  /* checksum */
  uchar c = 0;
  for (unsigned i = 0; i < pdu.size() - 1; i++)
    c ^= pdu[i];
  pdu[pdu.size() - 1] = ~c;

  return pdu;
}

std::string L_Data_PDU::Decode (TracePtr t)
{
  assert (data.size() >= 1);
  assert (data.size() <= 0xff);
  assert ((hop_count & 0xf8) == 0);

  std::string s ("L_Data");
  if (!valid_length)
    s += " (incomplete)";
  if (repeated)
    s += " (repeated)";
  switch (priority)
    {
    case PRIO_SYSTEM:
      s += " system";
      break;
    case PRIO_URGENT:
      s += " urgent";
      break;
    case PRIO_NORMAL:
      s += " normal";
      break;
    case PRIO_LOW:
      s += " low";
      break;
    }
  if (!valid_checksum)
    s += " INVALID CHECKSUM";
  s += " from ";
  s += FormatEIBAddr (source_address);
  s += " to ";
  s += (address_type == GroupAddress ?
        FormatGroupAddr (destination_address) :
        FormatEIBAddr (destination_address));
  s += " hops: ";
  addHex (s, hop_count);
  TPDUPtr d = TPDU::fromPacket (address_type, destination_address, data, t);
  s += d->Decode (t);
  return s;
}

/* L_Busmon */

L_Busmon_PDU::L_Busmon_PDU () : LPDU()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  time_stamp = tv.tv_sec*65536 + tv.tv_usec/(1000000/65536+1);
  status = 0;
}

bool
L_Busmon_PDU::init (const CArray & c)
{
  pdu = c;
  return true;
}

CArray
L_Busmon_PDU::ToPacket ()
{
  return pdu;
}

std::string
L_Busmon_PDU::Decode (TracePtr t)
{
  std::string s ("LPDU: ");

  if (pdu.size() == 0)
    return "empty LPDU";

  ITER (i,pdu)
  addHex (s, *i);
  s += ":";
  LPDUPtr l = LPDU::fromPacket (pdu, t);
  s += l->Decode (t);
  return s;
}
