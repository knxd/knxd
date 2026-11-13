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
 * @ingroup KNX_03_02_02
 * Communication Medium TP1
 * @{
 */

#ifndef CM_TP1_H
#define CM_TP1_H

#include "lpdu.h"

/**
 * How L_Data_to_CM_TP1() chooses between the standard and the extended
 * TP1 frame format.
 */
enum TP1_Format
{
  /**
   * Extended only if required: either the payload does not fit the standard
   * format's 4-bit length field, or L_Data_PDU::ext_frame_format carries an
   * LTE-HEE address that the standard format has no room for. Keeps the common
   * short standard frame in the cheaper encoding.
   */
  TP1_auto = 0,
  /**
   * Extended whenever the PDU says it arrived extended. Used where the frame
   * must be reproduced as it appeared on the wire, e.g. busmonitor.
   */
  TP1_as_received,
};

/** convert L_Data_PDU to TP1 frame */
CArray L_Data_to_CM_TP1 (const LDataPtr & p, TP1_Format fmt = TP1_auto);

/** create L_Data_PDU out of a TP1 frame */
LDataPtr CM_TP1_to_L_Data (const CArray & c, TracePtr tr);

#endif

/** @} */
