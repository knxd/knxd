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

#include <vector>

#define ITER(_i,_t) for(decltype(_t)::iterator _i = _t.begin(); _i != _t.end(); _i++)

/** implements a generic array */
#define Array std::vector

inline unsigned int _sub(unsigned int _a, unsigned int _b) { return (_a>_b) ? _a-_b : 0; }
inline unsigned int _min(unsigned int _a, unsigned int _b) { return (_a<_b) ? _a : _b; }

typedef std::vector<uint8_t> u8str;
class CArray : public u8str
{
public:
  CArray() : u8str() { }
  CArray(const CArray& __str, size_type __pos) : u8str(_sub(__pos,__str.size()))
  {
    CArray::iterator j = this->begin();
    const uint8_t *str = __str.data()+__pos;
    for(unsigned int i = this->size(); i > 0; i--,j++)
        *j = *str++;
  }
  CArray(const CArray& __str, size_type __pos, size_type __n) : u8str(_min(__n,_sub(__pos,__str.size())))
  {
    CArray::iterator j = this->begin();
    const uint8_t *str = __str.data()+__pos;
    while(j != this->end())
        *j++ = *str++;
  }
  CArray(const uint8_t *__str, size_type __pos, size_type __n) : u8str(__n)
  {
    CArray::iterator j = this->begin();
    __str += __pos;
    for(unsigned int i = __pos; j != this->end(); i++,j++)
        *j = *__str++;
  }
  CArray(const uint8_t *__str, size_type __n) : u8str(__n)
  {
    CArray::iterator j = this->begin();
    while(j != this->end())
        *j++ = *__str++;
  }

  inline void set (const uint8_t *elem, unsigned cnt)
  {
    this->resize(cnt);
    CArray::iterator j = this->begin();
    while(j != this->end())
        *j++ = *elem++;
  }

  /** copy content of an array */
  inline void set (const CArray & a)
  {
    this->resize(a.size());
    CArray::const_iterator i = a.begin();
    CArray::iterator j = this->begin();
    while (i != a.end())
      *j++ = *i++;
  }

  /** replace a part of the array and resize to fit
   * @param elem pointer to new elements
   * @param start start position
   * @param cnt element count
   */
  inline void setpart (const uint8_t *elem, unsigned start, unsigned cnt)
  {
    if (cnt + start > size())
      resize (cnt + start);

    CArray::iterator i = this->begin()+start;
    while(cnt--)
        *i++ = *elem++;
  }

  inline void operator+= (const CArray &a)
  {
    unsigned int off = this->size();
    resize(this->size()+a.size());

    CArray::const_iterator i = a.begin();
    CArray::iterator j = this->begin()+off;
    while (i != a.end())
      *j++ = *i++;

  }

  /** replace a part of the array with the content of a and resize to fit
   * @param start start index
   * @param a new elements
   */
  inline void setpart (const CArray &a, unsigned start)
  {
    setpart (a.data(), start, a.size());
  }

  /** delete content
   * @param start start index
   * @param cnt element count
   */
  inline void deletepart (unsigned start, unsigned cnt = 1)
  {
    if (start >= size())
      return;
    if (start+cnt >= size())
        cnt = size()-start;
    if (cnt == 0)
      return;
    erase (this->begin()+start,this->begin()+start+cnt);
  }

  /** add element elem to the add */
  inline void add (const uint8_t elem)
  {
    this->push_back(elem);
  }
};

#endif
