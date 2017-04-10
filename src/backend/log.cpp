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


LLlog::LLlog (LowLevelIface* parent, IniSectionPtr& s, LowLevelDriver* i) : LowLevelFilter(parent,s,i)
{
  tr()->setAuxName("log");
  tr()->TracePrintf (0, "Insert %d:%s / %d:%s", iface ? iface->tr()->seq : 0, iface ? iface->tr()->auxname : "??", parent->tr()->seq, parent->tr()->auxname);
}

LLlog::~LLlog()
{
  tr()->TracePrintf (0, "Closing");
}

void LLlog::do_send_Next()
{
  tr()->TracePrintf (0, "send_Next");
  master->send_Next();
}

FilterPtr LLlog::findFilter(std::string name)
{
  FilterPtr x = master->findFilter(name);
  tr()->TracePrintf (0, "Filter %s %sfound",name,x?"":"not ");
  return x;
}
bool LLlog::checkAddress(eibaddr_t addr)
{
  bool x = master->checkAddress(addr);
  tr()->TracePrintf (0, "Has Addr %s: %s",
      FormatEIBAddr(addr), x ? "yes" : "no");
  return x;
}
bool LLlog::checkGroupAddress(eibaddr_t addr)
{
  bool x = master->checkGroupAddress(addr);
  tr()->TracePrintf (0, "Has Addr %s: %s",
      FormatGroupAddr(addr), x ? "yes" : "no");
  return x;
}
bool LLlog::checkSysAddress(eibaddr_t addr)
{
  bool x = master->checkSysAddress(addr);
  tr()->TracePrintf (0, "Known Addr %s: %s",
      FormatEIBAddr(addr), x ? "yes" : "no");
  return x;
}
bool LLlog::checkSysGroupAddress(eibaddr_t addr)
{
  bool x = master->checkSysGroupAddress(addr);
  tr()->TracePrintf (0, "Known Addr %s: %s",
      FormatGroupAddr(addr), x ? "yes" : "no");
  return x;
}

bool LLlog::setup ()
{
  bool x;
  tr()->TracePrintf (0, "Setup");
  x = iface->setup();
  tr()->TracePrintf (0, "Setup OK: %s", x?"yes":"no");
  return x;
}

void LLlog::start ()
{
  tr()->TracePrintf (0, "Start");
  iface->start();
}

void LLlog::stop ()
{
  tr()->TracePrintf (0, "Stop");
  iface->stop();
}

void LLlog::started ()
{
  tr()->TracePrintf (0, "Started");
  master->started();
}

void LLlog::stopped ()
{
  tr()->TracePrintf (0, "Stopped");
  master->stopped();
}

void LLlog::errored ()
{
  tr()->TracePrintf (0, "Errored");
  master->errored();
}

void LLlog::recv_L_Data(LDataPtr l)
{
  tr()->TracePrintf (0, "Recv %s", l->Decode (t));
  master->recv_L_Data(std::move(l));
}

void LLlog::send_L_Data(LDataPtr l)
{
  tr()->TracePrintf (0, "Send %s", l->Decode (t));
  iface->send_L_Data(std::move(l));
}

void LLlog::recv_Data(CArray& l)
{
  tr()->TracePacket (0, "Recv", l);
  master->recv_Data(l);
}

void LLlog::send_Data(CArray& l)
{
  tr()->TracePacket (0, "Send", l);
  iface->send_Data(l);
}

void LLlog::send_Local(CArray& l, int raw)
{
  char x[15];
  sprintf(x,"SendLocal %d",raw);
  tr()->TracePacket (0, x, l);
  iface->send_Local(l,raw);
}

void LLlog::do_send_Local(CArray& l, int raw)
{
  char x[15];
  sprintf(x,"DoSendLocal %d",raw);
  tr()->TracePacket (0, x, l);
  iface->do_send_Local(l,raw);
}

void LLlog::recv_L_Busmonitor(LBusmonPtr l)
{
  tr()->TracePrintf (0, "Monitor %s", l->Decode (tr()));
  master->recv_L_Busmonitor(std::move(l));
}

void LLlog::sendReset()
{
  tr()->TracePrintf (0, "Reset");
  iface->sendReset();
}

void LLlog::abort_send()
{
  tr()->TracePrintf (0, "AbortSend");
  iface->abort_send();
}

