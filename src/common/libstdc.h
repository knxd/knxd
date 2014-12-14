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

#ifndef LIBSTDC___H
#define LIBSTDC___H

#include <stdlib.h>

inline void operator
delete (void *addr)
{
  free (addr);
}

inline void operator   delete[] (void *addr)
{
  free (addr);
}

inline void *operator   new[] (size_t size)
{
  return operator   new (size);
}

inline void *operator
new (size_t size)
{
  return malloc (size);
}

asm volatile ("      .set   __cxa_pure_virtual,0");

#endif
