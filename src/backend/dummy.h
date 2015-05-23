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

#ifndef DUMMY_H
#define DUMMY_H
#include <termios.h>
#include "lowlatency.h"
#include "layer2.h"
#include "lpdu.h"

/** TPUART user mode driver */
class DummyL2Driver:public Layer2Interface, private Thread
{
  /** state */
  int mode;
  /** vbusmonitor mode */
  int vmode;
  /** semaphore for inqueue */

  /** output queue (for bus "monitoring") */
  Queue < LPDU * >outqueue;
  /** signal for the outqueue */
  pth_sem_t outsignal;
  /** event to wait for outqueue */
  pth_event_t getwait;

  void Run (pth_sem_t * stop);
public:
  DummyL2Driver (Layer3 *l3);
  ~DummyL2Driver ();

  void Send_L_Data (LPDU * l);
  LPDU *Get_L_Data (pth_event_t stop);

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
