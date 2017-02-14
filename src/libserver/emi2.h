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

#ifndef EIB_EMI2_H
#define EIB_EMI2_H

#include "emi_common.h"

/** EMI2 backend */
class EMI2Driver:public EMI_Common
{
  void cmdEnterMonitor();
  void cmdLeaveMonitor();
  void cmdOpen();
  void cmdClose();
  const uint8_t * getIndTypes();
public:
  EMI2Driver (LowLevelDriver * i, LinkConnectPtr c, IniSection& s) : EMI_Common(i,c,s) {}
  ~EMI2Driver ();
};

#endif
