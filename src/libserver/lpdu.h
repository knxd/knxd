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

#include "common.h"

class Layer2Interface;

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
  Layer2Interface *l2;
  LPDU (Layer2Interface *layer2 = 0)
  {
    object = 0;
    l2 = layer2;
  }
  virtual ~LPDU ()
  {
  }

  virtual bool init (const CArray & c) = 0;
  /** convert to a character array */
  virtual CArray ToPacket () = 0;
  /** decode content as string */
  virtual String Decode () = 0;
  /** get frame type */
  virtual LPDU_Type getType () const = 0;
  /** converts a character array to a Layer 2 frame */
  static LPDU *fromPacket (const CArray & c, Layer2Interface * layer2);

  void *object;
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
  String Decode ();
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
  String Decode ();
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
  String Decode ();
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
  String Decode ();
  LPDU_Type getType () const
  {
    return L_ACK;
  }
};

class L_NACK_PDU:public LPDU
{
public:

  L_NACK_PDU ();

  bool init (const CArray & c);
  CArray ToPacket ();
  String Decode ();
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
  String Decode ();
  LPDU_Type getType () const
  {
    return L_BUSY;
  }
};

/** interface for callback for Layer 2 frames */
class LPDU_CallBack 
{
public:
  /** callback: a Layer 2 frame has been received */
  virtual void Get_LPDU (LPDU * l) = 0;
};

/** interface for callback for L_Data frames */
class L_Data_CallBack
{   
public:
  /** callback: a L_Data frame has been received */
  virtual void Get_L_Data (L_Data_PDU * l) = 0; 
};  
    
/** interface for callback for busmonitor frames */
class L_Busmonitor_CallBack
{
public:
  /** callback: a bus monitor frame has been received */
  virtual void Get_L_Busmonitor (L_Busmonitor_PDU * l) = 0;
};

#endif
