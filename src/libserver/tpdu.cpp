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

#include "tpdu.h"
#include "apdu.h"

TPDUPtr
TPDU::fromPacket (const EIB_AddrType address_type, const eibaddr_t destination_address, const CArray & c, TracePtr tr)
{
  TPDUPtr t;
  if (c.size() >= 1)
    {
      if (address_type == GroupAddress)
        {
          if ((c[0] & 0xFC) == 0x00)
            {
              if (destination_address == 0)
                t = TPDUPtr(new T_Data_Broadcast_PDU ()); // @todo T_Data_SystemBroadcast
              else
                t = TPDUPtr(new T_Data_Group_PDU ());
            }
          else if ((c[0] & 0xFC) == 0x04)
            t = TPDUPtr(new T_Data_Tag_Group_PDU ());
        }
      else
        {
          if ((c[0] & 0xFC) == 0x00)
            t = TPDUPtr(new T_Data_Individual_PDU ());
          else if ((c[0] & 0xC0) == 0x40)
            t = TPDUPtr(new T_Data_Connected_PDU ());
          else if (c[0] == 0x80)
            t = TPDUPtr(new T_Connect_PDU ());
          else if (c[0] == 0x81)
            t = TPDUPtr(new T_Disconnect_PDU ());
          else if ((c[0] & 0xC3) == 0xC2)
            t = TPDUPtr(new T_ACK_PDU ());
          else if ((c[0] & 0xC3) == 0xC3)
            t = TPDUPtr(new T_NAK_PDU ());
        }
    }
  if (t && t->init (c, tr))
    return t;

  t = TPDUPtr(new T_Unknown_PDU ());
  t->init (c, tr);
  return t;
}


/* T_Unknown */

bool
T_Unknown_PDU::init (const CArray & c, TracePtr)
{
  pdu = c;
  return true;
}

CArray T_Unknown_PDU::ToPacket () const
{
  return pdu;
}

std::string T_Unknown_PDU::Decode (TracePtr) const
{
  std::string s ("Unknown TPDU: ");

  if (pdu.size() == 0)
    return "empty TPDU";

  C_ITER (i,pdu)
  addHex (s, *i);

  return s;
}

/* T_Data_Broadcast */

bool
T_Data_Broadcast_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 1)
    return false;

  data = c;
  return true;
}

CArray T_Data_Broadcast_PDU::ToPacket () const
{
  assert (data.size() > 0);

  CArray pdu (data);
  pdu[0] = pdu[0] & 0x03;
  return pdu;
}

std::string T_Data_Broadcast_PDU::Decode (TracePtr tr) const
{
  APDUPtr a = APDU::fromPacket (data, t);
  std::string s ("T_Data_Broadcast ");
  s += a->Decode (tr);
  return s;
}

/* T_Data_SystemBroadcast */

bool
T_Data_SystemBroadcast_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 1)
    return false;

  data = c;
  return true;
}

CArray T_Data_SystemBroadcast_PDU::ToPacket () const
{
  assert (data.size() > 0);

  CArray pdu (data);
  pdu[0] = pdu[0] & 0x03;
  return pdu;
}

std::string T_Data_SystemBroadcast_PDU::Decode (TracePtr tr) const
{
  APDUPtr a = APDU::fromPacket (data, t);
  std::string s ("T_Data_SystemBroadcast ");
  s += a->Decode (tr);
  return s;
}

/* T_Data_Group */

bool
T_Data_Group_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 1)
    return false;

  data = c;
  return true;
}

CArray T_Data_Group_PDU::ToPacket () const
{
  assert (data.size() > 0);

  CArray pdu (data);
  pdu[0] = pdu[0] & 0x03;
  return pdu;
}

std::string T_Data_Group_PDU::Decode (TracePtr tr) const
{
  APDUPtr a = APDU::fromPacket (data, t);
  std::string s ("T_Data_Group ");
  s += a->Decode (tr);
  return s;
}

