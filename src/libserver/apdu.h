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

/**
 * @file
 * @ingroup KNX_03_03_07
 * Application Layer
 * @{
 */

#ifndef APDU_H
#define APDU_H

#include <array>

#include "common.h"

/** enumeration of APDU types */
enum APDU_type : uint16_t
{
  /** unknown APDU */
  A_Unknown = 0xFFFF, // A_GroupValue_Read is also 0
  A_GroupValue_Read = 0x000,
  A_GroupValue_Response = 0x040, // .. 0x07F
  A_GroupValue_Write = 0x080, // .. 0x0BF
  A_IndividualAddress_Write = 0x0C0,
  A_IndividualAddress_Read = 0x100,
  A_IndividualAddress_Response = 0x140,
  A_ADC_Read = 0x180, // .. 0x1BF
  A_ADC_Response = 0x1C0, // .. 0x1FF
  A_SystemNetworkParameter_Read = 0x1C8,
  A_SystemNetworkParameter_Response = 0x1C9,
  A_SystemNetworkParameter_Write = 0x1CA,
  // 0x1CB is planned for future system broadcast service
  A_Memory_Read = 0x200, // .. 0x20F
  A_Memory_Response = 0x240, // .. 0x24F
  A_Memory_Write = 0x280, // .. 0x028F
  A_UserMemory_Read = 0x2C0,
  A_UserMemory_Response = 0x2C1,
  A_UserMemory_Write = 0x2C2,
  A_UserMemoryBit_Write = 0x2C4,
  A_UserManufacturerInfo_Read = 0x2C5,
  A_UserManufacturerInfo_Response = 0x2C6,
  A_FunctionPropertyCommand = 0x2C7,
  A_FunctionPropertyState_Read = 0x2C8,
  A_FunctionPropertyState_Response = 0x2C9,
  // 0x2CA .. 0x2F7 are reserved USERMSG
  // 0x2F8 .. 0x2FE are manufacturer specific area for USERMSG
  A_DeviceDescriptor_Read = 0x300, // .. 0x33F
  A_DeviceDescriptor_Response = 0x340, // .. 0x37F // = A_DeviceDescriptor_InfoReport @todo
  A_Restart = 0x380, // .. 0x39F
  A_Restart_Response = 0x3A0, // .. 0x3BF
  /* Coupler specific services */
  A_Open_Routing_Table_Request = 0x3C0,
  A_Read_Routing_Table_Request = 0x3C1,
  A_Read_Routing_Table_Response = 0x3C2,
  A_Write_Routing_Table_Request = 0x3C3,
  A_Read_Router_Memory_Request = 0x3C8,
  A_Read_Router_Memory_Response = 0x3C9,
  A_Write_Router_Memory_Request = 0x3CA,
  A_Read_Router_Status_Request = 0x3CD,
  A_Read_Router_Status_Response = 0x3CE,
  A_Write_Router_Status_Request = 0x3CF,
  A_MemoryBit_Write = 0x3D0,
  A_Authorize_Request = 0x3D1,
  A_Authorize_Response = 0x3D2,
  A_Key_Write = 0x3D3,
  A_Key_Response = 0x3D4,
  A_PropertyValue_Read = 0x3D5,
  A_PropertyValue_Response = 0x3D6,
  A_PropertyValue_Write = 0x3D7,
  A_PropertyDescription_Read = 0x3D8,
  A_PropertyDescription_Response = 0x3D9,
  A_NetworkParameter_Read = 0x3DA,
  A_NetworkParameter_Response = 0x3DB, // = A_NetworkParameter_InfoReport @todo
  A_IndividualAddressSerialNumber_Read = 0x3DC,
  A_IndividualAddressSerialNumber_Response = 0x3DD,
  A_IndividualAddressSerialNumber_Write = 0x3DE,
  A_ServiceInformation_Indication_Write = 0x3DF,
  /* Open media specific services */
  A_DomainAddress_Write = 0x3E0,
  A_DomainAddress_Read = 0x3E1,
  A_DomainAddress_Response = 0x3E2,
  A_DomainAddressSelective_Read = 0x3E3,
  A_NetworkParameter_Write = 0x3E4,
  A_Link_Read = 0x3E5,
  A_Link_Response = 0x3E6,
  A_Link_Write = 0x3E7,
  A_GroupPropValue_Read = 0x3E8,
  A_GroupPropValue_Response = 0x3E9,
  A_GroupPropValue_Write = 0x3EA,
  A_GroupPropValue_InfoReport = 0x3EB,
  A_DomainAddressSerialNumber_Read = 0x3EC,
  A_DomainAddressSerialNumber_Response = 0x3ED,
  A_DomainAddressSerialNumber_Write = 0x3EE,
  A_FileStream_InfoReport = 0x3F0,
};

