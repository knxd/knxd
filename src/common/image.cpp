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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "loadctl.h"
#include "image.h"

String
HexDump (CArray data)
{
  char buf[200];
  String s;
  int i;
  sprintf (buf, "%04X ", 0);
  s = buf;
  for (i = 0; i < data (); i++)
    {
      sprintf (buf, "%02x ", data[i]);
      s += buf;
      if (i % 16 == 15)
	{
	  sprintf (buf, "\n%04X ", i + 1);
	  s += buf;
	}
    }
  s += "\n";
  return s;
}

STR_Stream *
STR_Stream::fromArray (const CArray & c)
{
  assert (c () >= 4);
  assert (((c[0] << 8) | c[1]) + 2 == c ());
  STR_Stream *i;

  switch (c[2] << 8 | c[3])
    {
    case L_STRING_PAR:
      i = new STR_StringParameter ();
      break;
    case L_INT_PAR:
      i = new STR_IntParameter ();
      break;
    case L_FLOAT_PAR:
      i = new STR_FloatParameter ();
      break;
    case L_LIST_PAR:
      i = new STR_ListParameter ();
      break;
    case L_GROUP_OBJECT:
      i = new STR_GroupObject ();
      break;
    case L_BCU_TYPE:
      i = new STR_BCUType ();
      break;
    case L_BCU2_INIT:
      i = new STR_BCU2Start ();
      break;
    case L_CODE:
      i = new STR_Code ();
      break;
    case L_BCU1_SIZE:
      i = new STR_BCU1Size ();
      break;
    case L_BCU2_SIZE:
      i = new STR_BCU2Size ();
      break;
    case L_BCU2_KEY:
      i = new STR_BCU2Key ();
      break;
    default:
      i = new STR_Unknown ();
      break;
    }
  if (i->init (c))
    return i;
  delete i;
  i = new STR_Invalid ();
  i->init (c);
  return i;
}

STR_Invalid::STR_Invalid ()
{
}

bool
STR_Invalid::init (const CArray & c)
{
  data = c;
  return true;
}

CArray
STR_Invalid::toArray ()
{
  return data;
}

String
STR_Invalid::decode ()
{
  char buf[200];
  String s;
  sprintf (buf, "Invalid:\n");
  s = buf;
  return s + HexDump (data);
}


STR_Unknown::STR_Unknown ()
{
  type = 0;
}

bool
STR_Unknown::init (const CArray & c)
{
  data.set (c.array () + 4, c () - 4);
  type = c[2] << 8 | c[3];
  return true;
}

CArray
STR_Unknown::toArray ()
{
  CArray d;
  uint16_t len = 2 + data ();
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (type >> 8) & 0xff;
  d[3] = (type) & 0xff;
  d.setpart (data, 4);
  return d;
}

String
STR_Unknown::decode ()
{
  char buf[200];
  String s;
  sprintf (buf, "Unknown %04x:\n", type);
  s = buf;
  return s + HexDump (data);
}

STR_BCUType::STR_BCUType ()
{
  bcutype = 0;
}

bool
STR_BCUType::init (const CArray & c)
{
  if (c () != 6)
    return false;
  bcutype = c[4] << 8 | c[5];
  return true;
}

CArray
STR_BCUType::toArray ()
{
  CArray d;
  uint16_t len = 4;
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_BCU_TYPE >> 8) & 0xff;
  d[3] = (L_BCU_TYPE) & 0xff;
  d[4] = (bcutype >> 8) & 0xff;
  d[5] = (bcutype) & 0xff;
  return d;
}

String
STR_BCUType::decode ()
{
  char buf[200];
  sprintf (buf, "Maskversion: %04x\n", bcutype);
  return buf;
}

STR_Code::STR_Code ()
{
}

bool
STR_Code::init (const CArray & c)
{
  code.set (c.array () + 4, c () - 4);
  return true;
}

CArray
STR_Code::toArray ()
{
  CArray d;
  uint16_t len = 2 + code ();
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_CODE >> 8) & 0xff;
  d[3] = (L_CODE) & 0xff;
  d.setpart (code, 4);
  return d;
}

