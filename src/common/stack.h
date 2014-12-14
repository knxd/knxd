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

#ifndef STACK_H
#define STACK_H

#include "array.h"

/** implement a generic LIFO stack*/
template < class T > class Stack
{
protected:
  /** elements in the stack*/
  Array < T > data;

public:

  /** initialize stack */
  Stack ()
  {
  }

  /** copy constructer */
  Stack (const Stack < T > &c)
  {
    data = c.data;
  }

  /** destructor */
  virtual ~ Stack ()
  {
  }

  /** assignment operator */
  const Stack < T > &operator = (const Stack < T > &c)
  {
    data = c.data;
    return c;
  }

  /** adds a element to the queue end */
  void push (const T & el)
  {
    data.resize (data () + 1);
    data[data () - 1] = el;
  }

  /** remove the element from the queue head and returns it */
  T pop ()
  {
    T a = data[data () - 1];
    data.resize (data () - 1);
    return a;
  }

  /** returns the element from the queue head */
  const T & top () const
  {
    return data[data () - 1];
  }

  /** return true, if the queue is empty */
  int isempty () const
  {
    return data () == 0;
  }

};

#endif