class APDU;
using APDUPtr = std::unique_ptr<APDU>;

/** represents an APDU */
class APDU
{
public:
  virtual ~APDU () = default;

  virtual bool init (const CArray &, TracePtr tr) = 0;
  /** convert to character array */
  virtual CArray ToPacket () const = 0;
  /** decode content as string */
  virtual std::string Decode (TracePtr tr) const = 0;

  /** converts character array to a APDU */
  static APDUPtr fromPacket (const CArray &, TracePtr tr);
  /** gets APDU type */
  virtual APDU_type getType () const = 0;
  /** returns true, if this is can be an answer of req */
  virtual bool isResponse (const APDU * req) const = 0;
};

class A_Unknown_PDU:public APDU
{
public:
  CArray pdu;

  A_Unknown_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Unknown;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_GroupValue_Read_PDU:public APDU
{
public:
  A_GroupValue_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_GroupValue_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_GroupValue_Response_PDU:public APDU
{
public:
  CArray data;

  A_GroupValue_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_GroupValue_Response;
  }
  virtual bool isResponse (const APDU * req) const override;

private:
  bool issmall = false;
};

class A_GroupValue_Write_PDU:public APDU
{
public:
  CArray data;

  A_GroupValue_Write_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_GroupValue_Write;
  }
  virtual bool isResponse (const APDU * req) const override;

private:
  bool issmall = false;
};

class A_IndividualAddress_Write_PDU:public APDU
{
public:
  eibaddr_t newaddress = 0;

