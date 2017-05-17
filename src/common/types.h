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

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <string.h>

#include <vector>
#include <string>
#include <memory>

#include "config.h"

#define CONST const
#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED 
#endif

/** unsigned char */
typedef uint8_t uchar;

/** EIB address */
typedef uint16_t eibaddr_t;

/** EIB key */
typedef uint32_t eibkey_type;

/** legacy */
typedef std::string String;

#define ITER(_i,_t) for(decltype(_t)::iterator _i = _t.begin(); _i != _t.end(); _i++)
#define R_ITER(_i,_t) for(decltype(_t)::reverse_iterator _i = _t.rbegin(); _i != _t.rend(); _i++)

/** generic arrays don't need any new code */
#define Array std::vector

/** byte arrays however need some help.
  We can't use strings: strings can't contain null characters.
  */

inline unsigned int _sub(unsigned int _a, unsigned int _b) { return (_a>_b) ? _a-_b : 0; }
inline unsigned int _min(unsigned int _a, unsigned int _b) { return (_a<_b) ? _a : _b; }

typedef std::vector<uint8_t> u8vec; // less typing
class CArray : public u8vec
{
public:
  /** start with various initializers */
  CArray() : u8vec() { }
  CArray(const CArray& __str, size_type __pos) : u8vec(_sub(__pos,__str.size()))
  {
    CArray::iterator j = this->begin();
    const uint8_t *str = __str.data()+__pos;
    for(unsigned int i = this->size(); i > 0; i--,j++)
        *j = *str++;
  }
  CArray(const CArray& __str, size_type __pos, size_type __n) : u8vec(_min(__n,_sub(__pos,__str.size())))
  {
    CArray::iterator j = this->begin();
    const uint8_t *str = __str.data()+__pos;
    while(j != this->end())
        *j++ = *str++;
  }
  CArray(const uint8_t *__str, size_type __pos, size_type __n) : u8vec(__n)
  {
    CArray::iterator j = this->begin();
    __str += __pos;
    for(unsigned int i = __pos; j != this->end(); i++,j++)
        *j = *__str++;
  }
  CArray(const uint8_t *__str, size_type __n) : u8vec(__n)
  {
    CArray::iterator j = this->begin();
    while(j != this->end())
        *j++ = *__str++;
  }

  /** set me to a C array */
  void set (const uint8_t *elem, unsigned cnt)
  {
    this->resize(cnt);
    CArray::iterator j = this->begin();
    while(j != this->end())
        *j++ = *elem++;
  }

  /** copy content. Should be equivalent to operator= */
  void set (const CArray & a)
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
  void setpart (const uint8_t *elem, unsigned start, unsigned cnt)
  {
    if (cnt + start > size())
      resize (cnt + start);

    CArray::iterator i = this->begin()+start;
    while(cnt--)
        *i++ = *elem++;
  }

  /** setpart for a string. This copies the terminal null character, */
  inline void setpart (const std::string &str, unsigned start)
  {
    setpart((const uint8_t *)str.c_str(),start,str.length()+1);
  }

  /** why doesn't std::vector have this?? */
  void operator+= (const CArray &a)
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

  /** delete content.
   * Like .erase(), but with random indices instead of * valid iterators.
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

  /** add a byte to the add */
  inline void add (const uint8_t elem)
  {
    this->push_back(elem);
  }
};

template <typename To, typename From>
std::unique_ptr<To>
dynamic_unique_cast(std::unique_ptr<From>&& p)
{
  if (To* cast = dynamic_cast<To*>(p.get()))
    {
      std::unique_ptr<To> result(cast);
      p.release();
      return result;
    }
  return std::unique_ptr<To>(nullptr); // or throw std::bad_cast() if you prefer
}

#endif
