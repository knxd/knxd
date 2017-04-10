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

#ifndef FILTER_H
#define FILTER_H
#include "link.h"
#include "lowlevel.h"

FILTER(LogFilter,log)
{
  bool log_send;
  bool log_recv;
  bool log_monitor;
  bool log_state;
  bool log_addr;

public:
  LogFilter (const LinkConnectPtr_& c, IniSectionPtr& s) : Filter(c,s) {}
  virtual ~LogFilter ();

  virtual bool setup();
  virtual void recv_L_Data (LDataPtr l);
  virtual void send_L_Data (LDataPtr l);
  virtual void recv_L_Busmonitor (LBusmonPtr l);
  virtual void send_Next();

  virtual void start();
  virtual void stop();
  virtual void started();
  virtual void stopped();
  virtual void errored();

  virtual bool hasAddress (eibaddr_t addr);
  virtual void addAddress (eibaddr_t addr);
  virtual bool checkAddress (eibaddr_t addr);
  virtual bool checkGroupAddress (eibaddr_t addr);
};

class LLlog : public LowLevelFilter
{
public:
  LLlog (LowLevelIface* parent, IniSectionPtr& s, LowLevelDriver* i = nullptr);
  virtual ~LLlog();

  virtual FilterPtr findFilter(std::string name);
  virtual bool checkAddress(eibaddr_t addr);
  virtual bool checkGroupAddress(eibaddr_t addr);
  virtual bool checkSysAddress(eibaddr_t addr);
  virtual bool checkSysGroupAddress(eibaddr_t addr);

  virtual bool setup();
  virtual void start();
  virtual void stop();
  virtual void started();
  virtual void stopped();
  virtual void errored();
  virtual void recv_L_Data(LDataPtr l);
  virtual void recv_L_Busmonitor(LBusmonPtr l);
  virtual void send_L_Data(LDataPtr l);
  virtual void send_Data(CArray& c);
  virtual void recv_Data(CArray& c);

  /** sends a EMI frame asynchronous */
  virtual void sendReset();
  virtual void abort_send();

  virtual void do_send_Next();
  virtual void send_Local (CArray& l, int raw = 0);
  virtual void do_send_Local (CArray& l, int raw = 0);
};

#endif