  A_IndividualAddress_Write_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_IndividualAddress_Write;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_IndividualAddress_Read_PDU:public APDU
{
public:

  A_IndividualAddress_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_IndividualAddress_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_IndividualAddress_Response_PDU:public APDU
{
public:

  A_IndividualAddress_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_IndividualAddress_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_ADC_Read_PDU:public APDU
{
public:
  uint8_t channel_nr = 0;
  uint8_t read_count = 0;

  A_ADC_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_ADC_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_ADC_Response_PDU:public APDU
{
public:
  uint8_t channel_nr = 0;
  uint8_t read_count = 0;
  int16_t sum = 0;

  A_ADC_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_ADC_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_SystemNetworkParameter_Read_PDU:public APDU
{
public:
  // @todo

  A_SystemNetworkParameter_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_SystemNetworkParameter_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_SystemNetworkParameter_Response_PDU:public APDU
{
public:
  // @todo

  A_SystemNetworkParameter_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_SystemNetworkParameter_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_SystemNetworkParameter_Write_PDU:public APDU
{
public:
  // @todo

  A_SystemNetworkParameter_Write_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_SystemNetworkParameter_Write;
  }
  virtual bool isResponse (const APDU * req) const override;
};

/** memory address type */
using memaddr_t = uint16_t;

class A_Memory_Read_PDU:public APDU
{
public:
  uint8_t number = 0;
  memaddr_t address = 0;

  A_Memory_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Memory_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Memory_Response_PDU:public APDU
{
public:
  uint8_t number = 0;
  memaddr_t address = 0;
  CArray data;

  A_Memory_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Memory_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Memory_Write_PDU:public APDU
{
public:
  uint8_t number = 0;
  memaddr_t address = 0;
  CArray data;

  A_Memory_Write_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Memory_Write;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_UserMemory_Read_PDU:public APDU
{
public:
  uint8_t address_extension = 0;
  uint8_t number = 0;
  memaddr_t address = 0;

  A_UserMemory_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_UserMemory_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_UserMemory_Response_PDU:public APDU
{
public:
  uint8_t address_extension = 0;
  uint8_t number = 0;
  memaddr_t address = 0;
  CArray data;

  A_UserMemory_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_UserMemory_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_UserMemory_Write_PDU:public APDU
{
public:
  uint8_t address_extension = 0;
  uint8_t number = 0;
  memaddr_t address = 0;
  CArray data;

  A_UserMemory_Write_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_UserMemory_Write;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_UserMemoryBit_Write_PDU:public APDU
{
public:
  uint8_t number = 0;
  memaddr_t address = 0;
  CArray and_data;
  CArray xor_data;

  A_UserMemoryBit_Write_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_UserMemoryBit_Write;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_UserManufacturerInfo_Read_PDU:public APDU
{
public:

  A_UserManufacturerInfo_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_UserManufacturerInfo_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_UserManufacturerInfo_Response_PDU:public APDU
{
public:
  uint8_t manufacturer_id = 0;
  uint16_t manufacturer_data = 0;

  A_UserManufacturerInfo_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_UserManufacturerInfo_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_FunctionPropertyCommand_PDU:public APDU
{
public:
  uint8_t object_index = 0;
  uint8_t property_id = 0;
  CArray data;

  A_FunctionPropertyCommand_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_FunctionPropertyCommand;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_FunctionPropertyState_Read_PDU:public APDU
{
public:
  uint8_t object_index;
  uint8_t property_id;
  CArray data;

  A_FunctionPropertyState_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_FunctionPropertyState_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_FunctionPropertyState_Response_PDU:public APDU
{
public:
  uint8_t object_index;
  uint8_t property_id;
  uint8_t return_code;
  CArray data;

  A_FunctionPropertyState_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_FunctionPropertyState_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_DeviceDescriptor_Read_PDU:public APDU
{
public:
  uint8_t descriptor_type = 0;

  A_DeviceDescriptor_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_DeviceDescriptor_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_DeviceDescriptor_Response_PDU:public APDU
{
public:
  uint8_t descriptor_type = 0;
  uint16_t device_descriptor = 0;

  A_DeviceDescriptor_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_DeviceDescriptor_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

// @todo A_DeviceDescriptor_InfoReport

class A_Restart_PDU:public APDU
{
public:
  uint8_t restart_type = 0;
  uint8_t erase_code = 0;
  uint8_t channel_number = 0;

  A_Restart_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Restart;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Restart_Response_PDU:public APDU
{
public:
  uint8_t restart_type = 0;
  uint8_t error_code = 0;
  uint16_t process_time = 0;

  A_Restart_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Restart;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Open_Routing_Table_Request_PDU:public APDU
{
public:
  CArray data;

  A_Open_Routing_Table_Request_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Open_Routing_Table_Request;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Read_Routing_Table_Request_PDU:public APDU
{
public:
  CArray data;

  A_Read_Routing_Table_Request_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Read_Routing_Table_Request;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Read_Routing_Table_Response_PDU:public APDU
{
public:
  CArray data;

  A_Read_Routing_Table_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Read_Routing_Table_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Write_Routing_Table_Request_PDU:public APDU
{
public:
  CArray data;

  A_Write_Routing_Table_Request_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Write_Routing_Table_Request;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Read_Router_Memory_Request_PDU:public APDU
{
public:
  CArray data;

  A_Read_Router_Memory_Request_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Read_Router_Memory_Request;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Read_Router_Memory_Response_PDU:public APDU
{
public:
  CArray data;

  A_Read_Router_Memory_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Read_Router_Memory_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Write_Router_Memory_Request_PDU:public APDU
{
public:
  CArray data;

  A_Write_Router_Memory_Request_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Write_Router_Memory_Request;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Read_Router_Status_Request_PDU:public APDU
{
public:
  CArray data;

  A_Read_Router_Status_Request_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Read_Router_Status_Request;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Read_Router_Status_Response_PDU:public APDU
{
public:
  CArray data;

  A_Read_Router_Status_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Read_Router_Status_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Write_Router_Status_Request_PDU:public APDU
{
public:
  CArray data;

  A_Write_Router_Status_Request_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Write_Router_Status_Request;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_MemoryBit_Write_PDU:public APDU
{
public:
  uint8_t number = 0;
  memaddr_t address = 0;
  CArray and_data;
  CArray xor_data;

  A_MemoryBit_Write_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_MemoryBit_Write;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Authorize_Request_PDU:public APDU
{
public:
  eibkey_type key = 0;

  A_Authorize_Request_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Authorize_Request;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Authorize_Response_PDU:public APDU
{
public:
  uint8_t level = 0;

  A_Authorize_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Authorize_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Key_Write_PDU:public APDU
{
public:
  uint8_t level = 0;
  eibkey_type key = 0;

  A_Key_Write_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Key_Write;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Key_Response_PDU:public APDU
{
public:
  uint8_t level = 0;

  A_Key_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Key_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

/** object index type */
using objectno_t = uint8_t;

/** property ID type */
using propertyid_t = uint8_t;

class A_PropertyValue_Read_PDU:public APDU
{
public:
  objectno_t object_index = 0;
  propertyid_t property_id = 0;
  uint8_t nr_of_elem = 0;
  uint16_t start_index = 0;

  A_PropertyValue_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_PropertyValue_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_PropertyValue_Response_PDU:public APDU
{
public:
  objectno_t object_index = 0;
  propertyid_t property_id = 0;
  uint8_t nr_of_elem = 0;
  uint16_t start_index = 0;
  CArray data;

  A_PropertyValue_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_PropertyValue_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_PropertyValue_Write_PDU:public APDU
{
public:
  objectno_t object_index = 0;
  propertyid_t property_id = 0;
  uint8_t nr_of_elem = 0;
  uint16_t start_index = 0;
  CArray data;

  A_PropertyValue_Write_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_PropertyValue_Write;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_PropertyDescription_Read_PDU:public APDU
{
public:
  objectno_t object_index = 0;
  propertyid_t property_id = 0;
  uint8_t property_index = 0;

  A_PropertyDescription_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_PropertyDescription_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_PropertyDescription_Response_PDU:public APDU
{
public:
  objectno_t object_index = 0;
  propertyid_t property_id = 0;
  uint8_t property_index = 0;
  bool write_enable = false;
  uint8_t type = 0;
  uint16_t max_nr_of_elem = 0;
  uint8_t access = 0;

  A_PropertyDescription_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_PropertyDescription_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_NetworkParameter_Read_PDU:public APDU
{
public:
  CArray parameter_type;
  CArray test_info;

  A_NetworkParameter_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_NetworkParameter_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_NetworkParameter_Response_PDU:public APDU
{
public:
  CArray parameter_type;
  CArray test_info_result; // @todo unclear where test_info ends and test_result begins

  A_NetworkParameter_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_NetworkParameter_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

// @todo A_NetworkParameter_InfoReport_PDU

/** serial number */
using serialnumber_t = std::array<uint8_t, 6>;

class A_IndividualAddressSerialNumber_Read_PDU:public APDU
{
public:
  serialnumber_t serial_number;

  A_IndividualAddressSerialNumber_Read_PDU ();
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_IndividualAddressSerialNumber_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_IndividualAddressSerialNumber_Response_PDU:public APDU
{
public:
  serialnumber_t serial_number;
  domainaddr_t domain_address = 0;
  CArray reserved;

  A_IndividualAddressSerialNumber_Response_PDU ();
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_IndividualAddressSerialNumber_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_IndividualAddressSerialNumber_Write_PDU:public APDU
{
public:
  serialnumber_t serial_number;
  eibaddr_t newaddress = 0;
  CArray reserved;

  A_IndividualAddressSerialNumber_Write_PDU ();
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_IndividualAddressSerialNumber_Write;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_ServiceInformation_Indication_Write_PDU:public APDU
{
public:
  bool verify_mode = false;
  bool duplicate_address = false;
  bool appl_stopped = false;

  A_ServiceInformation_Indication_Write_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_ServiceInformation_Indication_Write;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_DomainAddress_Write_PDU:public APDU
{
public:
  domainaddr_t domain_address = 0;

  A_DomainAddress_Write_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_DomainAddress_Write;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_DomainAddress_Read_PDU:public APDU
{
public:

  A_DomainAddress_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_DomainAddress_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_DomainAddress_Response_PDU:public APDU
{
public:
  domainaddr_t domain_address = 0;

  A_DomainAddress_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_DomainAddress_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_DomainAddressSelective_Read_PDU:public APDU
{
public:
  domainaddr_t domain_address = 0;
  eibaddr_t start_address = 0;
  uint8_t range = 0;

  A_DomainAddressSelective_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_DomainAddressSelective_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_NetworkParameter_Write_PDU:public APDU
{
public:
  CArray parameter_type;
  CArray value;

  A_NetworkParameter_Write_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_NetworkParameter_Write;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Link_Read_PDU:public APDU
{
public:
  uint8_t group_object_number = 0;
  uint8_t start_index = 0;

  A_Link_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Link_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Link_Response_PDU:public APDU
{
public:
  uint8_t group_object_number = 0;
  uint8_t sending_address = 0;
  uint8_t start_index = 0;
  CArray group_address_list;

  A_Link_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Link_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_Link_Write_PDU:public APDU
{
public:
  uint8_t group_object_number = 0;
  uint8_t flags = 0;
  eibaddr_t group_address = 0;

  A_Link_Write_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_Link_Write;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_GroupPropValue_Read_PDU:public APDU
{
public:
  // @todo

  A_GroupPropValue_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_GroupPropValue_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_GroupPropValue_Response_PDU:public APDU
{
public:
  // @todo

  A_GroupPropValue_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_GroupPropValue_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_GroupPropValue_Write_PDU:public APDU
{
public:
  // @todo

  A_GroupPropValue_Write_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_GroupPropValue_Write;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_GroupPropValue_InfoReport_PDU:public APDU
{
public:
  // @todo

  A_GroupPropValue_InfoReport_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_GroupPropValue_InfoReport;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_DomainAddressSerialNumber_Read_PDU:public APDU
{
public:
  serialnumber_t serial_number;

  A_DomainAddressSerialNumber_Read_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_DomainAddressSerialNumber_Read;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_DomainAddressSerialNumber_Response_PDU:public APDU
{
public:
  serialnumber_t serial_number;
  domainaddr_t domain_address = 0;

  A_DomainAddressSerialNumber_Response_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_DomainAddressSerialNumber_Response;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_DomainAddressSerialNumber_Write_PDU:public APDU
{
public:
  serialnumber_t serial_number;
  domainaddr_t domain_address = 0;

  A_DomainAddressSerialNumber_Write_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_DomainAddressSerialNumber_Write;
  }
  virtual bool isResponse (const APDU * req) const override;
};

class A_FileStream_InfoReport_PDU:public APDU
{
public:
  uint8_t file_handle = 0;
  uint8_t file_block_sequence_number = 0;
  CArray file_block;

  A_FileStream_InfoReport_PDU () = default;
  virtual bool init (const CArray & p, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual APDU_type getType () const override
  {
    return A_FileStream_InfoReport;
  }
  virtual bool isResponse (const APDU * req) const override;
};

#endif

/** @} */
