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

/** implement a generic FIFO queue*/
template < class T > class Queue
{
protected:
  /** element in the queue */
  typedef struct _Entry
  {
    /** value */
    T entry;
    /** next element */
    struct _Entry *Next;
  } Entry;

  /** list head */
  Entry *akt;
  /** pointer where to store the pointer to the next element */
  Entry **head;

public:

  /** initialize queue */
  Queue ()
  {
    akt = 0;
    head = &akt;
  }

  /** copy constructer */
  Queue (const Queue < T > &c)
  {
    Entry *a = c->akt;
    akt = 0;
    head = &akt;
    while (a)
      {
	put (a->entry);
	a = a->Next;
      }
  }

  /** destructor */
  virtual ~ Queue ()
  {
    while (akt)
      get ();
  }

  /** assignment operator */
  const Queue < T > &operator = (const Queue < T > &c)
  {
    while (akt)
      get ();
    Entry *a = c.akt;
    while (a)
      {
	put (a->entry);
	a = a->Next;
      }
  }

  /** adds a element to the queue end */
  void put (const T & el)
  {
    Entry *elem = new Entry;
    elem->Next = 0;
    elem->entry = el;
    *head = elem;
    head = &elem->Next;
  }

  /** remove the element from the queue head and returns it */
  T get ()
  {
    assert (akt != 0);
    Entry *e = akt;
    T a = akt->entry;
    akt = akt->Next;
    delete e;
    if (!akt)
      head = &akt;
    return a;
  }

  /** returns the element from the queue head */
  const T & top () const
  {
    assert (akt != 0);
    return akt->entry;
  }

  /** return true, if the queue is empty */
  int isempty () const
  {
    return akt == 0;
  }

};

#endif
