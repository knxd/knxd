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

#ifndef EIB_EMI_COMMON_H
#define EIB_EMI_COMMON_H

#include "link.h"
#include "router.h"
#include "lowlevel.h"
#include "emi.h"
#include "ev++.h"

typedef enum {
    I_CONFIRM = 0,
    I_DATA = 1,
    I_BUSMON = 2,
} indTypes;

typedef enum
{
  vERROR,
  vEMI1 = 1,
  vEMI2 = 2,
  vCEMI = 3,
  vRaw,
  vUnknown,
  vDiscovery,
  vTIMEOUT
} EMIVer;

typedef enum
{
  E_idle,
  E_timed_out, // but still waiting for sendNext
  // states after this mean that the timer is running
  E_wait, // waiting for sendNext and confirm
  E_wait_confirm, // sendNext did arrive but not the HL confirmation
} E_state;

EMIVer cfgEMIVersion(IniSectionPtr& s);

/** EMI common backend code */
class EMI_Common:public LowLevelFilter
{
protected:
  /** driver to send/receive */
  float send_timeout; // max wait for confirmation
  int max_retries;

  virtual void cmdEnterMonitor() = 0;
  virtual void cmdLeaveMonitor() = 0;
  virtual void cmdOpen() = 0;
  virtual void cmdClose() = 0;
  virtual const uint8_t * getIndTypes() = 0;
  virtual EMIVer getVersion() = 0;
private:
  E_state state;
  bool wait_confirm = false; // waiting for high-level cinfirm
protected:
  bool wait_confirm_low = false; // waiting for low_level confirm
private:
  bool monitor = false;

  ev::timer timeout;
  void timeout_cb(ev::timer &w, int revents);
  CArray out; // caches the packet to send
  int retries;

  void read_cb(CArray *p);

public:
  EMI_Common (LowLevelIface* c, IniSectionPtr& s, LowLevelDriver *i = nullptr);
  virtual ~EMI_Common ();
  bool setup();
  void start();
  void started();
  void stop();

  void send_L_Data (LDataPtr l);
  void do_send_Next();

  virtual CArray lData2EMI (uchar code, const LDataPtr &p)
  { return L_Data_ToEMI(code, p); }
  virtual LDataPtr EMI2lData (const CArray & data)
  { return EMI_to_L_Data(data, t); }

  virtual unsigned int maxPacketLen() { return 0x10; }

  void recv_Data(CArray& c);
  void send_Data(CArray& c);
};

typedef std::shared_ptr<EMI_Common> EMIPtr;

#endif