String
STR_Code::decode ()
{
  char buf[200];
  String s;
  int i;
  sprintf (buf, "Code:\n");
  s = buf;
  return s + HexDump (code);
}

STR_StringParameter::STR_StringParameter ()
{
  addr = 0;
  length = 0;
}

bool
STR_StringParameter::init (const CArray & c)
{
  const uchar *d;
  if (c () < 9)
    return false;
  addr = c[4] << 8 | c[5];
  length = c[6] << 8 | c[7];
  if (c[c () - 1])
    return false;
  d = &c[8];
  while (*d)
    d++;
  if (d != &c[c () - 1])
    return false;
  name = (const char *) c.array () + 8;
  return true;
}

CArray
STR_StringParameter::toArray ()
{
  CArray d;
  uint16_t len = 7 + strlen (name ());
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_STRING_PAR >> 8) & 0xff;
  d[3] = (L_STRING_PAR) & 0xff;
  d[4] = (addr >> 8) & 0xff;
  d[5] = (addr) & 0xff;
  d[6] = (length >> 8) & 0xff;
  d[7] = (length) & 0xff;
  d.setpart ((const uchar *) name (), 8, strlen (name ()) + 1);
  return d;
}

String
STR_StringParameter::decode ()
{
  char buf[200];
  sprintf (buf, "StringParameter: addr=%04x id=%s length=%d\n", addr, name (),
	   length);
  return buf;
}

STR_IntParameter::STR_IntParameter ()
{
  name = 0;
  type = 0;
}

bool
STR_IntParameter::init (const CArray & c)
{
  const uchar *d;
  if (c () < 8)
    return false;
  addr = c[4] << 8 | c[5];
  type = (int8_t) c[6];
  if (c[c () - 1])
    return false;
  d = &c[7];
  while (*d)
    d++;
  if (d != &c[c () - 1])
    return false;
  name = (const char *) c.array () + 7;
  return true;
}

CArray
STR_IntParameter::toArray ()
{
  CArray d;
  uint16_t len = 6 + strlen (name ());
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_INT_PAR >> 8) & 0xff;
  d[3] = (L_INT_PAR) & 0xff;
  d[4] = (addr >> 8) & 0xff;
  d[5] = (addr) & 0xff;
  d[6] = (type) & 0xff;
  d.setpart ((const uchar *) name (), 7, strlen (name ()) + 1);
  return d;
}

String
STR_IntParameter::decode ()
{
  char buf[200];
  sprintf (buf, "IntParameter: addr=%04x id=%s type=%s %d bytes\n", addr,
	   name (), type < 0 ? "signed" : "unsigned", 1 << (abs (type) - 1));
  return buf;
}

STR_FloatParameter::STR_FloatParameter ()
{
  addr = 0;
}

bool
STR_FloatParameter::init (const CArray & c)
{
  const uchar *d;
  if (c () < 7)
    return false;
  addr = c[4] << 8 | c[5];
  if (c[c () - 1])
    return false;
  d = &c[6];
  while (*d)
    d++;
  if (d != &c[c () - 1])
    return false;
  name = (const char *) c.array () + 6;
  return true;
}

CArray
STR_FloatParameter::toArray ()
{
  CArray d;
  uint16_t len = 5 + strlen (name ());
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_FLOAT_PAR >> 8) & 0xff;
  d[3] = (L_FLOAT_PAR) & 0xff;
  d[4] = (addr >> 8) & 0xff;
  d[5] = (addr) & 0xff;
  d.setpart ((const uchar *) name (), 6, strlen (name ()) + 1);
  return d;
}

String
STR_FloatParameter::decode ()
{
  char buf[200];
  sprintf (buf, "FloatParameter: addr=%04x id=%s\n", addr, name ());
  return buf;
}

STR_ListParameter::STR_ListParameter ()
{
  addr = 0;
}

