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

#ifndef TPUART_TCP_H
#define TPUART_TCP_H
#include <netinet/in.h>
#include "eibnetip.h"
#include "layer2.h"
#include "lpdu.h"

/** TPUART user mode driver */
class TPUARTTCPLayer2Driver:public Layer2, private Thread
{
  /** TCP/IP connection */
  int fd;
  /** semaphore for inqueue */
  pth_sem_t in_signal;
  /** input queue */
  Queue < LPDU * >inqueue;
  /** output queue */
  bool ackallgroup;
  bool ackallindividual;
  bool dischreset;

  /** process a received frame */
  void RecvLPDU (const uchar * data, int len);
  void Run (pth_sem_t * stop);
  const char *Name() { return "tpuarts"; }
public:
  TPUARTTCPLayer2Driver (const char *dest, int port, L2options *opt);
  ~TPUARTTCPLayer2Driver ();
  bool init (Layer3 *l3);

  void Send_L_Data (LPDU * l);

  bool enterBusmonitor ();
  bool leaveBusmonitor ();

  bool Open ();
  bool Send_Queue_Empty ();
};

#endif
