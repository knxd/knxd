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

#include "apdu.h"

#include <cstdio>
#include <cstring>

APDUPtr
APDU::fromPacket (const CArray & c, TracePtr tr)
{
  /* @todo provide additional parameter or split function to differentiate:
   * - multicast
   * - broadcast
   * - system broadcast
   * - p2p connectionless
   * - p2p connection-oriented
   */
  APDUPtr a;
  if (c.size() >= 2)
    {
      uint16_t apci = ((c[0] & 0x03) << 8) | c[1];
      switch (apci)
        {
        case A_GroupValue_Read:
          a = APDUPtr(new A_GroupValue_Read_PDU ());
          break;
        case A_GroupValue_Response ... A_GroupValue_Response | 0x03F:
          a = APDUPtr(new A_GroupValue_Response_PDU ());
          break;
        case A_GroupValue_Write ... A_GroupValue_Write | 0x03F:
          a = APDUPtr(new A_GroupValue_Write_PDU ());
          break;
        case A_IndividualAddress_Write:
          a = APDUPtr(new A_IndividualAddress_Write_PDU ());
          break;
        case A_IndividualAddress_Read:
          a = APDUPtr(new A_IndividualAddress_Read_PDU ());
          break;
        case A_IndividualAddress_Response:
          a = APDUPtr(new A_IndividualAddress_Response_PDU ());
          break;
        case A_ADC_Read ... A_ADC_Read | 0x03F:
          a = APDUPtr(new A_ADC_Read_PDU ());
          break;
        case A_ADC_Response ... A_ADC_Response | 0x03F:
          a = APDUPtr(new A_ADC_Response_PDU ());
          break;
        /* @todo
        case A_SystemNetworkParameter_Read: // same as A_ADC_Response with channel_nr=0x80
          a = APDUPtr(new A_SystemNetworkParameter_Read_PDU ());
          break;
        case A_SystemNetworkParameter_Response: // same as A_ADC_Response with channel_nr=0x81
          a = APDUPtr(new A_SystemNetworkParameter_Response_PDU ());
          break;
        case A_SystemNetworkParameter_Write: // same as A_ADC_Response with channel_nr=0x82
          a = APDUPtr(new A_SystemNetworkParameter_Write_PDU ());
          break;
        */
        case A_Memory_Read ... A_Memory_Read | 0x00F:
          a = APDUPtr(new A_Memory_Read_PDU ());
          break;
        case A_Memory_Response ... A_Memory_Response | 0x00F:
          a = APDUPtr(new A_Memory_Response_PDU ());
          break;
        case A_Memory_Write ... A_Memory_Write | 0x00F:
          a = APDUPtr(new A_Memory_Write_PDU ());
          break;
        case A_UserMemory_Read:
          a = APDUPtr(new A_UserMemory_Read_PDU ());
          break;
        case A_UserMemory_Response:
          a = APDUPtr(new A_UserMemory_Response_PDU ());
          break;
        case A_UserMemory_Write:
          a = APDUPtr(new A_UserMemory_Write_PDU ());
          break;
        case A_UserMemoryBit_Write:
          a = APDUPtr(new A_UserMemoryBit_Write_PDU ());
          break;
        case A_UserManufacturerInfo_Read:
          a = APDUPtr(new A_UserManufacturerInfo_Read_PDU ());
          break;
        case A_UserManufacturerInfo_Response:
          a = APDUPtr(new A_UserManufacturerInfo_Response_PDU ());
          break;
        case A_FunctionPropertyCommand:
          a = APDUPtr(new A_FunctionPropertyCommand_PDU ());
          break;
        case A_FunctionPropertyState_Read:
          a = APDUPtr(new A_FunctionPropertyState_Read_PDU ());
          break;
        case A_FunctionPropertyState_Response:
          a = APDUPtr(new A_FunctionPropertyState_Response_PDU ());
          break;
        case A_DeviceDescriptor_Read ... A_DeviceDescriptor_Read | 0x03F:
          a = APDUPtr(new A_DeviceDescriptor_Read_PDU ());
          break;
        case A_DeviceDescriptor_Response ... A_DeviceDescriptor_Response | 0x03F:
          a = APDUPtr(new A_DeviceDescriptor_Response_PDU ());
          break;
        case A_Restart ... A_Restart | 0x01F:
          a = APDUPtr(new A_Restart_PDU ());
          break;
        case A_Restart_Response ... A_Restart_Response | 0x01F:
          a = APDUPtr(new A_Restart_Response_PDU ());
          break;
        case A_Open_Routing_Table_Request:
          a = APDUPtr(new A_Open_Routing_Table_Request_PDU ());
          break;
        case A_Read_Routing_Table_Request:
          a = APDUPtr(new A_Read_Routing_Table_Request_PDU ());
          break;
        case A_Read_Routing_Table_Response:
          a = APDUPtr(new A_Read_Routing_Table_Response_PDU ());
          break;
        case A_Write_Routing_Table_Request:
          a = APDUPtr(new A_Write_Routing_Table_Request_PDU ());
          break;
        case A_Read_Router_Memory_Request:
          a = APDUPtr(new A_Read_Router_Memory_Request_PDU ());
          break;
        case A_Read_Router_Memory_Response:
          a = APDUPtr(new A_Read_Router_Memory_Response_PDU ());
          break;
        case A_Write_Router_Memory_Request:
          a = APDUPtr(new A_Write_Router_Memory_Request_PDU ());
          break;
        case A_Read_Router_Status_Request:
          a = APDUPtr(new A_Read_Router_Status_Request_PDU ());
          break;
        case A_Read_Router_Status_Response:
          a = APDUPtr(new A_Read_Router_Status_Response_PDU ());
          break;
        case A_Write_Router_Status_Request:
          a = APDUPtr(new A_Write_Router_Status_Request_PDU ());
          break;
        case A_MemoryBit_Write:
          a = APDUPtr(new A_MemoryBit_Write_PDU ());
          break;
        case A_Authorize_Request:
          a = APDUPtr(new A_Authorize_Request_PDU ());
          break;
        case A_Authorize_Response:
          a = APDUPtr(new A_Authorize_Response_PDU ());
          break;
        case A_Key_Write:
          a = APDUPtr(new A_Key_Write_PDU ());
          break;
        case A_Key_Response:
          a = APDUPtr(new A_Key_Response_PDU ());
          break;
        case A_PropertyValue_Read:
          a = APDUPtr(new A_PropertyValue_Read_PDU ());
          break;
        case A_PropertyValue_Response:
          a = APDUPtr(new A_PropertyValue_Response_PDU ());
          break;
        case A_PropertyValue_Write:
          a = APDUPtr(new A_PropertyValue_Write_PDU ());
          break;
        case A_PropertyDescription_Read:
          a = APDUPtr(new A_PropertyDescription_Read_PDU ());
          break;
        case A_PropertyDescription_Response:
          a = APDUPtr(new A_PropertyDescription_Response_PDU ());
          break;
        case A_NetworkParameter_Read:
          a = APDUPtr(new A_NetworkParameter_Read_PDU ());
          break;
        case A_NetworkParameter_Response:
          a = APDUPtr(new A_NetworkParameter_Response_PDU ());
          break;
        case A_IndividualAddressSerialNumber_Read:
          a = APDUPtr(new A_IndividualAddressSerialNumber_Read_PDU ());
          break;
        case A_IndividualAddressSerialNumber_Response:
          a = APDUPtr(new A_IndividualAddressSerialNumber_Response_PDU ());
          break;
        case A_IndividualAddressSerialNumber_Write:
          a = APDUPtr(new A_IndividualAddressSerialNumber_Write_PDU ());
          break;
        case A_ServiceInformation_Indication_Write:
          a = APDUPtr(new A_ServiceInformation_Indication_Write_PDU ());
          break;
        case A_DomainAddress_Write:
          a = APDUPtr(new A_DomainAddress_Write_PDU ());
          break;
        case A_DomainAddress_Read:
          a = APDUPtr(new A_DomainAddress_Read_PDU ());
          break;
        case A_DomainAddress_Response:
          a = APDUPtr(new A_DomainAddress_Response_PDU ());
          break;
        case A_DomainAddressSelective_Read:
          a = APDUPtr(new A_DomainAddressSelective_Read_PDU ());
          break;
        case A_NetworkParameter_Write:
          a = APDUPtr(new A_NetworkParameter_Write_PDU ());
          break;
        case A_Link_Read:
          a = APDUPtr(new A_Link_Read_PDU ());
          break;
        case A_Link_Response:
          a = APDUPtr(new A_Link_Response_PDU ());
          break;
        case A_Link_Write:
          a = APDUPtr(new A_Link_Write_PDU ());
          break;
        case A_GroupPropValue_Read:
          a = APDUPtr(new A_GroupPropValue_Read_PDU ());
          break;
        case A_GroupPropValue_Response:
          a = APDUPtr(new A_GroupPropValue_Response_PDU ());
          break;
        case A_GroupPropValue_Write:
          a = APDUPtr(new A_GroupPropValue_Write_PDU ());
          break;
        case A_GroupPropValue_InfoReport:
          a = APDUPtr(new A_GroupPropValue_InfoReport_PDU ());
          break;
        case A_DomainAddressSerialNumber_Read:
          a = APDUPtr(new A_DomainAddressSerialNumber_Read_PDU ());
          break;
        case A_DomainAddressSerialNumber_Response:
          a = APDUPtr(new A_DomainAddressSerialNumber_Response_PDU ());
          break;
        case A_DomainAddressSerialNumber_Write:
          a = APDUPtr(new A_DomainAddressSerialNumber_Write_PDU ());
          break;
        case A_FileStream_InfoReport:
          a = APDUPtr(new A_FileStream_InfoReport_PDU ());
          break;
        default:
          break;
        }
    }
  if (a && a->init (c, tr))
    return a;
  a = APDUPtr(new A_Unknown_PDU);
  a->init (c, tr);
  return a;
}