bool
STR_ListParameter::init (const CArray & c)
{
  uint16_t el, i;
  const uchar *d, *d1;
  if (c () < 9)
    return false;
  addr = c[4] << 8 | c[5];
  el = c[6] << 8 | c[7];
  if (c[c () - 1])
    return false;
  d = &c[8];
  d1 = d;
  while (*d)
    d++;
  if (d > &c[c () - 1])
    return false;
  name = (const char *) d1;
  d1 = ++d;
  if (d > &c[c () - 1])
    return false;
  elements.resize (el);
  for (i = 0; i < el; i++)
    {
      while (*d)
	d++;
      if (d > &c[c () - 1])
	return false;
      elements[i] = (const char *) d1;
      d1 = ++d;
      if (d > &c[c ()])
	return false;
    }
  if (d != &c[c ()])
    return false;
  return true;
}

CArray
STR_ListParameter::toArray ()
{
  CArray d;
  uint16_t i, p;
  uint16_t len = 7 + strlen (name ());
  for (i = 0; i < elements (); i++)
    len += strlen (elements[i] ()) + 1;
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_LIST_PAR >> 8) & 0xff;
  d[3] = (L_LIST_PAR) & 0xff;
  d[4] = (addr >> 8) & 0xff;
  d[5] = (addr) & 0xff;
  d[6] = (elements () >> 8) & 0xff;
  d[7] = (elements ()) & 0xff;
  d.setpart ((const uchar *) name (), 8, strlen (name ()) + 1);
  p = 8 + strlen (name ()) + 1;
  for (i = 0; i < elements (); p += strlen (elements[i] ()) + 1, i++)
    d.setpart ((const uchar *) elements[i] (), p,
	       strlen (elements[i] ()) + 1);
  return d;
}

String
STR_ListParameter::decode ()
{
  char buf[200];
  String s;
  sprintf (buf, "ListParameter: addr=%04x id=%s elements=", addr, name ());
  s = buf;
  for (int i = 0; i < elements (); i++)
    {
      sprintf (buf, "%s,", elements[i] ());
      s += buf;
    }
  s += "\n";
  return s;
}

STR_GroupObject::STR_GroupObject ()
{
  no = 0;
}

bool
STR_GroupObject::init (const CArray & c)
{
  const uchar *d;
  if (c () < 6)
    return false;
  no = c[4];
  if (c[c () - 1])
    return false;
  d = &c[5];
  while (*d)
    d++;
  if (d != &c[c () - 1])
    return false;
  name = (const char *) c.array () + 5;
  return true;
}

String
STR_GroupObject::decode ()
{
  char buf[200];
  sprintf (buf, "GROUP_OBJECT %d: id=%s\n", no, name ());
  return buf;
}

CArray
STR_GroupObject::toArray ()
{
  CArray d;
  uint16_t len = 4 + strlen (name ());
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_GROUP_OBJECT >> 8) & 0xff;
  d[3] = (L_GROUP_OBJECT) & 0xff;
  d[4] = (no) & 0xff;
  d.setpart ((const uchar *) name (), 5, strlen (name ()) + 1);
  return d;
}

STR_BCU1Size::STR_BCU1Size ()
{
  textsize = 0;
  stacksize = 0;
  datasize = 0;
  bsssize = 0;
}

bool
STR_BCU1Size::init (const CArray & c)
{
  if (c () != 12)
    return false;
  textsize = c[4] << 8 | c[5];
  stacksize = c[6] << 8 | c[7];
  datasize = c[8] << 8 | c[9];
  bsssize = c[10] << 8 | c[11];
  return true;
}

CArray
STR_BCU1Size::toArray ()
{
  CArray d;
  uint16_t len = 10;
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_BCU1_SIZE >> 8) & 0xff;
  d[3] = (L_BCU1_SIZE) & 0xff;
  d[4] = (textsize >> 8) & 0xff;
  d[5] = (textsize) & 0xff;
  d[6] = (stacksize >> 8) & 0xff;
  d[7] = (stacksize) & 0xff;
  d[8] = (datasize >> 8) & 0xff;
  d[9] = (datasize) & 0xff;
  d[10] = (bsssize >> 8) & 0xff;
  d[11] = (bsssize) & 0xff;
  return d;
}

