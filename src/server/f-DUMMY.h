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

#ifndef F_DUMMY_H
#define F_DUMMY_H

#include "dummy.h"
#include "layer23.h"

#define DUMMY_F_URL "dummy\n"
#define DUMMY_F_DOC "dummy is a transparent filter, used for testing\n\n"
#define DUMMY_F_PREFIX "dummy"
#define DUMMY_F_FILTER dummy_Filter

inline Layer2Ptr
dummy_Filter (const char *dev UNUSED, L2options *opt, Layer2Ptr l2)
{
  return std::shared_ptr<DummyL2Filter>(new DummyL2Filter (opt, l2));
}

#endif