/* A_Unknown_PDU */

bool
A_Unknown_PDU::init (const CArray & c, TracePtr)
{
  pdu = c;
  return true;
}

CArray A_Unknown_PDU::ToPacket () const
{
  return pdu;
}

std::string A_Unknown_PDU::Decode (TracePtr) const
{
  if (pdu.size() == 0)
    return "empty APDU";

  std::string s ("Unknown APDU: ");
  C_ITER (i, pdu)
  addHex (s, *i);

  return s;
}

bool A_Unknown_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_GroupValue_Read */

bool
A_GroupValue_Read_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 2)
    return false;
  return true;
}

CArray A_GroupValue_Read_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(2);
  pdu[0] = A_GroupValue_Read >> 8;
  pdu[1] = A_GroupValue_Read & 0xff;
  return pdu;
}

std::string A_GroupValue_Read_PDU::Decode (TracePtr) const
{
  return "A_GroupValue_Read";
}

bool A_GroupValue_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_GroupValue_Response */

bool
A_GroupValue_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 2)
    return false;

  issmall = (c.size() == 2);
  if (issmall)
    {
      data.resize(1);
      data[0] = c[1] & 0x3f;
    }
  else
    {
      data.set (c.data() + 2, c.size() - 2);
    }
  return true;
}

CArray A_GroupValue_Response_PDU::ToPacket () const
{
  assert (!issmall || (data.size() == 1 && (data[0] & 0xC0) == 0));

  CArray pdu;
  pdu.resize (2);
  pdu[0] = A_GroupValue_Response >> 8;
  pdu[1] = A_GroupValue_Response & 0xc0;
  if (issmall)
    {
      pdu[1] |= data[0] & 0x3f;
    }
  else
    {
      pdu.resize (2 + data.size());
      pdu.setpart (data.data(), 2, data.size());
    }
  return pdu;
}

std::string A_GroupValue_Response_PDU::Decode (TracePtr) const
{
  assert (!issmall || (data.size() == 1 && (data[0] & 0xC0) == 0));

  std::string s ("A_GroupValue_Response ");
  if (issmall)
    s += "(small) ";
  C_ITER (i, data)
  addHex (s, *i);
  return s;
}

bool A_GroupValue_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_GroupValue_Read)
    return false;
  return true;
}

/* A_GroupValue_Write */

bool
A_GroupValue_Write_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 2)
    return false;

  issmall = (c.size() == 2);
  if (issmall)
    {
      data.resize(1);
      data[0] = c[1] & 0x3f;
    }
  else
    {
      data.set (c.data() + 2, c.size() - 2);
    }
  return true;
}

CArray A_GroupValue_Write_PDU::ToPacket () const
{
  assert (!issmall || (data.size() == 1 && (data[0] & 0xC0) == 0));

  CArray pdu;
  pdu.resize (2);
  pdu[0] = A_GroupValue_Response >> 8;
  pdu[1] = A_GroupValue_Response & 0xc0;
  if (issmall)
    {
      pdu[1] |= data[0] & 0x3F;
    }
  else
    {
      pdu.resize (2 + data.size());
      pdu.setpart (data.data(), 2, data.size());
    }
  return pdu;
}

std::string A_GroupValue_Write_PDU::Decode (TracePtr) const
{
  assert (!issmall || (data.size() == 1 && (data[0] & 0xC0) == 0));

  std::string s ("A_GroupValue_Write ");
  if (issmall)
    s += "(small) ";
  C_ITER (i, data)
  addHex (s, *i);
  return s;
}

bool A_GroupValue_Write_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_IndividualAddress_Write */

bool
A_IndividualAddress_Write_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 4)
    return false;

  newaddress = (c[2] << 8) | (c[3]);
  return true;
}

CArray A_IndividualAddress_Write_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (4);
  pdu[0] = A_IndividualAddress_Write >> 8;
  pdu[1] = A_IndividualAddress_Write & 0xff;
  pdu[2] = newaddress >> 8;
  pdu[3] = newaddress & 0xff;
  return pdu;
}

std::string A_IndividualAddress_Write_PDU::Decode (TracePtr) const
{
  std::string s ("A_IndividualAddress_Write ");
  return s + FormatEIBAddr (newaddress);
}

bool A_IndividualAddress_Write_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_IndividualAddress_Read */

bool
A_IndividualAddress_Read_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 2)
    return false;
  return true;
}

CArray A_IndividualAddress_Read_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(2);
  pdu[0] = A_IndividualAddress_Read >> 8;
  pdu[1] = A_IndividualAddress_Read & 0xff;
  return pdu;
}

std::string A_IndividualAddress_Read_PDU::Decode (TracePtr) const
{
  return "A_IndividualAddress_Read";
}

bool A_IndividualAddress_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_IndividualAddress_Response */

bool A_IndividualAddress_Response_PDU::init (const CArray & c, TracePtr tr)
{
  if (c.size() != 2)
    {
      TRACEPRINTF (tr, 3, "BadLen %d",c.size());
      return false;
    }
  return true;
}

CArray A_IndividualAddress_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(2);
  pdu[0] = A_IndividualAddress_Response >> 8;
  pdu[1] = A_IndividualAddress_Response & 0xff;
  return pdu;
}

std::string A_IndividualAddress_Response_PDU::Decode (TracePtr) const
{
  return "A_IndividualAddress_Response";
}

bool A_IndividualAddress_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_IndividualAddress_Read)
    return false;
  return true;
}

/* A_ADC_Read */

bool
A_ADC_Read_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 3)
    return false;

  channel_nr = c[1] & 0x3F;
  read_count = c[2];
  return true;
}

CArray
A_ADC_Read_PDU::ToPacket () const
{
  assert ((channel_nr & 0xC0) == 0);

  CArray pdu;
  pdu.resize (3);
  pdu[0] = A_ADC_Read >> 8;
  pdu[1] = (A_ADC_Read & 0xc0) | (channel_nr & 0x3F);
  pdu[2] = read_count;
  return pdu;
}

std::string
A_ADC_Read_PDU::Decode (TracePtr) const
{
  assert ((channel_nr & 0xC0) == 0);

  std::string s ("A_ADC_Read Channel:");
  addHex (s, channel_nr);
  s += " Count: ";
  addHex (s, read_count);
  return s;
}

bool A_ADC_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_ADC_Response */

bool
A_ADC_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 5)
    return false;

  channel_nr = c[1] & 0x3F;
  read_count = c[2];
  sum = (c[3] << 8) | (c[4]);
  return true;
}

CArray
A_ADC_Response_PDU::ToPacket () const
{
  assert ((channel_nr & 0xC0) == 0);

  CArray pdu;
  pdu.resize (5);
  pdu[0] = A_ADC_Read >> 8;
  pdu[1] = (A_ADC_Read & 0xc0) | (channel_nr & 0x3F);
  pdu[2] = read_count;
  pdu[3] = sum >> 8;
  pdu[4] = sum & 0xff;
  return pdu;
}

std::string
A_ADC_Response_PDU::Decode (TracePtr) const
{
  assert ((channel_nr & 0xC0) == 0);

  std::string s ("A_ADC_Response Channel:");
  addHex (s, channel_nr);
  s += " Count: ";
  addHex (s, read_count);
  s += "Value: ";
  addHex (s, sum);
  return s;
}

bool A_ADC_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_ADC_Read)
    return false;
  const A_ADC_Read_PDU * a = (const A_ADC_Read_PDU *) req;
  if (a->channel_nr != channel_nr)
    return false;
  if (a->read_count != read_count)
    return false;
  return true;
}

/* A_SystemNetworkParameter_Read */

bool
A_SystemNetworkParameter_Read_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 5)
    return false;
  // @todo
  return true;
}

CArray
A_SystemNetworkParameter_Read_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(2);
  pdu[0] = A_SystemNetworkParameter_Read >> 8;
  pdu[1] = A_SystemNetworkParameter_Read & 0xff;
  // @todo
  return pdu;
}

std::string
A_SystemNetworkParameter_Read_PDU::Decode (TracePtr) const
{
  std::string s ("A_SystemNetworkParameter_Read");
  // @todo
  return s;
}

bool A_SystemNetworkParameter_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_SystemNetworkParameter_Response */

bool
A_SystemNetworkParameter_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 5)
    return false;
  // @todo
  return true;
}

CArray
A_SystemNetworkParameter_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(2);
  pdu[0] = A_SystemNetworkParameter_Response >> 8;
  pdu[1] = A_SystemNetworkParameter_Response & 0xff;
  // @todo
  return pdu;
}

std::string
A_SystemNetworkParameter_Response_PDU::Decode (TracePtr) const
{
  std::string s ("A_SystemNetworkParameter_Response");
  // @todo
  return s;
}

bool A_SystemNetworkParameter_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_SystemNetworkParameter_Read)
    return false;
  return true;
}

/* A_SystemNetworkParameter_Write */

bool
A_SystemNetworkParameter_Write_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 6)
    return false;
  // @todo
  return true;
}

CArray
A_SystemNetworkParameter_Write_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(2);
  pdu[0] = A_SystemNetworkParameter_Write >> 8;
  pdu[1] = A_SystemNetworkParameter_Write & 0xff;
  // @todo
  return pdu;
}