String
STR_BCU1Size::decode ()
{
  char buf[200];
  sprintf (buf, "BCU1_SIZE: text:%d stack:%d data:%d bss:%d\n", textsize,
	   stacksize, datasize, bsssize);
  return buf;
}

STR_BCU2Size::STR_BCU2Size ()
{
  textsize = 0;
  stacksize = 0;
  lo_datasize = 0;
  lo_bsssize = 0;
  hi_datasize = 0;
  hi_bsssize = 0;
}

bool
STR_BCU2Size::init (const CArray & c)
{
  if (c () != 16)
    return false;
  textsize = c[4] << 8 | c[5];
  stacksize = c[6] << 8 | c[7];
  lo_datasize = c[8] << 8 | c[9];
  lo_bsssize = c[10] << 8 | c[11];
  hi_datasize = c[12] << 8 | c[13];
  hi_bsssize = c[14] << 8 | c[15];
  return true;
}

CArray
STR_BCU2Size::toArray ()
{
  CArray d;
  uint16_t len = 14;
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_BCU2_SIZE >> 8) & 0xff;
  d[3] = (L_BCU2_SIZE) & 0xff;
  d[4] = (textsize >> 8) & 0xff;
  d[5] = (textsize) & 0xff;
  d[6] = (stacksize >> 8) & 0xff;
  d[7] = (stacksize) & 0xff;
  d[8] = (lo_datasize >> 8) & 0xff;
  d[9] = (lo_datasize) & 0xff;
  d[10] = (lo_bsssize >> 8) & 0xff;
  d[11] = (lo_bsssize) & 0xff;
  d[12] = (hi_datasize >> 8) & 0xff;
  d[13] = (hi_datasize) & 0xff;
  d[14] = (hi_bsssize >> 8) & 0xff;
  d[15] = (hi_bsssize) & 0xff;
  return d;
}

String
STR_BCU2Size::decode ()
{
  char buf[200];
  sprintf (buf,
	   "BCU2_SIZE: text:%d stack:%d lo_data:%d lo_bss:%d hi_data:%d hi_bss:%d\n",
	   textsize, stacksize, lo_datasize, lo_bsssize, hi_datasize,
	   hi_bsssize);
  return buf;
}

STR_BCU2Start::STR_BCU2Start ()
{
  addrtab_start = 0;
  addrtab_size = 0;
  assoctab_start = 0;
  assoctab_size = 0;
  readonly_start = 0;
  readonly_end = 0;
  param_start = 0;
  param_end = 0;

  obj_ptr = 0;
  obj_count = 0;
  appcallback = 0;
  groupobj_ptr = 0;
  seg0 = 0;
  seg1 = 0;
  sphandler = 0;
  initaddr = 0;
  runaddr = 0;
  saveaddr = 0;
  eeprom_start = 0;
  eeprom_end = 0;
  poll_addr = 0;
  poll_slot = 0;
}

bool
STR_BCU2Start::init (const CArray & c)
{
  if (c () != 47)
    return false;
  addrtab_start = c[4] << 8 | c[5];
  addrtab_size = c[6] << 8 | c[7];
  assoctab_start = c[8] << 8 | c[9];
  assoctab_size = c[10] << 8 | c[11];
  readonly_start = c[12] << 8 | c[13];
  readonly_end = c[14] << 8 | c[15];
  param_start = c[16] << 8 | c[17];
  param_end = c[18] << 8 | c[19];
  obj_ptr = c[20] << 8 | c[21];
  obj_count = c[22] << 8 | c[23];
  appcallback = c[24] << 8 | c[25];
  groupobj_ptr = c[26] << 8 | c[27];
  seg0 = c[28] << 8 | c[29];
  seg1 = c[30] << 8 | c[31];
  sphandler = c[32] << 8 | c[33];
  initaddr = c[34] << 8 | c[35];
  runaddr = c[36] << 8 | c[37];
  saveaddr = c[38] << 8 | c[39];
  eeprom_start = c[40] << 8 | c[41];
  eeprom_end = c[42] << 8 | c[43];
  poll_addr = c[44] << 8 | c[45];
  poll_slot = c[46];
  return true;
}

