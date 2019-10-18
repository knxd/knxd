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
 * Data Link Layer General
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

/** enumartion of Layer 2 frame types*/
typedef enum
{
  /** unknown frame */
  L_Unknown,
  /** L_Data */
  L_Data,
  /** L_Data incomplete. Note that nothing handles this. */
  L_Data_Part,
  /** ACK */
  L_ACK,
  /** NACK */
  L_NACK,
  /** BUSY */
  L_BUSY,
  /** L_Busmon */
  L_Busmon,
}
LPDU_Type;

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

/* L_Unknown */

class L_Unknown_PDU:public LPDU
{
public:
  /** real content*/
  CArray pdu;

  L_Unknown_PDU ();

  std::string Decode (TracePtr tr);
  LPDU_Type getType () const
  {
    return L_Unknown;
  }
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
    return (valid_length ? L_Data : L_Data_Part);
  }
};

class L_ACK_PDU:public LPDU
{
public:

  L_ACK_PDU ();

  std::string Decode (TracePtr tr);
  LPDU_Type getType () const
  {
    return L_ACK;
  }
};

class L_NACK_PDU:public LPDU
{
public:

  explicit L_NACK_PDU ();

  std::string Decode (TracePtr tr);
  LPDU_Type getType () const
  {
    return L_NACK;
  }
};

class L_BUSY_PDU:public LPDU
{
public:

  L_BUSY_PDU ();

  std::string Decode (TracePtr tr);
  LPDU_Type getType () const
  {
    return L_BUSY;
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

#endif
