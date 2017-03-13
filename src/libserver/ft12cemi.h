/*
    EIBD eib bus access and management daemon
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>
 
    cEMI support for USB
    Copyright (C) 2013 Meik Felser <felser@cs.fau.de>

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

#ifndef EIB_FT12CEMI_H
#define EIB_FT12CEMI_H

#include "cemi.h"

/** CEMI backend */
class FT12CEMIDriver : public CEMIDriver
{
  void cmdOpen(); 
public:
  FT12CEMIDriver (LowLevelDriver * i, LowLevelIface* c, IniSection& s) : CEMIDriver(i,c,s)
    {
      t->setAuxName("ft12cemi");
    }
  FT12CEMIDriver (LowLevelIface* c, IniSection& s) : CEMIDriver(c,s)
    {
      t->setAuxName("ft12cemi");
    }
  virtual ~FT12CEMIDriver ();
};

#endif  /* EIB_CEMI_H */
