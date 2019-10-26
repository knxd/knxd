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
 * @ingroup KNX_03_03_03
 * Network Layer
 * @{
 */

#ifndef NPDU_H
#define NPDU_H

#include <memory>

#include "lpdu.h"

/** enumeration of Layer 3 frame types */
enum NPDU_Type
{
  /** unknown NPDU */
  N_Unknown = 0,
  /** N_Data_Individual */
  N_Data_Individual,
  /** N_Data_Group */
  N_Data_Group,
  /** N_Data_Broadcast */
  N_Data_Broadcast,
  /** N_Data_SystemBroadcast */
  N_Data_SystemBroadcast,
};

class NPDU;
using NPDUPtr = std::unique_ptr<NPDU>;

/** NPDU */
class NPDU
{
public:
  uint8_t hop_count_type = 0x06;

  NPDU () = default;
  virtual ~NPDU () = default;

  virtual bool init (const CArray & c) = 0;
  /** convert to a character array */
  virtual CArray ToPacket () const = 0;
  /** decode content as string */
  virtual std::string Decode (TracePtr tr) const = 0;
  /** get frame type */
  virtual NPDU_Type getType () const = 0;
  /** converts a character array to a Layer 3 frame */
  static NPDUPtr fromPacket (const CArray & c, TracePtr tr);
};

/** N_Data_Individual */
class N_Data_Individual_PDU:public NPDU
{
public:
  L_Data_PDU * lpdu = nullptr;

  N_Data_Individual_PDU () = default;

  virtual bool init (const CArray & c) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual NPDU_Type getType () const override
  {
    return N_Data_Individual;
  }
};

/** N_Data_Group */
class N_Data_Group_PDU:public NPDU
{
public:
  L_Data_PDU * lpdu = nullptr;

  N_Data_Group_PDU () = default;

  virtual bool init (const CArray & c) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual NPDU_Type getType () const override
  {
    return N_Data_Group;
  }
};

/** N_Data_Broadcast */
class N_Data_Broadcast_PDU:public NPDU
{
public:
  L_Data_PDU * lpdu = nullptr;

  N_Data_Broadcast_PDU () = default;

  virtual bool init (const CArray & c) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual NPDU_Type getType () const override
  {
    return N_Data_Broadcast;
  }
};

/** N_Data_SystemBroadcast */
class N_Data_SystemBroadcast_PDU:public NPDU
{
public:
  L_SystemBroadcast_PDU * lpdu = nullptr;

  N_Data_SystemBroadcast_PDU () = default;

  virtual bool init (const CArray & c) override;
  virtual CArray ToPacket () const override;
  virtual std::string Decode (TracePtr tr) const override;
  virtual NPDU_Type getType () const override
  {
    return N_Data_SystemBroadcast;
  }
};

#endif
