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
LowLevelAdapter::~LowLevelAdapter() {}

LowLevelDriver::~LowLevelDriver()
{
  local_timeout.stop();
}

LowLevelFilter::~LowLevelFilter()
{
  delete iface;
}

void
LowLevelDriver::send_Local(CArray &d, bool raw)
{
  assert(!is_local);
  assert (!raw);
  is_local = true;
  local_timeout.start(0.9, 0);
  do_send_Local(d, raw);
  while(is_local)
    ev_run(EV_DEFAULT_ EVRUN_ONCE);
}

void
LowLevelFilter::do_send_Local(CArray &d, bool raw)
{
  if (raw)
    iface->send_Data(d);
  else
    send_Data(d);
}

void
LowLevelDriver::local_timeout_cb(ev::timer &w UNUSED, int revents UNUSED)
{
  ERRORPRINTF (t, E_ERROR, "send_Local timed out!");
  abort_send();
  is_local = false;
}

void
LowLevelDriver::send_Next()
{
  if (is_local)
    {
      local_timeout.stop();
      is_local = false;
    }
  else
    master->send_Next();
}

void
LowLevelAdapter::send_L_Data(LDataPtr l)
{
  if (!iface)
    {
      ERRORPRINTF (t, E_ERROR, "Send: not running??");
      stopped();
      return;
    }
  iface->send_L_Data(std::move(l));
}