std::string
A_SystemNetworkParameter_Write_PDU::Decode (TracePtr) const
{
  std::string s ("A_SystemNetworkParameter_Write");
  // @todo
  return s;
}

bool A_SystemNetworkParameter_Write_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_Memory_Read */

bool
A_Memory_Read_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 4)
    return false;

  number = c[1] & 0xf;
  address = (c[2] << 8) | c[3];
  return true;
}

CArray
A_Memory_Read_PDU::ToPacket () const
{
  assert ((number & 0xf0) == 0);

  CArray pdu;
  pdu.resize (4);
  pdu[0] = A_Memory_Read >> 8;
  pdu[1] = (A_Memory_Read & 0xf0) | (number & 0x0f);
  pdu[2] = address >> 8;
  pdu[3] = address & 0xff;
  return pdu;
}

std::string
A_Memory_Read_PDU::Decode (TracePtr) const
{
  assert ((number & 0xf0) == 0);

  std::string s ("A_Memory_Read Len: ");
  addHex (s, number);
  s += " Addr: ";
  add16Hex (s, address);
  return s;
}

bool A_Memory_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_Memory_Response */

bool
A_Memory_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 4)
    return false;

  number = c[1] & 0xf;
  address = (c[2] << 8) | c[3];
  data.set (c.data() + 4, c.size() - 4);
  if (data.size() != number)
    return false;
  return true;
}

CArray
A_Memory_Response_PDU::ToPacket () const
{
  assert ((number & 0xf0) == 0);
  assert (data.size() == number);

  CArray pdu;
  pdu.resize (4 + data.size());
  pdu[0] = A_Memory_Response >> 8;
  pdu[1] = (A_Memory_Response & 0xf0) | (number & 0x0f);
  pdu[2] = address >> 8;
  pdu[3] = address & 0xff;
  pdu.setpart (data.data(), 4, data.size());
  return pdu;
}

std::string
A_Memory_Response_PDU::Decode (TracePtr) const
{
  assert ((number & 0xf0) == 0);
  assert (data.size() == number);

  std::string s ("A_Memory_Response Len:");
  addHex (s, number);
  s += " Addr: ";
  add16Hex (s, address);
  s += "Data: ";
  C_ITER (i,data)
  addHex (s, *i);
  return s;
}

bool A_Memory_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_Memory_Read)
    return false;
  const A_Memory_Read_PDU *
  a = (const A_Memory_Read_PDU *) req;
  if (a->number != number)
    return false;
  if (a->address != address)
    return false;
  return true;
}

/* A_Memory_Write */

bool
A_Memory_Write_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 4)
    return false;

  number = c[1] & 0xf;
  address = (c[2] << 8) | c[3];
  data.set (c.data() + 4, c.size() - 4);
  if (data.size() != number)
    return false;
  return true;
}

CArray
A_Memory_Write_PDU::ToPacket () const
{
  assert ((number & 0xf0) == 0);
  assert (data.size() == number);

  CArray pdu;
  pdu.resize (4 + data.size());
  pdu[0] = A_Memory_Write >> 8;
  pdu[1] = (A_Memory_Write & 0xf0) | (number & 0x0f);
  pdu[2] = address >> 8;
  pdu[3] = address & 0xff;
  pdu.setpart (data.data(), 4, data.size());
  return pdu;
}

std::string
A_Memory_Write_PDU::Decode (TracePtr) const
{
  assert ((number & 0xf0) == 0);
  assert (data.size() == number);

  std::string s ("A_Memory_Write Len:");
  addHex (s, number);
  s += " Addr: ";
  add16Hex (s, address);
  s += "Data: ";
  C_ITER (i,data)
  addHex (s, *i);
  return s;
}

bool A_Memory_Write_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_UserMemory_Read */

bool
A_UserMemory_Read_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 5)
    return false;

  address_extension = (c[2] >> 4) & 0xf;
  number = c[2] & 0xf;
  address = (c[3] << 8) | c[4];
  return true;
}

CArray
A_UserMemory_Read_PDU::ToPacket () const
{
  assert ((address_extension & 0xf0) == 0);
  assert ((number & 0xf0) == 0);

  CArray pdu;
  pdu.resize (5);
  pdu[0] = A_UserMemory_Read >> 8;
  pdu[1] = A_UserMemory_Read & 0xff;
  pdu[2] = (address_extension & 0x0f) << 4 | (number & 0x0f);
  pdu[3] = address >> 8;
  pdu[4] = address & 0xff;
  return pdu;
}

std::string
A_UserMemory_Read_PDU::Decode (TracePtr) const
{
  assert ((address_extension & 0xf0) == 0);
  assert ((number & 0xf0) == 0);

  std::string s ("A_UserMemory_Read Addr_ext:");
  addHex (s, address_extension);
  s += " Len: ";
  addHex (s, number);
  s += " Addr: ";
  add16Hex (s, address);
  return s;
}

bool A_UserMemory_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_UserMemory_Response */

bool
A_UserMemory_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 5)
    return false;

  address_extension = (c[2] >> 4) & 0xf;
  number = c[2] & 0xf;
  address = (c[3] << 8) | c[4];
  data.set (c.data() + 5, c.size() - 5);
  if (data.size() != number)
    return false;
  return true;
}

CArray
A_UserMemory_Response_PDU::ToPacket () const
{
  assert ((address_extension & 0xf0) == 0);
  assert ((number & 0xf0) == 0);
  assert (data.size() == number);

  CArray pdu;
  pdu.resize (5 + data.size());
  pdu[0] = A_UserMemory_Response >> 8;
  pdu[1] = A_UserMemory_Response & 0xff;
  pdu[2] = (address_extension & 0x0f) << 4 | (number & 0x0f);
  pdu[3] = address >> 8;
  pdu[4] = address & 0xff;
  pdu.setpart (data.data(), 5, data.size());
  return pdu;
}

std::string
A_UserMemory_Response_PDU::Decode (TracePtr) const
{
  assert ((address_extension & 0xf0) == 0);
  assert ((number & 0xf0) == 0);
  assert (data.size() == number);

  std::string s ("A_UserMemory_Response Addr_ext:");
  addHex (s, address_extension);
  s += " Len: ";
  addHex (s, number);
  s += " Addr: ";
  add16Hex (s, address);
  s += " Data: ";
  C_ITER(i,data)
  addHex (s, *i);
  return s;
}

bool A_UserMemory_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_UserMemory_Read)
    return false;
  const A_UserMemory_Read_PDU *
  a = (const A_UserMemory_Read_PDU *) req;
  if (a->address_extension != address_extension)
    return false;
  if (a->number != number)
    return false;
  if (a->address != address)
    return false;
  return true;
}

/* A_UserMemory_Write */

bool
A_UserMemory_Write_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 5)
    return false;

  address_extension = (c[2] >> 4) & 0xf;
  number = c[2] & 0xf;
  address = (c[3] << 8) | c[4];
  data.set (c.data() + 5, c.size() - 5);
  if (data.size() != number)
    return false;
  return true;
}

CArray
A_UserMemory_Write_PDU::ToPacket () const
{
  assert ((address_extension & 0xf0) == 0);
  assert ((number & 0xf0) == 0);
  assert (data.size() == number);

  CArray pdu;
  pdu.resize (5 + data.size());
  pdu[0] = A_UserMemory_Write >> 8;
  pdu[1] = A_UserMemory_Write & 0xff;
  pdu[2] = (address_extension & 0x0f) << 4 | (number & 0x0f);
  pdu[3] = address >> 8;
  pdu[4] = address & 0xff;
  pdu.setpart (data.data(), 5, data.size());
  return pdu;
}

std::string
A_UserMemory_Write_PDU::Decode (TracePtr) const
{
  assert ((address_extension & 0xf0) == 0);
  assert ((number & 0xf0) == 0);
  assert (data.size() == number);

  std::string s ("A_UserMemory_Write Addr_ext:");
  addHex (s, address_extension);
  s += " Len: ";
  addHex (s, number);
  s += " Addr: ";
  add16Hex (s, address);
  s += " Data: ";
  C_ITER (i,data)
  addHex (s, *i);
  return s;
}

bool A_UserMemory_Write_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_UserMemoryBit_Write */

bool
A_UserMemoryBit_Write_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 5)
    return false;

  number = c[2];
  address = (c[3] << 8) | c[4];
  if (c.size() - 5 != number * 2)
    return false;
  and_data.set (c.data() + 5, number);
  xor_data.set (c.data() + 5 + number, number);
  return true;
}

CArray
A_UserMemoryBit_Write_PDU::ToPacket () const
{
  assert (and_data.size() == number);
  assert (xor_data.size() == number);

  CArray pdu;
  pdu.resize (5 + 2 * number);
  pdu[0] = A_UserMemoryBit_Write >> 8;
  pdu[1] = A_UserMemoryBit_Write & 0xff;
  pdu[2] = number;
  pdu[3] = address >> 8;
  pdu[4] = address & 0xff;
  pdu.setpart (and_data.data(), 5, number);
  pdu.setpart (xor_data.data(), 5 + number, number);
  return pdu;
}

std::string
A_UserMemoryBit_Write_PDU::Decode (TracePtr) const
{
  assert (and_data.size() == number);
  assert (xor_data.size() == number);

  std::string s ("A_UserMemoryBit_Write Len:");
  addHex (s, number);
  s += "Addr: ";
  add16Hex (s, address);
  s += "And: ";
  C_ITER(i, and_data)
  addHex (s, *i);
  s += "xor: ";
  C_ITER(i, xor_data)
  addHex (s, *i);
  return s;
}

