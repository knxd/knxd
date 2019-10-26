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

#include "lpdu.h"

#include <cstdio>

#include "cm_tp1.h"
#include "tpdu.h"

/* L_Data */

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

std::string
L_Busmon_PDU::Decode (TracePtr t)
{
  std::string s ("L_Busmon: ");

  if (pdu.size() == 0)
    return "empty LPDU";

  ITER (i,pdu)
  addHex (s, *i);
  s += ":";
  LDataPtr l = CM_TP1_to_L_Data (pdu, t);
  s += l->Decode (t);
  return s;
}
