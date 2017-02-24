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

#ifndef EIB_EMI1_H
#define EIB_EMI1_H

#include "emi_common.h"

/** EMI1 backend */
class EMI1Driver:public EMI_Common
{
  void cmdEnterMonitor();
  void cmdLeaveMonitor();
  void cmdOpen();
  void cmdClose();
  const uint8_t * getIndTypes();
public:
  EMI1Driver (LowLevelDriver * i, const LinkConnectPtr_& c, IniSection& s) : EMI_Common(i,c,s) {}
  EMI1Driver (const LinkConnectPtr_& c, IniSection& s) : EMI_Common(c,s) {}
  virtual ~EMI1Driver ();
};

#endif
