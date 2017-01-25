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

#ifndef TPDU_H
#define TPDU_H

#include "common.h"

/** enumaration of TPDU types */
typedef enum
{
  /** unknown TPDU */
  T_UNKNOWN,
  /** any connectionless TPDU */
  T_DATA_XXX_REQ,
  /** connectionoriented TPDU */
  T_DATA_CONNECTED_REQ,
  /** T_Connect */
  T_CONNECT_REQ,
  /** T_Disconnect */
  T_DISCONNECT_REQ,
  /** T_ACK */
  T_ACK,
  /** T_NACK */
  T_NACK,
}
TPDU_Type;

class TPDU;
typedef std::unique_ptr<TPDU> TPDUPtr;

/** represents a TPDU */
class TPDU
{
public:
  virtual ~TPDU ()
  {
  }

  virtual bool init (const CArray & c, TracePtr t) = 0;
  /** convert to character array */
  virtual CArray ToPacket () = 0;
  /** decode content as string */
  virtual String Decode (TracePtr t) = 0;
  /** gets TPDU type */
  virtual TPDU_Type getType () const = 0;
  /** converts character array to a TPDU */
  static TPDUPtr fromPacket (const CArray & c, TracePtr t);
};

class T_UNKNOWN_PDU:public TPDU
{
public:
  CArray pdu;

  T_UNKNOWN_PDU ();
  bool init (const CArray & c, TracePtr t);
  CArray ToPacket ();
  String Decode (TracePtr t);
  TPDU_Type getType () const
  {
    return T_UNKNOWN;
  }
};

class T_DATA_XXX_REQ_PDU:public TPDU
{
public:
  CArray data;

  T_DATA_XXX_REQ_PDU ();
  bool init (const CArray & c, TracePtr t);
  CArray ToPacket ();
  String Decode (TracePtr t);
  TPDU_Type getType () const
  {
    return T_DATA_XXX_REQ;
  }
};

class T_DATA_CONNECTED_REQ_PDU:public TPDU
{
public:
  uchar serno;
  CArray data;

  T_DATA_CONNECTED_REQ_PDU ();
  bool init (const CArray & c, TracePtr t);
  CArray ToPacket ();
  String Decode (TracePtr t);
  TPDU_Type getType () const
  {
    return T_DATA_CONNECTED_REQ;
  }
};

class T_CONNECT_REQ_PDU:public TPDU
{
public:

  T_CONNECT_REQ_PDU ();
  bool init (const CArray & c, TracePtr t);
  CArray ToPacket ();
  String Decode (TracePtr t);
  TPDU_Type getType () const
  {
    return T_CONNECT_REQ;
  }
};

class T_DISCONNECT_REQ_PDU:public TPDU
{
public:

  T_DISCONNECT_REQ_PDU ();
  bool init (const CArray & c, TracePtr t);
  CArray ToPacket ();
  String Decode (TracePtr t);
  TPDU_Type getType () const
  {
    return T_DISCONNECT_REQ;
  }
};

class T_ACK_PDU:public TPDU
{
public:
  uchar serno;

  T_ACK_PDU ();
  bool init (const CArray & c, TracePtr t);
  CArray ToPacket ();
  String Decode (TracePtr t);
  TPDU_Type getType () const
  {
    return T_ACK;
  }
};

class T_NACK_PDU:public TPDU
{
public:
  uchar serno;

  T_NACK_PDU ();
  bool init (const CArray & c, TracePtr t);
  CArray ToPacket ();
  String Decode (TracePtr t);
  TPDU_Type getType () const
  {
    return T_NACK;
  }
};

#endif
