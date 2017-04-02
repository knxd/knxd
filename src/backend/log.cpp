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
  auto cn = conn.lock();
  if (cn == nullptr)
    return false;
  log_state = cfg->value("state",true);
  log_send = cfg->value("send",true);
  log_recv = cfg->value("recv",true);
  log_addr = cfg->value("addr",false);
  log_monitor = cfg->value("monitor",false);
  if (log_state)
    t->TracePrintf (0, "State setup");
  return true;
}

void
LogFilter::start()
{
  if (log_state)
    t->TracePrintf (0, "State start");
  Filter::start();
}

void
LogFilter::stop()
{
  if (log_state)
    t->TracePrintf (0, "State stop");
  Filter::stop();
}

void
LogFilter::started()
{
  if (log_state)
    t->TracePrintf (0, "State started");
  Filter::started();
}

void
LogFilter::stopped()
{
  if (log_state)
    t->TracePrintf (0, "State stopped");
  Filter::stopped();
}

void
LogFilter::errored()
{
  if (log_state)
    t->TracePrintf (0, "State errored");
  Filter::errored();
}


void
LogFilter::recv_L_Data (LDataPtr l)
{
  if (log_recv)
    t->TracePrintf (0, "Recv %s", l->Decode (t));
  Filter::recv_L_Data(std::move(l));
}

void
LogFilter::send_Next()
{
  if (log_state)
    t->TracePrintf (6, "send next");
  Filter::send_Next();
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

bool
LogFilter::hasAddress (eibaddr_t addr)
{
  bool res = Filter::hasAddress(addr);
  if (log_addr)
    t->TracePrintf (0, "Has Addr %s: %s",
        FormatEIBAddr(addr), res ? "yes" : "no");
  return res;
}

void
LogFilter::addAddress (eibaddr_t addr)
{
  Filter::addAddress(addr);
  if (log_addr)
    t->TracePrintf (0, "Add Addr %s", FormatEIBAddr(addr));
}

bool
LogFilter::checkAddress (eibaddr_t addr)
{
  bool res = Filter::checkAddress(addr);
  if (log_addr)
    t->TracePrintf (0, "Addr Check %s: %s",
        FormatEIBAddr(addr), res ? "yes" : "no");
  return res;
}

bool
LogFilter::checkGroupAddress (eibaddr_t addr)
{
  bool res = Filter::checkGroupAddress(addr);
  if (log_addr)
    t->TracePrintf (0, "Addr Check %s: %s",
        FormatGroupAddr(addr), res ? "yes" : "no");
  return res;
}