CArray
STR_BCU2Start::toArray ()
{
  CArray d;
  uint16_t len = 45;
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_BCU2_INIT >> 8) & 0xff;
  d[3] = (L_BCU2_INIT) & 0xff;
  d[4] = (addrtab_start >> 8) & 0xff;
  d[5] = (addrtab_start) & 0xff;
  d[6] = (addrtab_size >> 8) & 0xff;
  d[7] = (addrtab_size) & 0xff;
  d[8] = (assoctab_start >> 8) & 0xff;
  d[9] = (assoctab_start) & 0xff;
  d[10] = (assoctab_size >> 8) & 0xff;
  d[11] = (assoctab_size) & 0xff;
  d[12] = (readonly_start >> 8) & 0xff;
  d[13] = (readonly_start) & 0xff;
  d[14] = (readonly_end >> 8) & 0xff;
  d[15] = (readonly_end) & 0xff;
  d[16] = (param_start >> 8) & 0xff;
  d[17] = (param_start) & 0xff;
  d[18] = (param_end >> 8) & 0xff;
  d[19] = (param_end) & 0xff;
  d[20] = (obj_ptr >> 8) & 0xff;
  d[21] = (obj_ptr) & 0xff;
  d[22] = (obj_count >> 8) & 0xff;
  d[23] = (obj_count) & 0xff;
  d[24] = (appcallback >> 8) & 0xff;
  d[25] = (appcallback) & 0xff;
  d[26] = (groupobj_ptr >> 8) & 0xff;
  d[27] = (groupobj_ptr) & 0xff;
  d[28] = (seg0 >> 8) & 0xff;
  d[29] = (seg0) & 0xff;
  d[30] = (seg1 >> 8) & 0xff;
  d[31] = (seg1) & 0xff;
  d[32] = (sphandler >> 8) & 0xff;
  d[33] = (sphandler) & 0xff;
  d[34] = (initaddr >> 8) & 0xff;
  d[35] = (initaddr) & 0xff;
  d[36] = (runaddr >> 8) & 0xff;
  d[37] = (runaddr) & 0xff;
  d[38] = (saveaddr >> 8) & 0xff;
  d[39] = (saveaddr) & 0xff;
  d[40] = (eeprom_start >> 8) & 0xff;
  d[41] = (eeprom_start) & 0xff;
  d[42] = (eeprom_end >> 8) & 0xff;
  d[43] = (eeprom_end) & 0xff;
  d[44] = (poll_addr >> 8) & 0xff;
  d[45] = (poll_addr) & 0xff;
  d[46] = (poll_slot) & 0xff;
  return d;
}

String
STR_BCU2Start::decode ()
{
  char buf[600];
  sprintf (buf,
	   "BCU2_INIT: init:%04X run:%04X save:%04X addr_tab:%04X(%d) assoc_tab:%04X(%d) text:%04X-%04X\n"
	   " param:%04X-%04X obj:%04X(%d) appcallback:%04X\n"
	   " groupobj:%04X seg0:%04X seg1:%04X sphandler:%04X eeprom:%04X-%04X poll_addr:%04X poll_slot:%d\n",
	   initaddr, runaddr, saveaddr, addrtab_start, addrtab_size,
	   assoctab_start, assoctab_size, readonly_start, readonly_end,
	   param_start, param_end, obj_ptr, obj_count, appcallback,
	   groupobj_ptr, seg0, seg1, sphandler, eeprom_start, eeprom_end,
	   poll_addr, poll_slot);
  return buf;
}

STR_BCU2Key::STR_BCU2Key ()
{
  installkey = 0xFFFFFFFF;
}

