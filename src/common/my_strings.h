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

#ifndef MY_STRING_H
#define MY_STRING_H

#include <string.h>

/** implements a string */
class String
{
  /** content */
  char *data;
  /** length */
  unsigned len;
public:
  /** free memory */
    virtual ~ String ()
  {
    if (data)
      delete[]data;
  }

  /** initialize with the empty string */
  String ()
  {
    data = 0;
    len = 0;
  }

  /** initialize with a string */
  String (const String & a)
  {
    len = a.len;
    if (len)
      {
	data = new char[len];
	memcpy (data, a.data, len);
      }
    else
      data = 0;
  }
  /** assign a String */
  const String & operator = (const String & a)
  {
    if (data)
      delete[]data;
    len = a.len;
    if (len)
      {
	data = new char[len];
	memcpy (data, a.data, len);
      }
    else
      data = 0;
    return a;
  }

  /** initialize with a character constant */
  String (const char *msg)
  {
    if (msg)
      {
	len = strlen (msg) + 1;
	data = new char[len];
	strcpy (data, msg);
      }
    else
      {
	data = 0;
	len = 0;
      }
  }

  /** assign a character constant */
  const String & operator = (const char *msg)
  {
    if (data)
      delete[]data;
    if (msg)
      {
	len = strlen (msg) + 1;
	data = new char[len];
	strcpy (data, msg);
      }
    else
      {
	data = 0;
	len = 0;
      }
    return *this;
  }

  /** concats two strings*/
  String operator + (const String & a)
  {
    String b;
    if (!len)
      return a;
    if (!a.len)
      return *this;
    b.len = a.len + len - 1;
    if (b.len > 0)
      {
	b.data = new char[b.len];
	if (data)
	  strcpy (b.data, data);
	else
	  b.data[0] = 0;
	if (a.data)
	  strcat (b.data, a.data);
      }
    return b;
  }

  bool operator == (const String & a) const
  {
    if (!a.len && !len)
      return 1;
    if (a.len != len)
      return 0;
    return (!strcmp (data, a.data));
  }

  bool operator!= (const String & a) const
  {
    return !(*this == a);
  }

  /** returns the content as char* */
  const char *operator  () () const
  {
    return data;
  }

};

/** adds a text to a string */
inline const String &
operator += (String & a, const String b)
{
  String c = a + b;
  return (a = c);
}

#endif