bool A_UserMemoryBit_Write_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_UserManufacturerInfo_Read */

bool A_UserManufacturerInfo_Read_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 2)
    return false;
  return true;
}

CArray
A_UserManufacturerInfo_Read_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (2);
  pdu[0] = A_UserManufacturerInfo_Read >> 8;
  pdu[1] = A_UserManufacturerInfo_Read & 0xff;
  return pdu;
}

std::string
A_UserManufacturerInfo_Read_PDU::Decode (TracePtr) const
{
  std::string s ("A_UserManufacturerInfo_Read");
  return s;
}

bool A_UserManufacturerInfo_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_UserManufacturerInfo_Response */

bool
A_UserManufacturerInfo_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 5)
    return false;

  manufacturer_id = c[2];
  manufacturer_data = (c[3] << 8) | c[4];
  return true;
}

CArray
A_UserManufacturerInfo_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (5);
  pdu[0] = A_UserManufacturerInfo_Response >> 8;
  pdu[1] = A_UserManufacturerInfo_Response & 0xff;
  pdu[2] = manufacturer_id;
  pdu[3] = manufacturer_data >> 8;
  pdu[4] = manufacturer_data & 0xff;
  return pdu;
}

std::string
A_UserManufacturerInfo_Response_PDU::Decode (TracePtr) const
{
  std::string s ("A_UserManufactueerInfo_Response Manufacturer:");
  addHex (s, manufacturer_id);
  s += " data: ";
  add16Hex (s, manufacturer_data);
  return s;
}

bool A_UserManufacturerInfo_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_UserManufacturerInfo_Read)
    return false;
  return true;
}

/* A_FunctionPropertyCommand */

bool
A_FunctionPropertyCommand_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 4)
    return false;

  object_index = c[2];
  property_id = c[3];
  data.set (c.data() + 4, c.size() - 4);
  return true;
}

CArray
A_FunctionPropertyCommand_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(4 + data.size());
  pdu[0] = A_FunctionPropertyCommand >> 8;
  pdu[1] = A_FunctionPropertyCommand & 0xff;
  pdu[2] = object_index;
  pdu[3] = property_id;
  pdu.setpart (data.data(), 4, data.size());
  return pdu;
}

std::string
A_FunctionPropertyCommand_PDU::Decode (TracePtr) const
{
  std::string s ("A_FunctionPropertyCommand");
  // @todo
  return s;
}

bool A_FunctionPropertyCommand_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_FunctionPropertyState_Read */

bool
A_FunctionPropertyState_Read_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 4)
    return false;

  object_index = c[2];
  property_id = c[3];
  data.set (c.data() + 4, c.size() - 4);
  return true;
}

CArray
A_FunctionPropertyState_Read_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(4 + data.size());
  pdu[0] = A_FunctionPropertyState_Read >> 8;
  pdu[1] = A_FunctionPropertyState_Read & 0xff;
  pdu[2] = object_index;
  pdu[3] = property_id;
  pdu.setpart (data.data(), 4, data.size());
  return pdu;
}

std::string
A_FunctionPropertyState_Read_PDU::Decode (TracePtr) const
{
  std::string s ("A_FunctionPropertyState_Read");
  // @todo
  return s;
}

bool A_FunctionPropertyState_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_FunctionPropertyState_Response */

bool
A_FunctionPropertyState_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 5)
    return false;

  object_index = c[2];
  property_id = c[3];
  return_code = c[4];
  data.set (c.data() + 5, c.size() - 5);
  return true;
}

CArray
A_FunctionPropertyState_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(5 + data.size());
  pdu[0] = A_FunctionPropertyState_Response >> 8;
  pdu[1] = A_FunctionPropertyState_Response & 0xff;
  pdu[2] = object_index;
  pdu[3] = property_id;
  pdu[4] = return_code;
  pdu.setpart (data.data(), 5, data.size());
  return pdu;
}

std::string
A_FunctionPropertyState_Response_PDU::Decode (TracePtr) const
{
  std::string s ("A_FunctionPropertyState_Response");
  // @todo
  return s;
}

bool A_FunctionPropertyState_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_FunctionPropertyState_Read)
    return false;
  const A_FunctionPropertyState_Read_PDU *
  a = (const A_FunctionPropertyState_Read_PDU *) req;
  if (a->object_index != object_index)
    return false;
  if (a->property_id != property_id)
    return false;
  return true;
}

/* A_DeviceDescriptor_Read */

bool
A_DeviceDescriptor_Read_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 2)
    return false;

  descriptor_type = c[1] & 0x3F;
  return true;
}

CArray
A_DeviceDescriptor_Read_PDU::ToPacket () const
{
  assert ((descriptor_type & 0xC0) == 0);

  CArray pdu;
  pdu.resize (2);
  pdu[0] = A_DeviceDescriptor_Read >> 8;
  pdu[1] = (A_DeviceDescriptor_Read & 0xc0) | (descriptor_type & 0x3f);
  return pdu;
}

std::string
A_DeviceDescriptor_Read_PDU::Decode (TracePtr) const
{
  assert ((descriptor_type & 0xC0) == 0);

  std::string s ("A_DeviceDescriptor_Read Type:");
  addHex (s, descriptor_type);
  return s;
}

bool A_DeviceDescriptor_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_DeviceDescriptor_Response */

bool A_DeviceDescriptor_Response_PDU::init (const CArray & c, TracePtr tr)
{
  if (c.size() != 4)
    {
      TRACEPRINTF (tr, 3, "BadLen %d",c.size());
      return false;
    }

  descriptor_type = c[1] & 0x3F;
  device_descriptor = (c[2] << 8) | c[3];
  return true;
}

CArray
A_DeviceDescriptor_Response_PDU::ToPacket () const
{
  assert ((descriptor_type & 0xC0) == 0);

  CArray pdu;
  pdu.resize (4);
  pdu[0] = A_DeviceDescriptor_Response >> 8;
  pdu[1] = (A_DeviceDescriptor_Response & 0xc0) | (descriptor_type & 0x3f);
  pdu[2] = device_descriptor >> 8;
  pdu[3] = device_descriptor & 0xff;
  return pdu;
}

std::string
A_DeviceDescriptor_Response_PDU::Decode (TracePtr) const
{
  assert ((descriptor_type & 0xC0) == 0);

  std::string s ("A_DeviceDescriptor_Response Type:");
  addHex (s, descriptor_type);
  s += " Descriptor: ";
  add16Hex (s, device_descriptor);
  return s;
}

bool A_DeviceDescriptor_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_DeviceDescriptor_Read)
    return false;
  const A_DeviceDescriptor_Read_PDU *
  a = (const A_DeviceDescriptor_Read_PDU *) req;
  if (a->descriptor_type != descriptor_type)
    return false;
  return true;
}

/* A_Restart */

bool
A_Restart_PDU::init (const CArray & c, TracePtr)
{
  if ((c.size() != 2) || (c.size() != 4))
    return false;

  restart_type = c[1] & 0x01;
  if (restart_type == 1)
    {
      erase_code = c[2];
      channel_number = c[3];
    }
  return true;
}

CArray
A_Restart_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (2);
  pdu[0] = A_Restart >> 8;
  pdu[1] = (A_Restart & 0xc0) | restart_type;
  if (restart_type == 1)
    {
      pdu.resize (4);
      pdu[2] = erase_code;
      pdu[3] = channel_number;
    }
  return pdu;
}

std::string
A_Restart_PDU::Decode (TracePtr) const
{
  std::string s ("A_Restart");
  return s;
}

bool A_Restart_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_Restart_Response */

bool
A_Restart_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 5)
    return false;

  restart_type = c[1] & 0x01;
  error_code = c[2];
  process_time = (c[3] << 8) | c[4];
  return true;
}

CArray
A_Restart_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (2);
  pdu[0] = A_Restart_Response >> 8;
  pdu[1] = (A_Restart_Response & 0xc0) | restart_type;
  pdu[2] = error_code;
  pdu[3] = process_time >> 8;
  pdu[4] = process_time & 0xff;
  return pdu;
}

std::string
A_Restart_Response_PDU::Decode (TracePtr) const
{
  std::string s ("A_Restart_Response");
  return s;
}

bool A_Restart_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_Restart)
    return false;
  return true;
}

/* A_Open_Routing_Table_Request */

bool
A_Open_Routing_Table_Request_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 2)
    return false;

  data.set (c.data() + 2, c.size() - 2);
  return true;
}

CArray
A_Open_Routing_Table_Request_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (2 + data.size());
  pdu[0] = A_Open_Routing_Table_Request >> 8;
  pdu[1] = A_Open_Routing_Table_Request & 0xff;
  pdu.setpart (data.data(), 2, data.size());
  return pdu;
}

std::string
A_Open_Routing_Table_Request_PDU::Decode (TracePtr) const
{
  std::string s ("A_Open_Routing_Table_Request: ");
  C_ITER (i, data)
  addHex (s, *i);
  return s;
}

bool A_Open_Routing_Table_Request_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_Read_Routing_Table_Request */

bool
A_Read_Routing_Table_Request_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 2)
    return false;

  data.set (c.data() + 2, c.size() - 2);
  return true;
}

CArray
A_Read_Routing_Table_Request_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (2 + data.size());
  pdu[0] = A_Read_Routing_Table_Request >> 8;
  pdu[1] = A_Read_Routing_Table_Request & 0xff;
  pdu.setpart (data.data(), 2, data.size());
  return pdu;
}

