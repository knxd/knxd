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
  A_GroupValue_Read,
  A_GroupValue_Response,
  A_GroupValue_Write,
  A_IndividualAddress_Read,
  A_IndividualAddress_Response,
  A_IndividualAddress_Write,
  A_IndividualAddressSerialNumber_Read,
  A_IndividualAddressSerialNumber_Response,
  A_IndividualAddressSerialNumber_Write,
  A_ServiceInformation_Indication_Write,
  A_DomainAddress_Write,
  A_DomainAddress_Read,
  A_DomainAddress_Response,
  A_DomainAddressSelective_Read,
  A_PropertyValue_Read,
  A_PropertyValue_Response,
  A_PropertyValue_Write,
  A_PropertyDescription_Read,
  A_PropertyDescription_Response,
  A_DeviceDescriptor_Read,
  A_DeviceDescriptor_Response,
  A_ADC_Read,
  A_ADC_Response,
  A_Memory_Read,
  A_Memory_Response,
  A_Memory_Write,
  A_MemoryBit_Write,
  A_UserMemory_Read,
  A_UserMemory_Response,
  A_UserMemory_Write,
  A_UserMemoryBit_Write,
  A_UserManufacturerInfo_Read,
  A_UserManufacturerInfo_Response,
  A_Restart,
  A_Authorize_Request,
  A_Authorize_Response,
  A_Key_Write,
  A_Key_Response,
}
APDU_type;

class APDU;
typedef std::unique_ptr<APDU> APDUPtr;

/** represents a TPDU */
class APDU
{
public:
  virtual ~APDU ()
  {
  };

  virtual bool init (const CArray &, TracePtr ) = 0;
  /** convert to character array */
  virtual CArray ToPacket () = 0;
  /** decode content as string */
  virtual String Decode (TracePtr t) = 0;

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

  A_Unknown_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Unknown;
  }
  bool isResponse (const APDU * req) const;
};

class A_GroupValue_Read_PDU:public APDU
{
public:

