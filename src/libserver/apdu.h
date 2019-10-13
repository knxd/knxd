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

#ifndef APDU_H
#define APDU_H

#include "common.h"

/** enumeration of APDU types */
typedef enum
{
  /** unknown APDU */
  A_Unknown,
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
  A_DeviceDescriptor_Read = 0x300,
  A_DeviceDescriptor_Response = 0x340,
  A_Restart = 0x380,
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
  A_NetworkParameter_Response = 0x3DB,
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
  A_GroupValue_InfoReport = 0x3EB,
  A_DomainAddressSerialNumber_Read = 0x3EC,
  A_DomainAddressSerialNumber_Response = 0x3ED,
  A_DomainAddressSerialNumber_Write = 0x3EE,
  A_FileStream_InfoReport = 0x3F0,
}
APDU_type;

class APDU;
typedef std::unique_ptr<APDU> APDUPtr;

/** represents an APDU */
class APDU
{
public:
  virtual ~APDU () = default;

  virtual bool init (const CArray &, TracePtr ) = 0;
  /** convert to character array */
  virtual CArray ToPacket () = 0;
  /** decode content as string */
  virtual std::string Decode (TracePtr t) = 0;

  /** converts character array to a APDU */
  static APDUPtr fromPacket (const CArray &, TracePtr t);
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
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Unknown;
  }
  bool isResponse (const APDU * req) const;
};

class A_GroupValue_Read_PDU:public APDU
{
public:

