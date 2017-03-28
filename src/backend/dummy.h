/*
    EIBD eib bus access and management daemon
    Copyright (C) 2015 Matthias Urlichs <matthias@urlichs.de>

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

#ifndef DUMMY_H
#define DUMMY_H
#include "link.h"

/** dummy L2 driver, does nothing */
DRIVER(DummyL2Driver,dummy)
{
public:
  DummyL2Driver (const LinkConnectPtr_& c, IniSectionPtr& s) : BusDriver(c,s) {}
  virtual ~DummyL2Driver ();

  void send_L_Data (LDataPtr l UNUSED) { send_Next(); }
  bool setup()
    {
      if (!BusDriver::setup())
        return false;
      return true;
    }
};

/** dummy L2 filter, is transparent */
FILTER(DummyL2Filter,dummy)
{
public:
  DummyL2Filter (const LinkConnectPtr_& c, IniSectionPtr& s) : Filter(c,s) {}
  virtual ~DummyL2Filter ();
};

#endif
