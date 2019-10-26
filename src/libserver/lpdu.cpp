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

std::string L_Data_PDU::Decode (TracePtr tr) const
{
  assert (lsdu.size() >= 1);
  assert (lsdu.size() <= 0xff);
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
  TPDUPtr d = TPDU::fromPacket (address_type, destination_address, lsdu, tr);
  s += d->Decode (tr);
  return s;
}

/* L_SystemBroadcast */

std::string L_SystemBroadcast_PDU::Decode (TracePtr) const
{
  std::string s ("L_SystemBroadcast");
  // @todo
  return s;
}

/* L_Poll_Data */

std::string L_Poll_Data_PDU::Decode (TracePtr) const
{
  std::string s ("L_Poll_Data");
  // @todo
  return s;
}

/* L_Poll_Update */

std::string L_Poll_Update_PDU::Decode (TracePtr) const
{
  std::string s ("L_Poll_Update");
  // @todo
  return s;
}

/* L_Busmon */

L_Busmon_PDU::L_Busmon_PDU () : LPDU()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  time_stamp = tv.tv_sec*65536 + tv.tv_usec/(1000000/65536+1);
  l_status = 0;
}

std::string
L_Busmon_PDU::Decode (TracePtr tr) const
{
  std::string s ("L_Busmon: ");

  if (lpdu.size() == 0)
    return "empty LPDU";

  C_ITER (i,lpdu)
  addHex (s, *i);
  s += ":";
  LDataPtr l = CM_TP1_to_L_Data (lpdu, tr);
  s += l->Decode (tr);
  return s;
}

/* L_Service_Information */

std::string L_Service_Information_PDU::Decode (TracePtr) const
{
  std::string s ("L_Service_Information");
  // @todo
  return s;
}

/* L_Management */

std::string L_Management_PDU::Decode (TracePtr) const
{
  std::string s ("L_Management");
  // @todo
  return s;
}
