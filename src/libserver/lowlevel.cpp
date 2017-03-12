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

#include "lowlevel.h"

LowLevelIface::~LowLevelIface() {}
LowLevelDriver::~LowLevelDriver() {}
LowLevelFilter::~LowLevelFilter() { delete iface; }
LowLevelAdapter::~LowLevelAdapter() {}

void
LowLevelDriver::send_Local(CArray &d)
{
  assert(!is_local);
  is_local = true;
  send_Data(d);
  while(is_local)
    ev_run(EV_DEFAULT_ EVRUN_ONCE);
  // TODO timer?
}

void
LowLevelDriver::send_Next()
{
  if (is_local)
    is_local = false;
  else
    master->send_Next();
}
