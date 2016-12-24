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

#ifndef TPUART_H
#define TPUART_H
#include <termios.h>
#include "lowlatency.h"
#include "layer2.h"
#include "lpdu.h"

/** TPUART user mode driver */
class TPUART_Base:public Layer2, protected Thread
{
protected:
  /** file descriptor */
  int fd;
  /** semaphore for inqueue */
  pth_sem_t in_signal;
  /** input queue */
  Queue < LPDU * >inqueue;
  /** output queue */
  bool ackallgroup;
  bool ackallindividual;
  bool dischreset;

  /** process a recevied frame */
  void RecvLPDU (const uchar * data, int len);
  void Run (pth_sem_t * stop);
  const char *Name() = 0;
  virtual void reset_dtr() = {};
public:
  TPUART_Base (L2options *opt);
  ~TPUART_Base ();
  bool init (Layer3 *l3);

  void Send_L_Data (LPDU * l);

  bool enterBusmonitor ();
  bool leaveBusmonitor ();

  bool Open ();
  bool Send_Queue_Empty ();
};

#endif
