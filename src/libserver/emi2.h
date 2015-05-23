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

#ifndef EIB_EMI2_H
#define EIB_EMI2_H

#include "layer2.h"
#include "lowlevel.h"

/** EMI2 backend */
class EMI2Layer2Interface:public Layer2Interface, private Thread
{
  /** driver to send/receive */
  LowLevelDriverInterface *iface;
  /** debug output */
  Trace *t;
  /** state */
  int mode;
  /** default address */
  eibaddr_t def;
  /** vbusmonitor */
  int vmode;
  /** semaphore for outqueue */
  pth_sem_t out_signal;
  /** semaphore for inqueue */
  pth_sem_t in_signal;
  /** output queue */
  Queue < LPDU * >outqueue;
  /** input queue */
  Queue < LPDU * >inqueue;
  /** event for outqueue*/
  pth_event_t getwait;
  bool noqueue;
  int sendmode;

  void Send (LPDU * l);
  void Run (pth_sem_t * stop);
  const char *Name() { return "emi2"; }
public:
  EMI2Layer2Interface (LowLevelDriverInterface * i, Layer3 * l3, int flags);
  ~EMI2Layer2Interface ();
  bool init ();

  void Send_L_Data (LPDU * l);
  LPDU *Get_L_Data (pth_event_t stop);

  bool addAddress (eibaddr_t addr);
  bool removeAddress (eibaddr_t addr);

  bool enterBusmonitor ();
  bool leaveBusmonitor ();
  bool openVBusmonitor ();
  bool closeVBusmonitor ();

  bool Open ();
  bool Close ();
  bool Connection_Lost ();
  bool Send_Queue_Empty ();
};

#endif
