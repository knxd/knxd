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

#include "management.h"

int
Management_Connection::X_Progmode_On ()
{
  CArray d;
  if (A_Memory_Read (0x60, 1, d) == -1)
    return -1;
  if (!(d[0] & 0x01))
    d[0] = d[0] ^ 0x81;
  if (X_Memory_Write (0x60, d) != 0)
    return -1;
  return 0;
}

int
Management_Connection::X_Progmode_Off ()
{
  CArray d;
  if (A_Memory_Read (0x60, 1, d) == -1)
    return -1;
  if ((d[0] & 0x01))
    d[0] = d[0] ^ 0x81;
  if (X_Memory_Write (0x60, d) != 0)
    return -1;
  return 0;
}

int
Management_Connection::X_Progmode_Toggle ()
{
  CArray d;
  if (A_Memory_Read (0x60, 1, d) == -1)
    return -1;
  d[0] = d[0] ^ 0x81;
  if (X_Memory_Write (0x60, d) != 0)
    return -1;
  return 0;
}

int
Management_Connection::X_Progmode_Status ()
{
  CArray d;
  if (A_Memory_Read (0x60, 1, d) == -1)
    return -1;
  if (d[0] & 0x01)
    return 1;
  else
    return 0;
}

int
Management_Connection::X_Get_PEIType (int16_t & val)
{
  int16_t v;
  if (A_ADC_Read (4, 1, v) == -1)
    return -1;
  val = (v * 10 + 16) / 128;
  return 0;
}

int
Management_Connection::X_PropertyScan (Array < PropertyInfo > &p)
{
  p.resize (0);
  PropertyInfo p1;
  uchar obj, i;
  obj = 0;
  do
    {
      i = 0;
      do
	{
	  p1.obj = obj;
	  p1.property = 0;
	  if (A_Property_Desc
	      (obj, p1.property, i, p1.type, p1.count, p1.access) == -1)
	    return -1;
	  if (p1.property == 1 && p1.type == 4)
	    {
	      CArray a;
	      if (A_Property_Read (obj, 1, 1, 1, a) == -1)
		return -1;
	      if (a () != 2)
		return -1;
	      p1.count = (a[0] << 8) | (a[1]);
	    }
	  if (p1.property != 0)
	    {
	      p.resize (p () + 1);
	      p[p () - 1] = p1;
	    }
	  i++;
	}
      while (p1.property != 0);
      obj++;
    }
  while (i != 1);
  return 0;
}