std::string
A_Read_Routing_Table_Request_PDU::Decode (TracePtr) const
{
  std::string s ("A_Read_Routing_Table_Request");
  C_ITER (i, data)
  addHex (s, *i);
  return s;
}

bool A_Read_Routing_Table_Request_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_Read_Routing_Table_Response */

bool
A_Read_Routing_Table_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 2)
    return false;

  data.set (c.data() + 2, c.size() - 2);
  return true;
}

CArray
A_Read_Routing_Table_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (2 + data.size());
  pdu[0] = A_Read_Routing_Table_Response >> 8;
  pdu[1] = A_Read_Routing_Table_Response & 0xff;
  pdu.setpart (data.data(), 2, data.size());
  return pdu;
}

std::string
A_Read_Routing_Table_Response_PDU::Decode (TracePtr) const
{
  std::string s ("A_Read_Routing_Table_Response");
  C_ITER (i, data)
  addHex (s, *i);
  return s;
}

bool A_Read_Routing_Table_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_Read_Routing_Table_Request)
    return false;
  return true;
}

/* A_Write_Routing_Table_Request */

bool
A_Write_Routing_Table_Request_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 2)
    return false;

  data.set (c.data() + 2, c.size() - 2);
  return true;
}

CArray
A_Write_Routing_Table_Request_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (2 + data.size());
  pdu[0] = A_Write_Routing_Table_Request >> 8;
  pdu[1] = A_Write_Routing_Table_Request & 0xff;
  pdu.setpart (data.data(), 2, data.size());
  return pdu;
}

std::string
A_Write_Routing_Table_Request_PDU::Decode (TracePtr) const
{
  std::string s ("A_Write_Routing_Table_Request");
  C_ITER (i, data)
  addHex (s, *i);
  return s;
}

bool A_Write_Routing_Table_Request_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_Read_Router_Memory_Request */

bool
A_Read_Router_Memory_Request_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 2)
    return false;

  data.set (c.data() + 2, c.size() - 2);
  return true;
}

CArray
A_Read_Router_Memory_Request_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (2 + data.size());
  pdu[0] = A_Read_Router_Memory_Request >> 8;
  pdu[1] = A_Read_Router_Memory_Request & 0xff;
  pdu.setpart (data.data(), 2, data.size());
  return pdu;
}

std::string
A_Read_Router_Memory_Request_PDU::Decode (TracePtr) const
{
  std::string s ("A_Read_Router_Memory_Request");
  C_ITER (i, data)
  addHex (s, *i);
  return s;
}

bool A_Read_Router_Memory_Request_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_Read_Router_Memory_Response */

bool
A_Read_Router_Memory_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 2)
    return false;

  data.set (c.data() + 2, c.size() - 2);
  return true;
}

CArray
A_Read_Router_Memory_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (2 + data.size());
  pdu[0] = A_Read_Router_Memory_Response >> 8;
  pdu[1] = A_Read_Router_Memory_Response & 0xff;
  pdu.setpart (data.data(), 2, data.size());
  return pdu;
}

std::string
A_Read_Router_Memory_Response_PDU::Decode (TracePtr) const
{
  std::string s ("A_Read_Router_Memory_Response");
  C_ITER (i, data)
  addHex (s, *i);
  return s;
}

bool A_Read_Router_Memory_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_Read_Router_Memory_Request)
    return false;
  return true;
}

/* A_Write_Router_Memory_Request */

bool
A_Write_Router_Memory_Request_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 2)
    return false;

  data.set (c.data() + 2, c.size() - 2);
  return true;
}

CArray
A_Write_Router_Memory_Request_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (2 + data.size());
  pdu[0] = A_Write_Router_Memory_Request >> 8;
  pdu[1] = A_Write_Router_Memory_Request & 0xff;
  pdu.setpart (data.data(), 2, data.size());
  return pdu;
}

std::string
A_Write_Router_Memory_Request_PDU::Decode (TracePtr) const
{
  std::string s ("A_Write_Router_Memory_Request");
  C_ITER (i, data)
  addHex (s, *i);
  return s;
}

bool A_Write_Router_Memory_Request_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_Read_Router_Status_Request */

bool
A_Read_Router_Status_Request_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 2)
    return false;

  data.set (c.data() + 2, c.size() - 2);
  return true;
}

CArray
A_Read_Router_Status_Request_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (2 + data.size());
  pdu[0] = A_Read_Router_Status_Request >> 8;
  pdu[1] = A_Read_Router_Status_Request & 0xff;
  pdu.setpart (data.data(), 2, data.size());
  return pdu;
}

std::string
A_Read_Router_Status_Request_PDU::Decode (TracePtr) const
{
  std::string s ("A_Read_Router_Status_Request");
  C_ITER (i, data)
  addHex (s, *i);
  return s;
}

bool A_Read_Router_Status_Request_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_Read_Router_Status_Response */

bool
A_Read_Router_Status_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 2)
    return false;

  data.set (c.data() + 2, c.size() - 2);
  return true;
}

CArray
A_Read_Router_Status_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (2 + data.size());
  pdu[0] = A_Read_Router_Status_Response >> 8;
  pdu[1] = A_Read_Router_Status_Response & 0xff;
  pdu.setpart (data.data(), 2, data.size());
  return pdu;
}

std::string
A_Read_Router_Status_Response_PDU::Decode (TracePtr) const
{
  std::string s ("A_Read_Router_Status_Response");
  C_ITER (i, data)
  addHex (s, *i);
  return s;
}

bool A_Read_Router_Status_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_Read_Router_Status_Request)
    return false;
  return true;
}

/* A_Write_Router_Status_Request */

bool
A_Write_Router_Status_Request_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 2)
    return false;

  data.set (c.data() + 2, c.size() - 2);
  return true;
}

CArray
A_Write_Router_Status_Request_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (2 + data.size());
  pdu[0] = A_Read_Router_Status_Request >> 8;
  pdu[1] = A_Read_Router_Status_Request & 0xff;
  pdu.setpart (data.data(), 2, data.size());
  return pdu;
}

std::string
A_Write_Router_Status_Request_PDU::Decode (TracePtr) const
{
  std::string s ("A_Write_Router_Status_Request");
  C_ITER (i, data)
  addHex (s, *i);
  return s;
}

bool A_Write_Router_Status_Request_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_MemoryBit_Write */

bool
A_MemoryBit_Write_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 5)
    return false;

  number = c[2];
  address = (c[3] << 8) | c[4];
  if (c.size() - 5 != number * 2)
    return false;
  and_data.set (c.data() + 5, number);
  xor_data.set (c.data() + 5 + number, number);
  return true;
}

CArray
A_MemoryBit_Write_PDU::ToPacket () const
{
  assert (and_data.size() == number);
  assert (xor_data.size() == number);

  CArray pdu;
  pdu.resize (number * 2 + 5);
  pdu[0] = A_MemoryBit_Write >> 8;
  pdu[1] = A_MemoryBit_Write & 0xff;
  pdu[2] = number;
  pdu[3] = address >> 8;
  pdu[4] = address & 0xff;
  pdu.setpart (and_data.data(), 5, number);
  pdu.setpart (xor_data.data(), 5 + number, number);
  return pdu;
}

std::string
A_MemoryBit_Write_PDU::Decode (TracePtr) const
{
  assert (and_data.size() == number);
  assert (xor_data.size() == number);

  std::string s ("A_MemoryBit_Write Len:");
  addHex (s, number);
  s += "Addr: ";
  add16Hex (s, address);
  s += "And: ";
  C_ITER (i, and_data)
  addHex (s, *i);
  s += "xor: ";
  C_ITER (i, xor_data)
  addHex (s, *i);
  return s;
}

bool A_MemoryBit_Write_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_Authorize_Request */

bool
A_Authorize_Request_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 7)
    return false;

  key = (c[3] << 24) | (c[4] << 16) | (c[5] << 8) | c[6];
  return true;
}

CArray
A_Authorize_Request_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (7);
  pdu[0] = A_Authorize_Request >> 8;
  pdu[1] = A_Authorize_Request & 0xff;
  pdu[2] = 0x00;
  pdu[3] = key >> 24;
  pdu[4] = (key >> 16) & 0xff;
  pdu[5] = (key >> 8) & 0xff;
  pdu[6] = key & 0xff;
  return pdu;
}

std::string
A_Authorize_Request_PDU::Decode (TracePtr) const
{
  std::string s ("A_Authorize_Request Key:");
  return s + FormatEIBKey (key);
}

bool A_Authorize_Request_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_Authorize_Response */

bool
A_Authorize_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 3)
    return false;

  level = c[2];
  return true;
}

CArray
A_Authorize_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (3);
  pdu[0] = A_Authorize_Response >> 8;
  pdu[1] = A_Authorize_Response & 0xff;
  pdu[2] = level;
  return pdu;
}

std::string
A_Authorize_Response_PDU::Decode (TracePtr) const
{
  std::string s ("A_Authorize_Response Level:");
  addHex (s, level);
  return s;
}

bool A_Authorize_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_Authorize_Request)
    return false;
  return true;
}

/* A_Key_Write */

bool
A_Key_Write_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 7)
    return false;

  level = c[2];
  key = (c[3] << 24) | (c[4] << 16) | (c[5] << 8) | c[6];
  return true;
}

