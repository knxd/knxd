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

#ifndef EIB_EMI1_H
#define EIB_EMI1_H

#include "layer2.h"
#include "lowlevel.h"

/** EMI1 backend */
class EMI1Layer2:public Layer2, private Thread
{
  /** driver to send/receive */
  LowLevelDriver *iface;
  /** semaphore for inqueue */
  pth_sem_t in_signal;
  /** input queue */
  Queue < LPDU * >inqueue;
  bool noqueue;

  void Send (LPDU * l);
  void Run (pth_sem_t * stop);
  const char *Name() { return "emi1"; }
public:
  EMI1Layer2 (LowLevelDriver * i, L2options *opt);
  ~EMI1Layer2 ();
  bool init (Layer3 *l3);

  void Send_L_Data (LPDU * l);

  bool enterBusmonitor ();
  bool leaveBusmonitor ();

  bool Open ();
  bool Close ();
  bool Send_Queue_Empty ();
};

#endif