  A_GroupValue_Read_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_GroupValue_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_GroupValue_Response_PDU:public APDU
{
public:
  bool issmall = 0;
  CArray data;

  A_GroupValue_Response_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_GroupValue_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_GroupValue_Write_PDU:public APDU
{
public:
  bool issmall = 0;
  CArray data;

  A_GroupValue_Write_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_GroupValue_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_IndividualAddress_Read_PDU:public APDU
{
public:

  A_IndividualAddress_Read_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_IndividualAddress_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_IndividualAddress_Response_PDU:public APDU
{
public:

  A_IndividualAddress_Response_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_IndividualAddress_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_IndividualAddress_Write_PDU:public APDU
{
public:
  eibaddr_t addr = 0;

  A_IndividualAddress_Write_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_IndividualAddress_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_IndividualAddressSerialNumber_Read_PDU:public APDU
{
public:
  serialnumber_t serno;

  A_IndividualAddressSerialNumber_Read_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_IndividualAddressSerialNumber_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_IndividualAddressSerialNumber_Response_PDU:public APDU
{
public:
  serialnumber_t serno;
  domainaddr_t addr = 0;

  A_IndividualAddressSerialNumber_Response_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_IndividualAddressSerialNumber_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_IndividualAddressSerialNumber_Write_PDU:public APDU
{
public:
  serialnumber_t serno;
  eibaddr_t addr = 0;

  A_IndividualAddressSerialNumber_Write_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_IndividualAddressSerialNumber_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_ServiceInformation_Indication_Write_PDU:public APDU
{
public:
  bool verify_mode = 0;
  bool duplicate_address = 0;
  bool appl_stopped = 0;

  A_ServiceInformation_Indication_Write_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_ServiceInformation_Indication_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_DomainAddress_Write_PDU:public APDU
{
public:
  domainaddr_t addr = 0;

  A_DomainAddress_Write_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_DomainAddress_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_DomainAddress_Read_PDU:public APDU
{
public:

  A_DomainAddress_Read_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_DomainAddress_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_DomainAddress_Response_PDU:public APDU
{
public:
  domainaddr_t addr = 0;

  A_DomainAddress_Response_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_DomainAddress_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_DomainAddressSelective_Read_PDU:public APDU
{
public:
  domainaddr_t domainaddr = 0;
  eibaddr_t addr = 0;
  uchar range = 0;

    A_DomainAddressSelective_Read_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_DomainAddressSelective_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_PropertyValue_Read_PDU:public APDU
{
public:
  objectno_t obj = 0;
  propertyid_t prop = 0;
  uchar count = 0;
  uint16_t start = 0;

  A_PropertyValue_Read_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_PropertyValue_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_PropertyValue_Response_PDU:public APDU
{
public:
  objectno_t obj = 0;
  propertyid_t prop = 0;
  uchar count = 0;
  uint16_t start = 0;
  CArray data;

  A_PropertyValue_Response_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_PropertyValue_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_PropertyValue_Write_PDU:public APDU
{
public:
  objectno_t obj = 0;
  propertyid_t prop = 0;
  uchar count = 0;
  uint16_t start = 0;
  CArray data;

  A_PropertyValue_Write_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_PropertyValue_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_PropertyDescription_Read_PDU:public APDU
{
public:
  objectno_t obj = 0;
  propertyid_t prop = 0;
  uchar property_index = 0;

  A_PropertyDescription_Read_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_PropertyDescription_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_PropertyDescription_Response_PDU:public APDU
{
public:
  objectno_t obj = 0;
  propertyid_t prop = 0;
  uchar property_index = 0;
  uchar type = 0;
  uint16_t count = 0;
  uchar access = 0;

  A_PropertyDescription_Response_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_PropertyDescription_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_DeviceDescriptor_Read_PDU:public APDU
{
public:
  uchar type = 0;

  A_DeviceDescriptor_Read_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_DeviceDescriptor_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_DeviceDescriptor_Response_PDU:public APDU
{
public:
  uchar type = 0;
  uint16_t descriptor = 0;

  A_DeviceDescriptor_Response_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_DeviceDescriptor_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_ADC_Read_PDU:public APDU
{
public:
  uchar channel = 0;
  uchar count = 0;

  A_ADC_Read_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_ADC_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_ADC_Response_PDU:public APDU
{
public:
  uchar channel = 0;
  uchar count = 0;
  int16_t val = 0;

  A_ADC_Response_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_ADC_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_Memory_Read_PDU:public APDU
{
public:
  uchar count = 0;
  memaddr_t addr = 0;

  A_Memory_Read_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Memory_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_Memory_Response_PDU:public APDU
{
public:
  uchar count = 0;
  memaddr_t addr = 0;
  CArray data;

  A_Memory_Response_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Memory_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_Memory_Write_PDU:public APDU
{
public:
  uchar count = 0;
  memaddr_t addr = 0;
  CArray data;

  A_Memory_Write_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Memory_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_MemoryBit_Write_PDU:public APDU
{
public:
  uchar count = 0;
  memaddr_t addr = 0;
  CArray andmask;
  CArray xormask;

  A_MemoryBit_Write_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_MemoryBit_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_UserMemory_Read_PDU:public APDU
{
public:
  uchar addr_extension = 0;
  uchar count = 0;
  memaddr_t addr = 0;

  A_UserMemory_Read_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_UserMemory_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_UserMemory_Response_PDU:public APDU
{
public:
  uchar addr_extension = 0;
  uchar count = 0;
  memaddr_t addr = 0;
  CArray data;

  A_UserMemory_Response_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_UserMemory_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_UserMemory_Write_PDU:public APDU
{
public:
  uchar addr_extension = 0;
  uchar count = 0;
  memaddr_t addr = 0;
  CArray data;

  A_UserMemory_Write_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_UserMemory_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_UserMemoryBit_Write_PDU:public APDU
{
public:
  uchar addr_extension = 0;
  uchar count = 0;
  memaddr_t addr = 0;
  CArray andmask;
  CArray xormask;

  A_UserMemoryBit_Write_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_UserMemoryBit_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_UserManufacturerInfo_Read_PDU:public APDU
{
public:

  A_UserManufacturerInfo_Read_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_UserManufacturerInfo_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_UserManufacturerInfo_Response_PDU:public APDU
{
public:
  uchar manufacturerid = 0;
  uint16_t data = 0;

  A_UserManufacturerInfo_Response_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_UserManufacturerInfo_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_Restart_PDU:public APDU
{
public:

  A_Restart_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Restart;
  }
  bool isResponse (const APDU * req) const;
};

class A_Authorize_Request_PDU:public APDU
{
public:
  eibkey_type key = 0;

  A_Authorize_Request_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Authorize_Request;
  }
  bool isResponse (const APDU * req) const;
};

class A_Authorize_Response_PDU:public APDU
{
public:
  uchar level = 0;

  A_Authorize_Response_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Authorize_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_Key_Write_PDU:public APDU
{
public:
  uchar level = 0;
  eibkey_type key = 0;

  A_Key_Write_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Key_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_Key_Response_PDU:public APDU
{
public:
  uchar level = 0;

  A_Key_Response_PDU () = default;
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  std::string Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Key_Response;
  }
  bool isResponse (const APDU * req) const;
};

#endif
