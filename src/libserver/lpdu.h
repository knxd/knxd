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
 * @ingroup KNX_03_03_02
 * Data Link Layer General
 * @{
 */

#ifndef LPDU_H
#define LPDU_H

#include <memory>

class LPDU;
class L_Data_PDU;
class L_Busmon_PDU;
using LPDUPtr = std::unique_ptr<LPDU>;
using LDataPtr = std::unique_ptr<L_Data_PDU>;
using LBusmonPtr = std::unique_ptr<L_Busmon_PDU>;

#include "common.h"
#include "link.h"

/** enumeration of Layer 2 frame types */
enum LPDU_Type
{
  /** unknown LPDU */
  L_Unknown = 0,
  /** L_Data */
  L_Data,
  /** L_SystemBroadcast */
  L_SystemBroadcast,
  /** L_Poll_Data */
  L_Poll_Data,
  /** L_Poll_Update */
  L_Poll_Update,
  /** L_Busmon */
  L_Busmon,
  /** L_Service_Information */
  L_Service_Information,
  /** L_Management */
  L_Management,
};

/** represents a Layer 2 frame */
class LPDU
{
public:
  LPDU () = default;
  virtual ~LPDU () = default;

  /** decode content as string */
  virtual std::string Decode (TracePtr tr) = 0;
  /** get frame type */
  virtual LPDU_Type getType () const = 0;
};

/* L_Data */

class L_Data_PDU:public LPDU
{
public:
  /* acknowledge mandatory/optional */
  uint8_t ack_request = 0;
  /** address type */
  EIB_AddrType address_type = IndividualAddress;
  /* destination address */
  eibaddr_t destination_address = 0;
  /** standard/extended frame format */
  uint8_t frame_format = 0;
  /** octet count */
  uint8_t octet_count = 0;
  /** priority */
  EIB_Priority priority = PRIO_LOW;
  /* source address */
  eibaddr_t source_address = 0;
  /** payload of Layer 4 (LSDU) */
  CArray data; // @todo rename to lsdu
  /* status */
  uint8_t l_status = 0;

  /** is repreated */
  bool repeated = 0;
  /** checksum ok */
  bool valid_checksum = 1;
  /** length ok */
  bool valid_length = 1;
  uchar hop_count = 0x06;

  L_Data_PDU () = default;

  std::string Decode (TracePtr tr);
  LPDU_Type getType () const
  {
    return L_Data;
  }
};

/* L_SystemBroadcast */

class L_SystemBroadcast_PDU:public LPDU
{
public:
  /* acknowledge mandatory/optional */
  uint8_t ack_request = 0;
  /** address type */
  EIB_AddrType address_type = IndividualAddress;
  /* destination address */
  eibaddr_t destination_address = 0;
  /** standard/extended frame format */
  uint8_t frame_format = 0;
  /** octet count */
  uint8_t octet_count = 0;
  /** priority */
  EIB_Priority priority = PRIO_LOW;
  /* source address */
  eibaddr_t source_address = 0;
  /** payload of Layer 4 (LSDU) */
  CArray data; // @todo rename to lsdu
  /* status */
  uint8_t l_status = 0;

  L_SystemBroadcast_PDU () = default;

  LPDU_Type getType () const
  {
    return L_SystemBroadcast;
  }
};

/* L_Poll_Data */

class L_Poll_Data_PDU:public LPDU
{
public:
  /* destination address */
  eibaddr_t destination_address = 0;
  uint8_t no_of_expected_poll_data = 0;
  CArray poll_data_sequence;
  uint8_t l_status = 0;

  L_Poll_Data_PDU () = default;

  LPDU_Type getType () const
  {
    return L_Poll_Data;
  }
};

/* L_Poll_Update */

class L_Poll_Update_PDU:public LPDU
{
public:
  uint8_t poll_data;

  L_Poll_Update_PDU () = default;

  LPDU_Type getType () const
  {
    return L_Poll_Update;
  }
};

/* L_Busmon */

class L_Busmon_PDU:public LPDU
{
public:
  uint8_t status; // @todo rename to l_status
  /** content of the TP1 frame */
  CArray pdu; // @todo rename to lpdu
  uint32_t time_stamp;

  L_Busmon_PDU ();

  std::string Decode (TracePtr tr);
  LPDU_Type getType () const
  {
    return L_Busmon;
  }
};

/** interface for callback for busmonitor frames */
class L_Busmonitor_CallBack
{
public:
  L_Busmonitor_CallBack(std::string& n) : name(n) { }
  std::string& name;
  /** callback: a bus monitor frame has been received */
  virtual void send_L_Busmonitor (LBusmonPtr l) = 0;
};

/* L_Service_Information */

class L_Service_Information_PDU:public LPDU
{
public:
  L_Service_Information_PDU () = default;

  LPDU_Type getType () const
  {
    return L_Service_Information;
  }
};

/* L_Management */

class L_Management_PDU:public LPDU
{
public:
  L_Management_PDU () = default;

  LPDU_Type getType () const
  {
    return L_Management;
  }
};

#endif
