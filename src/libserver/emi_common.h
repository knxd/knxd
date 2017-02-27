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

/** EMI common backend code */
class EMI_Common:public BusDriver
{
protected:
  /** driver to send/receive */
  LowLevelDriver *iface;
  /** input queue */
  Queue < LDataPtr >send_q;
  float send_delay;

  void Send (LDataPtr l);

  virtual void cmdEnterMonitor() = 0;
  virtual void cmdLeaveMonitor() = 0;
  virtual void cmdOpen() = 0;
  virtual void cmdClose() = 0;
  virtual const uint8_t * getIndTypes() = 0;
private:
  bool wait_confirm = false;
  bool monitor = false;

  ev::async trigger;
  void trigger_cb (ev::async &w, int revents);

  ev::timer timeout;
  void timeout_cb(ev::timer &w, int revents);

  void read_cb(CArray *p);

public:
  EMI_Common (const LinkConnectPtr_& c, IniSection& s);
  EMI_Common (LowLevelDriver * i, const LinkConnectPtr_& c, IniSection& s);
  virtual ~EMI_Common ();
  bool setup();
  void start();
  void stop();

  void send_L_Data (LDataPtr l);

  virtual CArray lData2EMI (uchar code, const LDataPtr &p)
  { return L_Data_ToEMI(code, p); }
  virtual LDataPtr EMI2lData (const CArray & data)
  { return EMI_to_L_Data(data, t); }

  virtual unsigned int maxPacketLen() { return 0x10; }
};

typedef std::shared_ptr<EMI_Common> EMIPtr;

#endif
