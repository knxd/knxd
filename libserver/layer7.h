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

#ifndef LAYER7_H
#define LAYER7_H

#include "layer4.h"

class APDU;

/** layer 7 broadcast connection */
class Layer7_Broadcast
{
  Trace *t;
  T_Broadcast *l4;

public:
    Layer7_Broadcast (Layer3 * l3, Trace * tr);
    virtual ~ Layer7_Broadcast ();
  bool init ();

    /** send IndividualAddress_Write */
  void A_IndividualAddress_Write (eibaddr_t addr);
  /** sends A_IndividualAddress_Read and collects responses */
    Array < eibaddr_t > A_IndividualAddress_Read (unsigned timeout = 3);
};

/** Layer 7 Individual Connection */
class Layer7_Connection
{
  Trace *t;
  T_Connection *l4;
  /** destination address */
  eibaddr_t dest;

  /** sends APDU and waits for respone; return NULL, if it fails */
  APDU *Request_Response (APDU * r);
public:
    Layer7_Connection (Layer3 * l3, Trace * tr, eibaddr_t dest);
    virtual ~ Layer7_Connection ();
  bool init ();

    /** send A_Restart */
  void A_Restart ();
  /** read a property */
  int A_Property_Read (uchar obj, uchar propertyid, uint16_t start,
		       uchar count, CArray & erg);
  /** write a property */
  int A_Property_Write (uchar obj, uchar propertyid, uint16_t start,
			uchar count, const CArray & data, CArray & result);
  /** descripbe a property */
  int A_Property_Desc (uchar obj, uchar & property, uchar property_index,
		       uchar & type, uint16_t & max_nr_elements,
		       uchar & access);
  /** read device descript (mask version) */
  int A_Device_Descriptor_Read (uint16_t & maskver, uchar type = 0);
  /** read ADC */
  int A_ADC_Read (uchar channel, uchar readcount, int16_t & value);
  /** read memory */
  int A_Memory_Read (memaddr_t addr, uchar len, CArray & data);
  /** write memory */
  int A_Memory_Write (memaddr_t addr, const CArray & data);
  /** try to authorize */
  int A_Authorize (eibkey_type key, uchar & level);
  /** try to write a key */
  int A_KeyWrite (eibkey_type key, uchar & level);

  /** write a property and verify */
  int X_Property_Write (uchar obj, uchar propertyid, uint16_t start,
			uchar count, const CArray & data);
  /** write memory and verify */
  int X_Memory_Write (memaddr_t addr, const CArray & data);

  /** write arbitray memory block and verify */
  int X_Memory_Write_Block (memaddr_t addr, const CArray & data);
  /** read arbitray memory block*/
  int X_Memory_Read_Block (memaddr_t addr, int len, CArray & erg);
  /** write arbitray memory block without verify */
  int A_Memory_Write_Block (memaddr_t addr, const CArray & data);
};

/** Layer 7 Individual  */
class Layer7_Individual
{
  Trace *t;
  T_Individual *l4;
  /** destination address */
  eibaddr_t dest;

  /** sends APDU and waits for respone; return NULL, if it fails */
  APDU *Request_Response (APDU * r);
public:
    Layer7_Individual (Layer3 * l3, Trace * tr, eibaddr_t dest);
    virtual ~ Layer7_Individual ();
  bool init ();

  /** read a property */
  int A_Property_Read (uchar obj, uchar propertyid, uint16_t start,
		       uchar count, CArray & erg);
  /** write a property */
  int A_Property_Write (uchar obj, uchar propertyid, uint16_t start,
			uchar count, const CArray & data, CArray & result);
};

#endif
