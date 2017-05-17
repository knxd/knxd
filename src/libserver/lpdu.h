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

#ifndef LPDU_H
#define LPDU_H

#include <memory>

class LPDU;
class L_Data_PDU;
class L_Busmonitor_PDU;
typedef std::unique_ptr<LPDU> LPDUPtr;
typedef std::unique_ptr<L_Data_PDU> LDataPtr;
typedef std::unique_ptr<L_Busmonitor_PDU> LBusmonPtr;

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
  /** busmonitor or vBusmonitor frame */
  L_Busmonitor,
}
LPDU_Type;

/** represents a Layer 2 frame */
class LPDU
{
public:
  LPDU () { }
  virtual ~LPDU () { }

  virtual bool init (const CArray & c) = 0;
  /** convert to a character array */
  virtual CArray ToPacket () = 0;
  /** decode content as string */
  virtual String Decode (TracePtr t) = 0;
  /** get frame type */
  virtual LPDU_Type getType () const = 0;
  /** converts a character array to a Layer 2 frame */
  static LPDUPtr fromPacket (const CArray & c, TracePtr t);
};

/* L_Unknown */

class L_Unknown_PDU:public LPDU
{
public:
  /** real content*/
  CArray pdu;

  L_Unknown_PDU ();

  bool init (const CArray & c);
  CArray ToPacket ();
  String Decode (TracePtr t);
  LPDU_Type getType () const
  {
    return L_Unknown;
  }
};

/* L_Data */

class L_Data_PDU:public LPDU
{
public:
  /** priority*/
  EIB_Priority prio;
  /** is repreated */
  bool repeated;
  /** checksum ok */
  bool valid_checksum;
  /** length ok */
  bool valid_length;
  /** to group/individual address*/
  EIB_AddrType AddrType;
  eibaddr_t source, dest;
  uchar hopcount;
  /** payload of Layer 4 */
  CArray data;

  L_Data_PDU ();

  bool init (const CArray & c);
  CArray ToPacket ();
  String Decode (TracePtr t);
  LPDU_Type getType () const
  {
    return (valid_length ? L_Data : L_Data_Part);
  }
};

/* L_Busmonitor */

class L_Busmonitor_PDU:public LPDU
{
public:
  /** content of the TP1 frame */
  CArray pdu;
  uint8_t status;
  uint32_t timestamp;

  L_Busmonitor_PDU ();

  bool init (const CArray & c);
  CArray ToPacket ();
  String Decode (TracePtr t);
  LPDU_Type getType () const
  {
    return L_Busmonitor;
  }
};

class L_ACK_PDU:public LPDU
{
public:

  L_ACK_PDU ();

  bool init (const CArray & c);
  CArray ToPacket ();
  String Decode (TracePtr t);
  LPDU_Type getType () const
  {
    return L_ACK;
  }
};

class L_NACK_PDU:public LPDU
{
public:

  explicit L_NACK_PDU ();

  bool init (const CArray & c);
  CArray ToPacket ();
  String Decode (TracePtr t);
  LPDU_Type getType () const
  {
    return L_NACK;
  }
};

class L_BUSY_PDU:public LPDU
{
public:

  L_BUSY_PDU ();

  bool init (const CArray & c);
  CArray ToPacket ();
  String Decode (TracePtr t);
  LPDU_Type getType () const
  {
    return L_BUSY;
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
