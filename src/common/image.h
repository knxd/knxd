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

String HexDump (CArray data);

typedef enum
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
} STR_Type;

class STR_Stream
{
public:
  virtual ~ STR_Stream ()
  {
  }
  static STR_Stream *fromArray (const CArray & c);
  virtual bool init (const CArray & str) = 0;
  virtual CArray toArray () = 0;
  virtual STR_Type getType () = 0;
  virtual String decode () = 0;
};

class STR_Invalid:public STR_Stream
{
public:
  CArray data;

  STR_Invalid ();
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_Invalid;
  }
  String decode ();
};

class STR_Unknown:public STR_Stream
{
public:
  uint16_t type;
  CArray data;

    STR_Unknown ();
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_Unknown;
  }
  String decode ();
};

class STR_BCUType:public STR_Stream
{
public:
  uint16_t bcutype;

  STR_BCUType ();
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_BCUType;
  }
  String decode ();
};
class STR_Code:public STR_Stream
{
public:
  CArray code;

  STR_Code ();
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_Code;
  }
  String decode ();
};
class STR_StringParameter:public STR_Stream
{
public:
  uint16_t addr;
  uint16_t length;
  String name;

    STR_StringParameter ();
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_StringParameter;
  }
  String decode ();
};
class STR_ListParameter:public STR_Stream
{
public:
  uint16_t addr;
  String name;
    Array < String > elements;

    STR_ListParameter ();
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_ListParameter;
  }
  String decode ();
};
class STR_IntParameter:public STR_Stream
{
public:
  uint16_t addr;
  int8_t type;
  String name;

    STR_IntParameter ();
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_IntParameter;
  }
  String decode ();
};
class STR_FloatParameter:public STR_Stream
{
public:
  uint16_t addr;
  String name;

    STR_FloatParameter ();
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_FloatParameter;
  }
  String decode ();
};
class STR_GroupObject:public STR_Stream
{
public:
  uchar no;
  String name;

    STR_GroupObject ();
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_GroupObject;
  }
  String decode ();
};
class STR_BCU1Size:public STR_Stream
{
public:
  uint16_t textsize;
  uint16_t stacksize;
  uint16_t datasize;
  uint16_t bsssize;

    STR_BCU1Size ();
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_BCU1Size;
  }
  String decode ();
};
class STR_BCU2Size:public STR_Stream
{
public:
  uint16_t textsize;
  uint16_t stacksize;
  uint16_t lo_datasize;
  uint16_t lo_bsssize;
  uint16_t hi_datasize;
  uint16_t hi_bsssize;

    STR_BCU2Size ();
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_BCU2Size;
  }
  String decode ();
};
class STR_BCU2Start:public STR_Stream
{
public:
  uint16_t addrtab_start;
  uint16_t addrtab_size;
  uint16_t assoctab_start;
  uint16_t assoctab_size;
  uint16_t readonly_start;
  uint16_t readonly_end;
  uint16_t param_start;
  uint16_t param_end;

  uint16_t obj_ptr;
  uint16_t obj_count;
  uint16_t appcallback;
  uint16_t groupobj_ptr;
  uint16_t seg0;
  uint16_t seg1;
  uint16_t sphandler;
  uint16_t initaddr;
  uint16_t runaddr;
  uint16_t saveaddr;
  uint16_t eeprom_start;
  uint16_t eeprom_end;
  eibaddr_t poll_addr;
  uint8_t poll_slot;

    STR_BCU2Start ();
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_BCU2Start;
  }
  String decode ();
};

class STR_BCU2Key:public STR_Stream
{
public:
  eibkey_type installkey;
  Array < eibkey_type > keys;

  STR_BCU2Key ();
  bool init (const CArray & str);
  CArray toArray ();
  STR_Type getType ()
  {
    return S_BCU2Key;
  }
  String decode ();
};

class Image
{
public:
  Array < STR_Stream * >str;

  Image ();
  virtual ~ Image ();

  static Image *fromArray (CArray c);
  CArray toArray ();
  String decode ();
  bool isValid ();

  int findStreamNumber (STR_Type t);
  STR_Stream *findStream (STR_Type t);
};

#endif
