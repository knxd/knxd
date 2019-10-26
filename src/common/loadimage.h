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

#ifndef LOADIMAGE_H
#define LOADIMAGE_H

#include "eibloadresult.h"

#include "types.h"

struct EIBLoadRequest
{
  BCU_LOAD_RESULT error;
  uint8_t obj;
  uint8_t prop;
  uint16_t start;
  CArray req;
  CArray result;
  uint16_t memaddr;
  uint16_t len;
};

class BCUImage
{
public:
  enum
  {
    B_bcu1, B_bcu20, B_bcu21
  } BCUType;
  CArray code;
  std::vector < EIBLoadRequest > load;
  eibaddr_t addr;
  eibkey_type installkey;
  std::vector < eibkey_type > keys;
};

BCU_LOAD_RESULT PrepareLoadImage (const CArray & c, BCUImage * &img);

std::string decodeBCULoadResult (const BCU_LOAD_RESULT r);

#endif
