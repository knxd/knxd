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

#ifndef MANAGEMENT_H
#define MANAGEMENT_H

#include "layer7.h"

/** information structure abot a property */
typedef struct
{
  /** object index */
  uchar obj;
  /** property id */
  uchar property;
  /** property type */
  uchar type;
  /** for property 1 (Object Type) it contains the object type, else the element count */
  uint16_t count;
  /** access level */
  uchar access;
} PropertyInfo;

class Management_Connection:public Layer7_Connection
{
public:
  Management_Connection (TracePtr tr,
                         eibaddr_t dest):Layer7_Connection (tr, dest)
  {
    t->setAuxName("Mgr");
  }

  /** turns programming mode on */
  int X_Progmode_On ();
  /** turns programming mode off */
  int X_Progmode_Off ();
  /** toggles programming mode */
  int X_Progmode_Toggle ();
  /** returns programming mode status */
  int X_Progmode_Status ();
  /** reads PEI type */
  int X_Get_PEIType (int16_t & val);
  /** scans all properties */
  int X_PropertyScan (Array < PropertyInfo > &pi);
};

class Management_Individual:public Layer7_Individual
{
public:
  Management_Individual (TracePtr tr,
                         eibaddr_t dest):Layer7_Individual (tr, dest) { }
};

#endif
