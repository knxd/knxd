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

#include "npdu.h"

NPDUPtr NPDU::fromPacket (const CArray & c, TracePtr tr)
{
}

/* N_Data_Individual */

bool
N_Data_Individual_PDU::init (const CArray & c)
{
  if (c.size() < 6)
    return false;

//  if (!lpdu.init (c))
//    return false;
  hop_count_type = (c[5] >> 4) & 0x07;
  return true;
}

CArray N_Data_Individual_PDU::ToPacket () const
{
  CArray pdu;
//  pdu = lpdu.ToPacket();
  pdu[5] = (pdu[5] & 0x8f) | ((hop_count_type & 0x07) << 4);
  return pdu;
}

std::string N_Data_Individual_PDU::Decode (TracePtr) const
{
  return "N_Data_Individual";
}

/* N_Data_Group */

bool
N_Data_Group_PDU::init (const CArray & c)
{
  if (c.size() < 6)
    return false;

//  if (!lpdu.init (c))
//    return false;
  hop_count_type = (c[5] >> 4) & 0x07;
  return true;
}

CArray N_Data_Group_PDU::ToPacket () const
{
  CArray pdu;
//  pdu = lpdu.ToPacket();
  pdu[5] = (pdu[5] & 0x8f) | ((hop_count_type & 0x07) << 4);
  return pdu;
}

std::string N_Data_Group_PDU::Decode (TracePtr) const
{
  return "N_Data_Group";
}

/* N_Data_Broadcast */

bool
N_Data_Broadcast_PDU::init (const CArray & c)
{
  if (c.size() < 6)
    return false;

//  if (!lpdu.init (c))
//    return false;
  hop_count_type = (c[5] >> 4) & 0x07;
  return true;
}

CArray N_Data_Broadcast_PDU::ToPacket () const
{
  CArray pdu;
//  pdu = lpdu.ToPacket();
  pdu[5] = (pdu[5] & 0x8f) | ((hop_count_type & 0x07) << 4);
  return pdu;
}

std::string N_Data_Broadcast_PDU::Decode (TracePtr) const
{
  return "N_Data_Broadcast";
}

/* N_Data_SystemBroadcast */

bool
N_Data_SystemBroadcast_PDU::init (const CArray & c)
{
  if (c.size() < 6)
    return false;

//  if (!lpdu.init (c))
//    return false;
  hop_count_type = (c[5] >> 4) & 0x07;
  return true;
}

CArray N_Data_SystemBroadcast_PDU::ToPacket () const
{
  CArray pdu;
//  pdu = lpdu.ToPacket();
  pdu[5] = (pdu[5] & 0x8f) | ((hop_count_type & 0x07) << 4);
  return pdu;
}

std::string N_Data_SystemBroadcast_PDU::Decode (TracePtr) const
{
  return "N_Data_SystemBroadcast";
}
