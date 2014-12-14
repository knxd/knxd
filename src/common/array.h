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

#ifndef ARRAY_H
#define ARRAY_H

/** implements a generic array */
template < class T > class Array
{
protected:
  /** pointer to the data */
  T * data;
  /** element count */
  unsigned count;
public:
  /** create empty array */
  Array ()
  {
    data = 0;
    count = 0;
  }
  /** destructor */
  virtual ~ Array ()
  {
    if (data)
      delete[]data;
  }
  /** copy constructor */
  Array (const Array < T > &a)
  {
    count = a.count;
    data = 0;
    if (count)
      {
	data = new T[count];
	for (unsigned i = 0; i < count; i++)
	  data[i] = a.data[i];
      }
  }

  /** create array from old style array
   * @param elem pointer to elements
   * @param cnt element count
   */
  Array (const T elem[], unsigned cnt)
  {
    data = 0;
    count = 0;
    set (elem, cnt);
  }

  /** set content to them of a old style array
   * @param elem pointer to elements
   * @param cnt element count
   */
  void set (const T elem[], unsigned cnt)
  {
    unsigned i;
    resize (cnt);
    for (i = 0; i < cnt; i++)
      data[i] = elem[i];
  }

  /** copy content of an array */
  void set (const Array & a)
  {
    set (a.array (), a ());
  }

  /** replace a part of the array and resize to fit
   * @param elem pointer to new elements
   * @param start start position
   * @param cnt element count
   */
  void setpart (const T elem[], unsigned start, unsigned cnt)
  {
    unsigned i;
    if (cnt + start > count)
      resize (cnt + start);
    for (i = 0; i < cnt; i++)
      data[i + start] = elem[i];
  }

  /** replace a part of the array with the content of a and resize to fit
   * @param start start index
   * @param a new elements
   */
  void setpart (const Array < T > &a, unsigned start)
  {
    setpart (a.array (), start, a ());
  }

  /** delete content
   * @param start start index
   * @param cnt element count
   */
  void deletepart (unsigned start, unsigned cnt)
  {
    if (start >= count)
      return;
    if (cnt <= 0)
      return;
    if (start + cnt >= count)
      cnt -= start + cnt - count;
    for (unsigned i = start + cnt; i < count; i++)
      data[i - cnt] = data[i];
    resize (count - cnt);
  }

  /** assignement operator */
  const Array & operator = (const Array & a)
  {
    if (data)
      delete[]data;
    count = a.count;
    data = 0;
    if (count)
      {
	data = new T[count];
	for (unsigned i = 0; i < count; i++)
	  data[i] = a.data[i];
      }
    return a;
  }

  /** compare array elementwise for equal */
  bool operator== (const Array & a)
  {
    if (a () != count)
      return 0;
    for (unsigned i = 0; i < count; i++)
      if (a[i] != data[i])
	return 0;
    return 1;
  }

  /** compare array elementwise for not equal */
  bool operator!= (const Array & a)
  {
    return !(*this == a);
  }

  /** returns pointer to the elements */
  const T *array () const
  {
    return data;
  }

  /** returns pointer to the elements */
  T *array ()
  {
    return data;
  }

  /** resize array to newcount elements */
  void resize (unsigned newcount)
  {
    if (!newcount)
      {
	if (data)
	  delete[]data;
	data = 0;
	count = 0;
	return;
      }
    T *d1 = new T[newcount];
    for (unsigned i = 0; i < (count < newcount ? count : newcount); i++)
      d1[i] = data[i];
    if (data);
    delete[]data;
    data = d1;
    count = newcount;
  }

  /** insert elem at pos */
  void insert (unsigned pos, const T & elem)
  {
    if (pos >= count)
      {
	add (elem);
	return;
      }

    T *d1 = new T[count + 1];
    for (unsigned i = 0; i < pos; i++)
      d1[i] = data[i];
    data[pos] = elem;
    for (unsigned i = pos; i < count; i++)
      d1[i + 1] = data[i];
    if (data);
    delete[]data;
    data = d1;
    count = count + 1;
  }

  /** add element elem to the add */
  void add (const T & elem)
  {
    resize (count + 1);
    operator[](count - 1) = elem;
  }
  /** access random element in standart C notation */
  T & operator[](unsigned elem)
  {
    //no boundcheck
    return data[elem];
  }
  /** access random element in standart C notation */
  const T & operator[] (unsigned elem) const
  {
    //kein Boundcheck
    return data[elem];
  }
  /** returns element count */
  unsigned operator  () () const
  {
    return count;
  }
  /** return element count */
  unsigned len () const
  {
    return count;
  }

  void sort ()
  {
    for (int i = 0; i < count; i++)
      for (int j = i + 1; j < count; j++)
	if (data[i] > data[j])
	  {
	    T x = data[i];
	    data[i] = data[j];
	    data[j] = x;
	  }
  }
};

#endif
