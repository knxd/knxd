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

#include "log.h"

LogFilter::~LogFilter() { }

bool
LogFilter::setup()
{
  if (!Filter::setup())
    return false;
  t->name = cfg.value("name",cfg.name);
  log_send = cfg.value("send",true);
  log_recv = cfg.value("recv",true);
  log_monitor = cfg.value("monitor",false);
  return true;
}

void
LogFilter::recv_L_Data (LDataPtr l)
{
  if (log_recv)
    t->TracePrintf (0, "Recv %s", l->Decode (t));
  Filter::recv_L_Data(std::move(l));
}

void
LogFilter::send_L_Data (LDataPtr l)
{
  if (log_send)
    t->TracePrintf (0, "Send %s", l->Decode (t));
  Filter::send_L_Data(std::move(l));
}

void
LogFilter::recv_L_Busmonitor (LBusmonPtr l)
{
  if (log_monitor)
    t->TracePrintf (0, "Monitor %s", l->Decode (t));
  Filter::recv_L_Busmonitor(std::move(l));
}