/* T_Data_Tag_Group */

bool
T_Data_Tag_Group_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 1)
    return false;

  data = c;
  return true;
}

CArray T_Data_Tag_Group_PDU::ToPacket () const
{
  assert (data.size() > 0);

  CArray pdu (data);
  pdu[0] = pdu[0] & 0x03;
  return pdu;
}

std::string T_Data_Tag_Group_PDU::Decode (TracePtr tr) const
{
  APDUPtr a = APDU::fromPacket (data, t);
  std::string s ("T_Data_Tag_Group ");
  s += a->Decode (tr);
  return s;
}

/* T_Data_Individual */

bool
T_Data_Individual_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 1)
    return false;

  data = c;
  return true;
}

CArray T_Data_Individual_PDU::ToPacket () const
{
  assert (data.size() > 0);

  CArray pdu (data);
  pdu[0] = pdu[0] & 0x03;
  return pdu;
}

std::string T_Data_Individual_PDU::Decode (TracePtr tr) const
{
  APDUPtr a = APDU::fromPacket (data, t);
  std::string s ("T_Data_Individual ");
  s += a->Decode (tr);
  return s;
}

/* T_Data_Connected */

bool
T_Data_Connected_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 1)
    return false;

  data = c;
  sequence_number = (c[0] >> 2) & 0x0f;
  return true;
}

CArray T_Data_Connected_PDU::ToPacket () const
{
  assert (data.size() > 0);
  assert ((sequence_number & 0xf0) == 0);

  CArray pdu (data);
  pdu[0] = 0x40 | ((sequence_number & 0x0f) << 2) | (pdu[0] & 0x03);
  return pdu;
}

std::string T_Data_Connected_PDU::Decode (TracePtr tr) const
{
  assert ((sequence_number & 0xf0) == 0);

  APDUPtr a = APDU::fromPacket (data, t);
  std::string s ("T_Data_Connected serno:");
  addHex (s, sequence_number);
  s += a->Decode (tr);
  return s;
}

/* T_Connect */

bool
T_Connect_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 1)
    return false;
  return true;
}

CArray T_Connect_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(1);
  pdu[0] = 0x80;
  return pdu;
}

std::string T_Connect_PDU::Decode (TracePtr) const
{
  return "T_Connect";
}

/* T_Disconnect */

bool
T_Disconnect_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 1)
    return false;
  return true;
}

CArray T_Disconnect_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(1);
  pdu[0] = 0x81;
  return pdu;
}

std::string T_Disconnect_PDU::Decode (TracePtr) const
{
  return "T_Disconnect";
}

/* T_ACK */

bool
T_ACK_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 1)
    return false;

  sequence_number = (c[0] >> 2) & 0x0f;
  return true;
}

CArray T_ACK_PDU::ToPacket () const
{
  assert ((sequence_number & 0xf0) == 0);

  CArray pdu;
  pdu.resize(1);
  pdu[0] = 0xC2 | ((sequence_number & 0x0f) << 2);
  return pdu;
}

std::string T_ACK_PDU::Decode (TracePtr) const
{
  assert ((sequence_number & 0xf0) == 0);

  std::string s ("T_ACK Serno:");
  addHex (s, sequence_number);
  return s;
}

/* T_NAK  */

bool T_NAK_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 1)
    return false;

  sequence_number = (c[0] >> 2) & 0x0f;
  return true;
}

CArray T_NAK_PDU::ToPacket () const
{
  assert ((sequence_number & 0xf0) == 0);

  CArray pdu;
  pdu.resize(1);
  pdu[0] = 0xC3 | ((sequence_number & 0x0f) << 2);
  return pdu;
}

std::string T_NAK_PDU::Decode (TracePtr) const
{
  assert ((sequence_number & 0xf0) == 0);

  std::string s ("T_NAK Serno:");
  addHex (s, sequence_number);
  return s;
}
