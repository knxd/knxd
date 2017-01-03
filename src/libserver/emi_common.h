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

#include "layer2.h"
#include "lowlevel.h"
#include "emi.h"
#include "ev++.h"

typedef enum {
    I_CONFIRM = 0,
    I_DATA = 1,
    I_BUSMON = 2,
} indTypes;

/** EMI common backend code */
class EMI_Common:public Layer2
{
protected:
  /** driver to send/receive */
  LowLevelDriver *iface;
  /** input queue */
  Queue < LPDU * >send_q;
  float send_delay;

  void Send (L_Data_PDU * l);
  virtual const char *Name() = 0;

  virtual void cmdEnterMonitor() = 0;
  virtual void cmdLeaveMonitor() = 0;
  virtual void cmdOpen() = 0;
  virtual void cmdClose() = 0;
  virtual const uint8_t * getIndTypes() = 0;
private:
  bool wait_confirm = false;

  ev::async trigger;
  void trigger_cb (ev::async &w, int revents);

  ev::timer timeout;
  void timeout_cb(ev::timer &w, int revents);

  void on_recv_cb(CArray *p);

public:
  EMI_Common (LowLevelDriver * i, L2options *opt);
  ~EMI_Common ();
  bool init (Layer3 *l3);

  void Send_L_Data (L_Data_PDU * l);

  virtual bool enterBusmonitor ();
  bool leaveBusmonitor ();

  virtual CArray lData2EMI (uchar code, const L_Data_PDU & p)
  { return L_Data_ToEMI(code, p); }
  virtual L_Data_PDU *EMI2lData (const CArray & data, Layer2Ptr l2)
  { return EMI_to_L_Data(data,l2); }

  virtual unsigned int maxPacketLen() { return 0x10; }
  bool Open ();
  bool Close ();
};

#endif