  A_GroupValue_Read_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_GroupValue_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_GroupValue_Response_PDU:public APDU
{
public:
  bool issmall;
  CArray data;

  A_GroupValue_Response_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_GroupValue_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_GroupValue_Write_PDU:public APDU
{
public:
  bool issmall;
  CArray data;

  A_GroupValue_Write_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_GroupValue_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_IndividualAddress_Read_PDU:public APDU
{
public:

  A_IndividualAddress_Read_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_IndividualAddress_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_IndividualAddress_Response_PDU:public APDU
{
public:

  A_IndividualAddress_Response_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_IndividualAddress_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_IndividualAddress_Write_PDU:public APDU
{
public:
  eibaddr_t addr;

  A_IndividualAddress_Write_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
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
  String Decode (TracePtr t);
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
  domainaddr_t addr;

  A_IndividualAddressSerialNumber_Response_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
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
  eibaddr_t addr;

  A_IndividualAddressSerialNumber_Write_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_IndividualAddressSerialNumber_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_ServiceInformation_Indication_Write_PDU:public APDU
{
public:
  bool verify_mode;
  bool duplicate_address;
  bool appl_stopped;

  A_ServiceInformation_Indication_Write_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_ServiceInformation_Indication_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_DomainAddress_Write_PDU:public APDU
{
public:
  domainaddr_t addr;

  A_DomainAddress_Write_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_DomainAddress_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_DomainAddress_Read_PDU:public APDU
{
public:

  A_DomainAddress_Read_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_DomainAddress_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_DomainAddress_Response_PDU:public APDU
{
public:
  domainaddr_t addr;

  A_DomainAddress_Response_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_DomainAddress_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_DomainAddressSelective_Read_PDU:public APDU
{
public:
  domainaddr_t domainaddr;
  eibaddr_t addr;
  uchar range;

    A_DomainAddressSelective_Read_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_DomainAddressSelective_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_PropertyValue_Read_PDU:public APDU
{
public:
  objectno_t obj;
  propertyid_t prop;
  uchar count;
  uint16_t start;

  A_PropertyValue_Read_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_PropertyValue_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_PropertyValue_Response_PDU:public APDU
{
public:
  objectno_t obj;
  propertyid_t prop;
  uchar count;
  uint16_t start;
  CArray data;

  A_PropertyValue_Response_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_PropertyValue_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_PropertyValue_Write_PDU:public APDU
{
public:
  objectno_t obj;
  propertyid_t prop;
  uchar count;
  uint16_t start;
  CArray data;

  A_PropertyValue_Write_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_PropertyValue_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_PropertyDescription_Read_PDU:public APDU
{
public:
  objectno_t obj;
  propertyid_t prop;
  uchar property_index;

  A_PropertyDescription_Read_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_PropertyDescription_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_PropertyDescription_Response_PDU:public APDU
{
public:
  objectno_t obj;
  propertyid_t prop;
  uchar property_index;
  uchar type;
  uint16_t count;
  uchar access;

  A_PropertyDescription_Response_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_PropertyDescription_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_DeviceDescriptor_Read_PDU:public APDU
{
public:
  uchar type;

  A_DeviceDescriptor_Read_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_DeviceDescriptor_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_DeviceDescriptor_Response_PDU:public APDU
{
public:
  uchar type;
  uint16_t descriptor;

  A_DeviceDescriptor_Response_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_DeviceDescriptor_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_ADC_Read_PDU:public APDU
{
public:
  uchar channel;
  uchar count;

  A_ADC_Read_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_ADC_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_ADC_Response_PDU:public APDU
{
public:
  uchar channel;
  uchar count;
  int16_t val;

  A_ADC_Response_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_ADC_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_Memory_Read_PDU:public APDU
{
public:
  uchar count;
  memaddr_t addr;

  A_Memory_Read_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Memory_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_Memory_Response_PDU:public APDU
{
public:
  uchar count;
  memaddr_t addr;
  CArray data;

  A_Memory_Response_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Memory_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_Memory_Write_PDU:public APDU
{
public:
  uchar count;
  memaddr_t addr;
  CArray data;

  A_Memory_Write_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Memory_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_MemoryBit_Write_PDU:public APDU
{
public:
  uchar count;
  memaddr_t addr;
  CArray andmask;
  CArray xormask;

  A_MemoryBit_Write_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_MemoryBit_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_UserMemory_Read_PDU:public APDU
{
public:
  uchar addr_extension;
  uchar count;
  memaddr_t addr;

  A_UserMemory_Read_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_UserMemory_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_UserMemory_Response_PDU:public APDU
{
public:
  uchar addr_extension;
  uchar count;
  memaddr_t addr;
  CArray data;

  A_UserMemory_Response_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_UserMemory_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_UserMemory_Write_PDU:public APDU
{
public:
  uchar addr_extension;
  uchar count;
  memaddr_t addr;
  CArray data;

  A_UserMemory_Write_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_UserMemory_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_UserMemoryBit_Write_PDU:public APDU
{
public:
  uchar addr_extension;
  uchar count;
  memaddr_t addr;
  CArray andmask;
  CArray xormask;

  A_UserMemoryBit_Write_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_UserMemoryBit_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_UserManufacturerInfo_Read_PDU:public APDU
{
public:

  A_UserManufacturerInfo_Read_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_UserManufacturerInfo_Read;
  }
  bool isResponse (const APDU * req) const;
};

class A_UserManufacturerInfo_Response_PDU:public APDU
{
public:
  uchar manufacturerid;
  uint16_t data;

  A_UserManufacturerInfo_Response_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_UserManufacturerInfo_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_Restart_PDU:public APDU
{
public:

  A_Restart_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Restart;
  }
  bool isResponse (const APDU * req) const;
};

class A_Authorize_Request_PDU:public APDU
{
public:
  eibkey_type key;

  A_Authorize_Request_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Authorize_Request;
  }
  bool isResponse (const APDU * req) const;
};

class A_Authorize_Response_PDU:public APDU
{
public:
  uchar level;

  A_Authorize_Response_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Authorize_Response;
  }
  bool isResponse (const APDU * req) const;
};

class A_Key_Write_PDU:public APDU
{
public:
  uchar level;
  eibkey_type key;

  A_Key_Write_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Key_Write;
  }
  bool isResponse (const APDU * req) const;
};

class A_Key_Response_PDU:public APDU
{
public:
  uchar level;

  A_Key_Response_PDU ();
  bool init (const CArray & p, TracePtr tr);
  CArray ToPacket ();
  String Decode (TracePtr t);
  APDU_type getType () const
  {
    return A_Key_Response;
  }
  bool isResponse (const APDU * req) const;
};

#endif
