/*
    EIBD eib bus monitor filter
    Copyright (C) 2020 MagicBear <mb@bilibili.com>

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

#ifndef MONITOR_H
#define MONITOR_H
#include "link.h"

/** monitor L2 filter, is transparent */
FILTER(MonitorL2Filter,monitor)
{
  bool mon_send;
  bool mon_recv;

public:
  MonitorL2Filter (const LinkConnectPtr_& c, IniSectionPtr& s) : Filter(c,s) {}
  virtual ~MonitorL2Filter () = default;
  
  virtual bool setup();
  virtual void recv_L_Data (LDataPtr l);
  virtual void send_L_Data (LDataPtr l);
  virtual void recv_L_Busmonitor (LBusmonPtr l);
};

#endif