bool
STR_BCU2Key::init (const CArray & c)
{
  unsigned i;
  if (c () % 4)
    return false;
  if (c () < 8)
    return false;
  installkey = (c[4] << 24) | (c[5] << 16) | (c[6] << 8) | (c[7]);
  for (i = 8; i < c (); i += 4)
    keys.add ((c[i] << 24) | (c[i + 1] << 16) | (c[i + 2] << 8) | (c[i + 3]));
  return true;
}

CArray
STR_BCU2Key::toArray ()
{
  CArray d;
  int i;
  uint16_t len = keys () * 4 + 4 + 2;
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_BCU2_KEY >> 8) & 0xff;
  d[3] = (L_BCU2_KEY) & 0xff;
  d[4] = (installkey >> 24) & 0xff;
  d[5] = (installkey >> 16) & 0xff;
  d[6] = (installkey >> 8) & 0xff;
  d[7] = (installkey >> 0) & 0xff;
  for (i = 0; i < keys (); i++)
    {
      d[8 + 4 * i] = (keys[i] >> 24) & 0xff;
      d[9 + 4 * i] = (keys[i] >> 16) & 0xff;
      d[10 + 4 * i] = (keys[i] >> 8) & 0xff;
      d[11 + 4 * i] = (keys[i] >> 0) & 0xff;
    }
  return d;
}


String
STR_BCU2Key::decode ()
{
  char buf[200];
  String s;
  int i;
  sprintf (buf, "BCU2_KEY: install:%08X ", installkey);
  s = buf;
  for (i = 0; i < keys (); i++)
    {
      sprintf (buf, "level%d: %08X ", i, keys[i]);
      s += buf;
    }
  return s;
}

Image::Image ()
{
}

Image::~Image ()
{
  for (int i = 0; i < str (); i++)
    if (str[i])
      delete str[i];
}

String
Image::decode ()
{
  String s = "BCU Memory Image\n";
  for (int i = 0; i < str (); i++)
    s += str[i]->decode ();
  return s;
}

CArray
Image::toArray ()
{
  CArray data;
  data.resize (10);
  data[0] = 0xbc;
  data[1] = 0x68;
  data[2] = 0x0c;
  data[3] = 0x05;
  data[4] = 0xbc;
  data[5] = 0x68;
  data[6] = 0x0c;
  data[7] = 0x05;
  for (int i = 0; i < str (); i++)
    data.setpart (str[i]->toArray (), data ());
  data[8] = (data () >> 8) & 0xff;
  data[9] = (data ()) & 0xff;
  return data;
}

int
Image::findStreamNumber (STR_Type t)
{
  for (int i = 0; i < str (); i++)
    if (str[i]->getType () == t)
      return i;
  return -1;
}

STR_Stream *
Image::findStream (STR_Type t)
{
  int i = findStreamNumber (t);
  if (i == -1)
    return 0;
  else
    return str[i];
}

Image *
Image::fromArray (CArray c)
{
  uint16_t pos = 10;
  uint16_t len;
  if (c () < 10)
    return 0;
  if (c[0] != 0xbc)
    return 0;
  if (c[1] != 0x68)
    return 0;
  if (c[2] != 0x0c)
    return 0;
  if (c[3] != 0x05)
    return 0;
  if (c[4] != 0xbc)
    return 0;
  if (c[5] != 0x68)
    return 0;
  if (c[6] != 0x0c)
    return 0;
  if (c[7] != 0x05)
    return 0;
  if (c[8] != ((c () >> 8) & 0xff))
    return 0;
  if (c[9] != ((c ()) & 0xff))
    return 0;
  Image *i = new Image;
  while (pos < c ())
    {
      if (pos + 4 >= c ())
	{
	  delete i;
	  return 0;
	}
      len = c[pos] << 8 | c[pos + 1];
      if (pos + 2 + len > c () || len < 2)
	{
	  delete i;
	  return 0;
	}
      i->str.add (STR_Stream::fromArray (CArray (&c[pos], len + 2)));
      pos += 2 + len;
    }
  return i;
}

bool
Image::isValid ()
{
  return findStreamNumber (S_Invalid) == -1;
}