CArray
A_Key_Write_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (7);
  pdu[0] = A_Key_Write >> 8;
  pdu[1] = A_Key_Write & 0xff;
  pdu[2] = level;
  pdu[3] = key >> 24;
  pdu[4] = (key >> 16) & 0xff;
  pdu[5] = (key >> 8) & 0xff;
  pdu[6] = key & 0xff;
  return pdu;
}

std::string
A_Key_Write_PDU::Decode (TracePtr) const
{
  std::string s ("A_Key_Write Level:");
  addHex (s, level);
  s += " Key: ";
  return s + FormatEIBKey (key);
}

bool A_Key_Write_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_Key_Response */

bool
A_Key_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 3)
    return false;

  level = c[2];
  return true;
}

CArray
A_Key_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (3);
  pdu[0] = A_Key_Response >> 8;
  pdu[1] = A_Key_Response & 0xff;
  pdu[2] = level;
  return pdu;
}

std::string
A_Key_Response_PDU::Decode (TracePtr) const
{
  std::string s ("A_Key_Response Level:");
  addHex (s, level);
  return s;
}

bool A_Key_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_Key_Write)
    return false;
  const A_Key_Write_PDU *
  a = (const A_Key_Write_PDU *) req;
  return a->level == level;
}

/* A_PropertyValue_Read */

bool
A_PropertyValue_Read_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 6)
    return false;

  object_index = c[2];
  property_id = c[3];
  nr_of_elem = c[4] >> 4;
  start_index = (c[4] & 0x0f) << 8 | c[5];
  return true;
}

CArray
A_PropertyValue_Read_PDU::ToPacket () const
{
  assert ((nr_of_elem & 0xf0) == 0);
  assert ((start_index & 0xf000) == 0);

  CArray pdu;
  pdu.resize (6);
  pdu[0] = A_PropertyValue_Read >> 8;
  pdu[1] = A_PropertyValue_Read & 0xff;
  pdu[2] = object_index;
  pdu[3] = property_id;
  pdu[4] = (nr_of_elem << 4) | (start_index >> 8);
  pdu[5] = start_index & 0xff;
  return pdu;
}

std::string
A_PropertyValue_Read_PDU::Decode (TracePtr) const
{
  assert ((nr_of_elem & 0xf0) == 0);
  assert ((start_index & 0xf000) == 0);

  std::string s ("A_PropertyValue_Read Obj:");
  addHex (s, object_index);
  s += " Prop: ";
  addHex (s, property_id);
  s += " start: ";
  addHex (s, start_index);
  s += " max_nr: ";
  addHex (s, nr_of_elem);
  return s;
}

bool A_PropertyValue_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_PropertyValue_Response */

bool
A_PropertyValue_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 6)
    return false;

  object_index = c[2];
  property_id = c[3];
  nr_of_elem = c[4] >> 4;
  start_index = ((c[4] & 0x0f) << 8) | c[5];
  data.set (c.data() + 6, c.size() - 6);
  return true;
}

CArray
A_PropertyValue_Response_PDU::ToPacket () const
{
  assert ((nr_of_elem & 0xf0) == 0);
  assert ((start_index & 0xf000) == 0);

  CArray pdu;
  pdu.resize (6 + data.size());
  pdu[0] = A_PropertyValue_Response >> 8;
  pdu[1] = A_PropertyValue_Response & 0xff;
  pdu[2] = object_index;
  pdu[3] = property_id;
  pdu[4] = (nr_of_elem << 4) | (start_index >> 8);
  pdu[5] = start_index & 0xff;
  pdu.setpart (data.data(), 6, data.size());
  return pdu;
}

std::string
A_PropertyValue_Response_PDU::Decode (TracePtr) const
{
  assert ((nr_of_elem & 0xf0) == 0);
  assert ((start_index & 0xf000) == 0);

  std::string s ("A_PropertyValue_Response Obj:");
  addHex (s, object_index);
  s += " Prop: ";
  addHex (s, property_id);
  s += " start: ";
  addHex (s, start_index);
  s += " max_nr: ";
  addHex (s, nr_of_elem);
  s += "data: ";
  C_ITER (i,data)
  addHex (s, *i);
  return s;
}

bool A_PropertyValue_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () == A_PropertyValue_Write)
    {
      const A_PropertyValue_Write_PDU *
      a = (const A_PropertyValue_Write_PDU *) req;
      if (a->object_index != object_index)
        return false;
      if (a->property_id != property_id)
        return false;
      if (a->nr_of_elem != nr_of_elem)
        return false;
      if (a->start_index != start_index)
        return false;
      return true;
    }
  if (req->getType () == A_PropertyValue_Read)
    {
      const A_PropertyValue_Read_PDU *
      a = (const A_PropertyValue_Read_PDU *) req;
      if (a->object_index != object_index)
        return false;
      if (a->property_id != property_id)
        return false;
      if (a->nr_of_elem != nr_of_elem)
        return false;
      if (a->start_index != start_index)
        return false;
      return true;
    }
  return false;
}

/* A_PropertyValue_Write */

bool
A_PropertyValue_Write_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 6)
    return false;

  object_index = c[2];
  property_id = c[3];
  nr_of_elem = c[4] >> 4;
  start_index = ((c[4] & 0x0f) << 8) | c[5];
  data.set (c.data() + 6, c.size() - 6);
  return true;
}

CArray
A_PropertyValue_Write_PDU::ToPacket () const
{
  assert ((nr_of_elem & 0xf0) == 0);
  assert ((start_index & 0xf000) == 0);

  CArray pdu;
  pdu.resize (6 + data.size());
  pdu[0] = A_PropertyValue_Write >> 8;
  pdu[1] = A_PropertyValue_Write & 0xff;
  pdu[2] = object_index;
  pdu[3] = property_id;
  pdu[4] = (nr_of_elem << 4) | (start_index >> 8);
  pdu[5] = start_index & 0xff;
  pdu.setpart (data.data(), 6, data.size());
  return pdu;
}

std::string
A_PropertyValue_Write_PDU::Decode (TracePtr) const
{
  assert ((nr_of_elem & 0xf0) == 0);
  assert ((start_index & 0xf000) == 0);

  std::string s ("A_PropertyValue_Write Obj:");
  addHex (s, object_index);
  s += " Prop: ";
  addHex (s, property_id);
  s += " start: ";
  addHex (s, start_index);
  s += " max_nr: ";
  addHex (s, nr_of_elem);
  s += "data: ";
  C_ITER (i,data)
  addHex (s, *i);
  return s;
}

bool A_PropertyValue_Write_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_PropertyDescription_Read */

bool
A_PropertyDescription_Read_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 5)
    return false;

  object_index = c[2];
  property_id = c[3];
  property_index = c[4];
  return true;
}

CArray
A_PropertyDescription_Read_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (5);
  pdu[0] = A_PropertyDescription_Read >> 8;
  pdu[1] = A_PropertyDescription_Read & 0xff;
  pdu[2] = object_index;
  pdu[3] = property_id;
  pdu[4] = property_index;
  return pdu;
}

std::string
A_PropertyDescription_Read_PDU::Decode (TracePtr) const
{
  std::string s ("A_PropertyDescription_Read Obj: ");
  addHex (s, object_index);
  s += " Property: ";
  addHex (s, property_id);
  s += " Property_index: ";
  addHex (s, property_index);
  return s;
}

bool A_PropertyDescription_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_PropertyDescription_Response */

bool
A_PropertyDescription_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 9)
    return false;

  object_index = c[2];
  property_id = c[3];
  property_index = c[4];
  write_enable = c[5] >> 7;
  type = c[5] & 0x3f;
  max_nr_of_elem = ((c[6] & 0x0f) << 8) | c[7];
  access = c[8];
  return true;
}

CArray
A_PropertyDescription_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (9);
  pdu[0] = A_PropertyDescription_Response >> 8;
  pdu[1] = A_PropertyDescription_Response & 0xff;
  pdu[2] = object_index;
  pdu[3] = property_id;
  pdu[4] = property_index;
  pdu[5] = (write_enable << 7) | (type & 0x3f);
  pdu[6] = max_nr_of_elem >> 8;
  pdu[7] = max_nr_of_elem & 0xff;
  pdu[8] = access;
  return pdu;
}

std::string
A_PropertyDescription_Response_PDU::Decode (TracePtr) const
{
  std::string s ("A_PropertyDescription_Response Obj:");
  addHex (s, object_index);
  s += " Property: ";
  addHex (s, property_id);
  s += " Property_index: ";
  addHex (s, property_index);
  // @todo write_enable
  s += " Type: ";
  addHex (s, type);
  s += "max_elements: ";
  add16Hex (s, max_nr_of_elem);
  s += " access: ";
  addHex (s, access);
  return s;
}

bool A_PropertyDescription_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_PropertyDescription_Read)
    return false;
  const A_PropertyDescription_Read_PDU *
  a = (const A_PropertyDescription_Read_PDU *) req;
  if (a->object_index != object_index)
    return false;
  return true;
}

/* A_NetworkParameter_Read */

bool
A_NetworkParameter_Read_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 5)
    return false;

  parameter_type.set (c.data() + 2, 3);
  test_info.set (c.data() + 5, c.size() - 5);
  return true;
}

CArray
A_NetworkParameter_Read_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (5 + test_info.size());
  pdu[0] = A_NetworkParameter_Read >> 8;
  pdu[1] = A_NetworkParameter_Read & 0xff;
  pdu.setpart (parameter_type.data(), 3, 3);
  pdu.setpart (test_info.data(), 5, test_info.size());
  return pdu;
}

