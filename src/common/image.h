/*
    BCU SDK bcu development enviroment
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

#ifndef IMAGE_H
#define IMAGE_H

#include "types.h"

std::string HexDump (CArray data);

enum STR_Type
{
  S_Invalid,
  S_Unknown,
  S_BCUType,
  S_Code,
  S_StringParameter,
  S_IntParameter,
  S_FloatParameter,
  S_ListParameter,
  S_GroupObject,
  S_BCU1Size,
  S_BCU2Size,
  S_BCU2Start,
  S_BCU2Key,
};

class STR_Stream
{
public:
  virtual ~STR_Stream () = default;
  static STR_Stream *fromArray (const CArray & c);
  virtual bool init (const CArray & str) = 0;
  virtual CArray toArray () = 0;
  virtual STR_Type getType () = 0;
  virtual std::string decode () = 0;
};

class STR_Invalid:public STR_Stream
{
public:
  CArray data;

  STR_Invalid () = default;
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_Invalid;
  }
  std::string decode ();
};

class STR_Unknown:public STR_Stream
{
public:
  uint16_t type = 0;
  CArray data;

    STR_Unknown () = default;
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_Unknown;
  }
  std::string decode ();
};

class STR_BCUType:public STR_Stream
{
public:
  uint16_t bcutype = 0;

  STR_BCUType () = default;
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_BCUType;
  }
  std::string decode ();
};

class STR_Code:public STR_Stream
{
public:
  CArray code;

  STR_Code () = default;
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_Code;
  }
  std::string decode ();
};

class STR_StringParameter:public STR_Stream
{
public:
  uint16_t addr = 0;
  uint16_t length = 0;
  std::string name;

    STR_StringParameter () = default;
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_StringParameter;
  }
  std::string decode ();
};

class STR_ListParameter:public STR_Stream
{
public:
  uint16_t addr = 0;
  std::string name;
    std::vector < std::string > elements;

    STR_ListParameter () = default;
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_ListParameter;
  }
  std::string decode ();
};

class STR_IntParameter:public STR_Stream
{
public:
  uint16_t addr;
  int8_t type = 0;
  std::string name = "";

    STR_IntParameter () = default;
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_IntParameter;
  }
  std::string decode ();
};

class STR_FloatParameter:public STR_Stream
{
public:
  uint16_t addr = 0;
  std::string name;

    STR_FloatParameter () = default;
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_FloatParameter;
  }
  std::string decode ();
};

class STR_GroupObject:public STR_Stream
{
public:
  uchar no = 0;
  std::string name;

    STR_GroupObject () = default;
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_GroupObject;
  }
  std::string decode ();
};

class STR_BCU1Size:public STR_Stream
{
public:
  uint16_t textsize = 0;
  uint16_t stacksize = 0;
  uint16_t datasize = 0;
  uint16_t bsssize = 0;

    STR_BCU1Size () = default;
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_BCU1Size;
  }
  std::string decode ();
};

class STR_BCU2Size:public STR_Stream
{
public:
  uint16_t textsize = 0;
  uint16_t stacksize = 0;
  uint16_t lo_datasize = 0;
  uint16_t lo_bsssize = 0;
  uint16_t hi_datasize = 0;
  uint16_t hi_bsssize = 0;

    STR_BCU2Size () = default;
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_BCU2Size;
  }
  std::string decode ();
};

class STR_BCU2Start:public STR_Stream
{
public:
  uint16_t addrtab_start = 0;
  uint16_t addrtab_size = 0;
  uint16_t assoctab_start = 0;
  uint16_t assoctab_size = 0;
  uint16_t readonly_start = 0;
  uint16_t readonly_end = 0;
  uint16_t param_start = 0;
  uint16_t param_end = 0;

  uint16_t obj_ptr = 0;
  uint16_t obj_count = 0;
  uint16_t appcallback = 0;
  uint16_t groupobj_ptr = 0;
  uint16_t seg0 = 0;
  uint16_t seg1 = 0;
  uint16_t sphandler = 0;
  uint16_t initaddr = 0;
  uint16_t runaddr = 0;
  uint16_t saveaddr = 0;
  uint16_t eeprom_start = 0;
  uint16_t eeprom_end = 0;
  eibaddr_t poll_addr = 0;
  uint8_t poll_slot = 0;

    STR_BCU2Start () = default;
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_BCU2Start;
  }
  std::string decode ();
};

class STR_BCU2Key:public STR_Stream
{
public:
  eibkey_type installkey = 0xFFFFFFFF;
  std::vector < eibkey_type > keys;

  STR_BCU2Key () = default;
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_BCU2Key;
  }
  std::string decode ();
};

class Image
{
public:
  std::vector < STR_Stream * >str;

  Image () = default;
  virtual ~Image ();

  static Image *fromArray (CArray c);
  CArray toArray ();
  std::string decode ();
  bool isValid ();

  int findStreamNumber (STR_Type t);
  STR_Stream *findStream (STR_Type t);
};

#endif
