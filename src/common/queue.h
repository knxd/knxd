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

#ifndef QUEUE_H
#define QUEUE_H

#include <assert.h>
#include <queue>

/** implement a generic FIFO queue*/
template < typename _T >
class Queue : public std::queue<_T>
{
public:
  typedef typename std::queue<_T>::value_type value_type;

  /** initialize queue */
  Queue () : std::queue<_T>() {};

  /** destructor */
  virtual ~Queue () {}

  using std::queue<_T>::front;
  using std::queue<_T>::pop;
  using std::queue<_T>::push;

  inline void clear()
    {
      while (!isempty())
        pop();
    }

  inline void put (value_type && el)
    {
      std::queue<_T>::push(std::move(el));
    }

  inline _T get ()
    {
      value_type v = std::move(std::queue<_T>::front());
      std::queue<_T>::pop();
      return v;
    }

  /** return true, if the queue is empty */
  inline bool isempty () const
    {
      return std::queue<_T>::empty();
    }

};

#endif