std::string
A_NetworkParameter_Read_PDU::Decode (TracePtr) const
{
  std::string s ("A_NetworkParameter_Read");
  // @todo
  return s;
}

bool A_NetworkParameter_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_NetworkParameter_Response */

bool
A_NetworkParameter_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 16)
    return false;

  parameter_type.set (c.data() + 2, 3);
  test_info_result.set (c.data() + 5, c.size() - 5);
  return true;
}

CArray
A_NetworkParameter_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (5 + test_info_result.size());
  pdu[0] = A_NetworkParameter_Response >> 8;
  pdu[1] = A_NetworkParameter_Response & 0xff;
  pdu.setpart (parameter_type.data(), 3, 3);
  pdu.setpart (test_info_result.data(), 5, test_info_result.size());
  return pdu;
}

std::string
A_NetworkParameter_Response_PDU::Decode (TracePtr) const
{
  std::string s ("A_NetworkParameter_Response");
  // @todo
  return s;
}

bool A_NetworkParameter_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_NetworkParameter_Read)
    return false;
  const A_NetworkParameter_Read_PDU *
  a = (const A_NetworkParameter_Read_PDU *) req;
  if (a->parameter_type != parameter_type)
    return false;
  // @todo compare test_info
  return true;
}

/* A_IndividualAddressSerialNumber_Read */

A_IndividualAddressSerialNumber_Read_PDU::
A_IndividualAddressSerialNumber_Read_PDU ()
{
  serial_number.fill(0);
}

bool
A_IndividualAddressSerialNumber_Read_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 8)
    return false;

  std::copy(c.begin() + 2, c.begin() + 8, serial_number.begin());
  return true;
}

CArray A_IndividualAddressSerialNumber_Read_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (8);
  pdu[0] = A_IndividualAddressSerialNumber_Read >> 8;
  pdu[1] = A_IndividualAddressSerialNumber_Read & 0xff;
  pdu.setpart (serial_number.data(), 2, 6);
  return pdu;
}

std::string A_IndividualAddressSerialNumber_Read_PDU::Decode (TracePtr) const
{
  std::string
  s ("A_IndividualAddressSerialNumber_Read ");
  addHex (s, serial_number[0]);
  addHex (s, serial_number[1]);
  addHex (s, serial_number[2]);
  addHex (s, serial_number[3]);
  addHex (s, serial_number[4]);
  addHex (s, serial_number[5]);
  return s;
}

bool
A_IndividualAddressSerialNumber_Read_PDU::isResponse (const APDU *)
const
{
  return false;
}

/* A_IndividualAddressSerialNumber_Response */

A_IndividualAddressSerialNumber_Response_PDU::
A_IndividualAddressSerialNumber_Response_PDU ()
{
  serial_number.fill(0);
}

bool
A_IndividualAddressSerialNumber_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 12)
    return false;

  std::copy(c.begin() + 2, c.begin() + 8, serial_number.begin());
  domain_address = (c[8] << 8) | (c[9]);
  reserved.set (c.data() + 10, 2);
  return true;
}

CArray A_IndividualAddressSerialNumber_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (12);
  pdu[0] = A_IndividualAddressSerialNumber_Response >> 8;
  pdu[1] = A_IndividualAddressSerialNumber_Response & 0xff;
  pdu.setpart (serial_number.data(), 2, 6);
  pdu[8] = domain_address >> 8;
  pdu[9] = domain_address & 0xff;
  pdu[10] = reserved[0];
  pdu[11] = reserved[1];
  return pdu;
}

std::string A_IndividualAddressSerialNumber_Response_PDU::Decode (TracePtr) const
{
  std::string
  s ("A_IndividualAddressSerialNumber_Response ");
  addHex (s, serial_number[0]);
  addHex (s, serial_number[1]);
  addHex (s, serial_number[2]);
  addHex (s, serial_number[3]);
  addHex (s, serial_number[4]);
  addHex (s, serial_number[5]);
  s += "Addr: ";
  add16Hex (s, domain_address);
  return s;
}

bool
A_IndividualAddressSerialNumber_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_IndividualAddressSerialNumber_Read)
    return false;
  const A_IndividualAddressSerialNumber_Read_PDU *a =
    (const A_IndividualAddressSerialNumber_Read_PDU *) req;
  if (a->serial_number != serial_number)
    return false;
  return true;
}

/* A_IndividualAddressSerialNumber_Write */

A_IndividualAddressSerialNumber_Write_PDU::
A_IndividualAddressSerialNumber_Write_PDU ()
{
  serial_number.fill(0);
}

bool A_IndividualAddressSerialNumber_Write_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 14)
    return false;

  std::copy(c.begin() + 2, c.begin() + 8, serial_number.begin());
  newaddress = (c[8] << 8) | (c[9]);
  reserved.set (c.data() + 10, 4);
  return true;
}

CArray A_IndividualAddressSerialNumber_Write_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (14);
  pdu[0] = A_IndividualAddressSerialNumber_Write >> 8;
  pdu[1] = A_IndividualAddressSerialNumber_Write & 0xff;
  pdu.setpart (serial_number.data(), 2, 6);
  pdu[8] = newaddress >> 8;
  pdu[9] = newaddress & 0xff;
  pdu[10] = reserved[0];
  pdu[11] = reserved[1];
  pdu[12] = reserved[2];
  pdu[13] = reserved[3];
  return pdu;
}

std::string A_IndividualAddressSerialNumber_Write_PDU::Decode (TracePtr) const
{
  std::string
  s ("A_IndividualAddressSerialNumber_Write ");
  addHex (s, serial_number[0]);
  addHex (s, serial_number[1]);
  addHex (s, serial_number[2]);
  addHex (s, serial_number[3]);
  addHex (s, serial_number[4]);
  addHex (s, serial_number[5]);
  s += "Addr: ";
  s += FormatEIBAddr (newaddress);
  return s;
}

bool
A_IndividualAddressSerialNumber_Write_PDU::isResponse (const APDU *)
const
{
  return false;
}

/* A_ServiceInformation_Indication_Write_PDU */

bool
A_ServiceInformation_Indication_Write_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 5)
    return false;

  verify_mode = (c[2] & 0x04) ? true : false;
  duplicate_address = (c[3] & 0x02) ? true : false;
  appl_stopped = (c[2] & 0x01) ? true : false;
  return true;
}

CArray
A_ServiceInformation_Indication_Write_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (5);
  pdu[0] = A_ServiceInformation_Indication_Write >> 8;
  pdu[1] = A_ServiceInformation_Indication_Write & 0xff;
  pdu[2] = (verify_mode << 2) | (duplicate_address << 1) | appl_stopped;
  pdu[3] = 0x00;
  pdu[4] = 0x00;
  return pdu;
}

std::string
A_ServiceInformation_Indication_Write_PDU::Decode (TracePtr) const
{
  std::string s ("A_ServiceInformation_Indication_Write ");
  if (verify_mode)
    s += "verify ";
  if (duplicate_address)
    s += "dupplicate_address ";
  if (appl_stopped)
    s += "appl_stopped ";

  return s;
}

bool
A_ServiceInformation_Indication_Write_PDU::isResponse (const APDU *)
const
{
  return false;
}

/* A_DomainAddress_Write */

bool
A_DomainAddress_Write_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 4)
    return false;

  domain_address = (c[2] << 8) | (c[3]);
  return true;
}

CArray A_DomainAddress_Write_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (4);
  pdu[0] = A_DomainAddress_Write >> 8;
  pdu[1] = A_DomainAddress_Write & 0xff;
  pdu[2] = domain_address >> 8;
  pdu[3] = domain_address & 0xff;
  return pdu;
}

std::string A_DomainAddress_Write_PDU::Decode (TracePtr) const
{
  std::string s ("A_DomainAddress_Write ");
  return s + FormatDomainAddr (domain_address);
}


bool A_DomainAddress_Write_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_DomainAddress_Read */

bool
A_DomainAddress_Read_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 2)
    return false;
  return true;
}

CArray A_DomainAddress_Read_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(2);
  pdu[0] = A_DomainAddress_Read >> 8;
  pdu[1] = A_DomainAddress_Read & 0xff;
  return pdu;
}

std::string A_DomainAddress_Read_PDU::Decode (TracePtr) const
{
  return "A_DomainAddress_Read";
}

bool A_DomainAddress_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_DomainAddress_Response */

bool
A_DomainAddress_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 4)
    return false;

  domain_address = (c[2] << 8) | c[3];
  return true;
}

CArray A_DomainAddress_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (4);
  pdu[0] = A_DomainAddress_Response >> 8;
  pdu[1] = A_DomainAddress_Response & 0xff;
  pdu[2] = domain_address >> 8;
  pdu[3] = domain_address & 0xff;
  return pdu;
}

std::string A_DomainAddress_Response_PDU::Decode (TracePtr) const
{
  std::string s ("A_DomainAddress_Response");
  s += FormatDomainAddr (domain_address);
  return s;
}

bool A_DomainAddress_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_DomainAddress_Read)
    return false;
  return true;
}

/* A_DomainAddressSelective_Read */

bool
A_DomainAddressSelective_Read_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 7)
    return false;

  domain_address = (c[2] << 8) | c[3];
  start_address = (c[4] << 8) | c[5];
  range = c[6];
  return true;
}

CArray A_DomainAddressSelective_Read_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (7);
  pdu[0] = A_DomainAddressSelective_Read >> 8;
  pdu[1] = A_DomainAddressSelective_Read & 0xff;
  pdu[2] = domain_address >> 8;
  pdu[3] = domain_address & 0xff;
  pdu[2] = start_address >> 8;
  pdu[3] = start_address & 0xff;
  pdu[6] = range;
  return pdu;
}

std::string
A_DomainAddressSelective_Read_PDU::Decode (TracePtr) const
{
  std::string s ("A_DomainAddressSelective_Read ");
  s += FormatDomainAddr (domain_address);
  s += " ";
  s += FormatEIBAddr (start_address);
  s += " ";
  addHex (s, range);
  return s;
}

bool A_DomainAddressSelective_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_NetworkParameter_Write */

bool
A_NetworkParameter_Write_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 5)
    return false;

  parameter_type.set (c.data() + 2, 3);
  value.set (c.data() + 5, c.size() - 5);
  return true;
}

CArray
A_NetworkParameter_Write_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize (5 + value.size());
  pdu[0] = A_NetworkParameter_Write >> 8;
  pdu[1] = A_NetworkParameter_Write & 0xff;
  pdu.setpart (parameter_type.data(), 3, 3);
  pdu.setpart (value.data(), 5, value.size());
  return pdu;
}

std::string
A_NetworkParameter_Write_PDU::Decode (TracePtr) const
{
  std::string s ("A_NetworkParameter_Write");
  // @todo
  return s;
}

bool A_NetworkParameter_Write_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_Link_Read */

bool
A_Link_Read_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 4)
    return false;

  group_object_number = c[2];
  start_index = c[3] & 0x0f;
  return true;
}

CArray
A_Link_Read_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(4);
  pdu[0] = A_Link_Read >> 8;
  pdu[1] = A_Link_Read & 0xff;
  pdu[2] = group_object_number;
  pdu[3] = start_index & 0x0f;
  return pdu;
}

std::string
A_Link_Read_PDU::Decode (TracePtr) const
{
  std::string s ("A_Link_Read");
  // @todo
  return s;
}

bool A_Link_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_Link_Response */

bool
A_Link_Response_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 4)
    return false;

  group_object_number = c[2];
  sending_address = c[3] >> 4;
  start_index = c[3] & 0x0f;
  group_address_list.set (c.data() + 4, c.size() - 4);
  return true;
}

CArray
A_Link_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(4 + group_address_list.size());
  pdu[0] = A_Link_Response >> 8;
  pdu[1] = A_Link_Response & 0xff;
  pdu[2] = group_object_number;
  pdu[3] = (sending_address << 4) | start_index;
  pdu.setpart (group_address_list.data(), 4, group_address_list.size());
  return pdu;
}

std::string
A_Link_Response_PDU::Decode (TracePtr) const
{
  std::string s ("A_Link_Response");
  // @todo
  return s;
}

bool A_Link_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_Link_Read)
    return false;
  const A_Link_Read_PDU * a =
    (const A_Link_Read_PDU *) req;
  if (a->group_object_number != group_object_number)
    return false;
  if (a->start_index != start_index)
    return false;
  return true;
}

/* A_Link_Write */

bool
A_Link_Write_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() != 6)
    return false;

  group_object_number = c[2];
  flags = c[3] & 0x03;
  group_address = (c[4] << 8) | c[5];
  return true;
}

CArray
A_Link_Write_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(6);
  pdu[0] = A_Link_Write >> 8;
  pdu[1] = A_Link_Write & 0xff;
  pdu[2] = group_object_number;
  pdu[3] = flags;
  pdu[4] = group_address >> 8;
  pdu[5] = group_address & 0xff;
  return pdu;
}

std::string
A_Link_Write_PDU::Decode (TracePtr) const
{
  std::string s ("A_Link_Write");
  // @todo
  return s;
}

bool A_Link_Write_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_GroupPropValue_Read */

bool
A_GroupPropValue_Read_PDU::init (const CArray &, TracePtr)
{
  // @todo
  return true;
}

CArray
A_GroupPropValue_Read_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(2);
  pdu[0] = A_GroupPropValue_Read >> 8;
  pdu[1] = A_GroupPropValue_Read & 0xff;
  // @todo
  return pdu;
}

std::string
A_GroupPropValue_Read_PDU::Decode (TracePtr) const
{
  std::string s ("A_GroupPropValue_Read");
  // @todo
  return s;
}

bool A_GroupPropValue_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_GroupPropValue_Response */

bool
A_GroupPropValue_Response_PDU::init (const CArray &, TracePtr)
{
  // @todo
  return true;
}

CArray
A_GroupPropValue_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(2);
  pdu[0] = A_GroupPropValue_Response >> 8;
  pdu[1] = A_GroupPropValue_Response & 0xff;
  // @todo
  return pdu;
}

std::string
A_GroupPropValue_Response_PDU::Decode (TracePtr) const
{
  std::string s ("A_GroupPropValue_Response");
  // @todo
  return s;
}

bool A_GroupPropValue_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_GroupPropValue_Read)
    return false;
  // @todo
  return true;
}

/* A_GroupPropValue_Write */

bool
A_GroupPropValue_Write_PDU::init (const CArray &, TracePtr)
{
  // @todo
  return true;
}

CArray
A_GroupPropValue_Write_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(2);
  pdu[0] = A_GroupPropValue_Write >> 8;
  pdu[1] = A_GroupPropValue_Write & 0xff;
  // @todo
  return pdu;
}

std::string
A_GroupPropValue_Write_PDU::Decode (TracePtr) const
{
  std::string s ("A_GroupPropValue_Write");
  // @todo
  return s;
}

bool A_GroupPropValue_Write_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_GroupPropValue_InfoReport */

bool
A_GroupPropValue_InfoReport_PDU::init (const CArray &, TracePtr)
{
  // @todo
  return true;
}

CArray
A_GroupPropValue_InfoReport_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(2);
  pdu[0] = A_GroupPropValue_InfoReport >> 8;
  pdu[1] = A_GroupPropValue_InfoReport & 0xff;
  // @todo
  return pdu;
}

std::string
A_GroupPropValue_InfoReport_PDU::Decode (TracePtr) const
{
  std::string s ("A_GroupPropValue_InfoReport");
  // @todo
  return s;
}

bool A_GroupPropValue_InfoReport_PDU::isResponse (const APDU *) const
{
  // @todo
  return false;
}

/* A_DomainAddressSerialNumber_Read */

bool
A_DomainAddressSerialNumber_Read_PDU::init (const CArray &, TracePtr)
{
  // @todo
  return true;
}

CArray
A_DomainAddressSerialNumber_Read_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(2);
  pdu[0] = A_DomainAddressSerialNumber_Read >> 8;
  pdu[1] = A_DomainAddressSerialNumber_Read & 0xff;
  // @todo
  return pdu;
}

std::string
A_DomainAddressSerialNumber_Read_PDU::Decode (TracePtr) const
{
  std::string s ("A_DomainAddressSerialNumber_Read");
  // @todo
  return s;
}

bool A_DomainAddressSerialNumber_Read_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_DomainAddressSerialNumber_Response */

bool
A_DomainAddressSerialNumber_Response_PDU::init (const CArray &, TracePtr)
{
  // @todo
  return true;
}

CArray
A_DomainAddressSerialNumber_Response_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(2);
  pdu[0] = A_DomainAddressSerialNumber_Response >> 8;
  pdu[1] = A_DomainAddressSerialNumber_Response & 0xff;
  // @todo
  return pdu;
}

std::string
A_DomainAddressSerialNumber_Response_PDU::Decode (TracePtr) const
{
  std::string s ("A_DomainAddressSerialNumber_Response");
  // @todo
  return s;
}

bool A_DomainAddressSerialNumber_Response_PDU::isResponse (const APDU * req) const
{
  if (req->getType () != A_DomainAddressSerialNumber_Read)
    return false;
  // @todo
  return true;
}

/* A_DomainAddressSerialNumber_Write */

bool
A_DomainAddressSerialNumber_Write_PDU::init (const CArray &, TracePtr)
{
  // @todo
  return true;
}

CArray
A_DomainAddressSerialNumber_Write_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(2);
  pdu[0] = A_DomainAddressSerialNumber_Write >> 8;
  pdu[1] = A_DomainAddressSerialNumber_Write & 0xff;
  // @todo
  return pdu;
}

std::string
A_DomainAddressSerialNumber_Write_PDU::Decode (TracePtr) const
{
  std::string s ("A_DomainAddressSerialNumber_Write");
  // @todo
  return s;
}

bool A_DomainAddressSerialNumber_Write_PDU::isResponse (const APDU *) const
{
  return false;
}

/* A_FileStream_InfoReport */

bool
A_FileStream_InfoReport_PDU::init (const CArray & c, TracePtr)
{
  if (c.size() < 3)
    return false;

  file_handle = c[2] >> 4;
  file_block_sequence_number = c[2] & 0x0f;
  file_block.set (c.data() + 3, c.size() - 3);
  return true;
}

CArray
A_FileStream_InfoReport_PDU::ToPacket () const
{
  CArray pdu;
  pdu.resize(3 + file_block.size());
  pdu[0] = A_FileStream_InfoReport >> 8;
  pdu[1] = A_FileStream_InfoReport & 0xff;
  pdu[2] = (file_handle << 4) | file_block_sequence_number;
  pdu.setpart (file_block.data(), 3, file_block.size());
  return pdu;
}

std::string
A_FileStream_InfoReport_PDU::Decode (TracePtr) const
{
  std::string s ("A_FileStream_InfoReport");
  // @todo
  return s;
}

bool A_FileStream_InfoReport_PDU::isResponse (const APDU *) const
{
  // @todo
  return false;
}
