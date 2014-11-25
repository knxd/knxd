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

#ifndef TPUART_SERIAL_H
#define TPUART_SERIAL_H
#include <termios.h>
#include "lowlatency.h"
#include "layer2.h"

/** TPUART user mode driver */
class TPUARTSerialLayer2Driver:public Layer2Interface, private Thread
{
  /** old serial config */
  low_latency_save sold;
  /** old termios state */
  struct termios old;
  /** file descriptor */
  int fd;
  /** debug output */
  Trace *t;
  /** default EIB address */
  eibaddr_t addr;
  /** state */
  int mode;
  /** vbusmonitor mode */
  int vmode;
  /** semaphore for inqueue */
  pth_sem_t in_signal;
  /** semaphore for outqueue */
  pth_sem_t out_signal;
  /** input queue */
    Queue < LPDU * >inqueue;
    /** output queue */
    Queue < LPDU * >outqueue;
    /** event to wait for outqueue */
  pth_event_t getwait;
  /** my individual addresses */
    Array < eibaddr_t > indaddr;
    /** my group addresses */
    Array < eibaddr_t > groupaddr;
  bool ackallgroup;
  bool ackallindividual;
  bool dischreset;

    /** process a recevied frame */
  void RecvLPDU (const uchar * data, int len);
  void Run (pth_sem_t * stop);
public:
    TPUARTSerialLayer2Driver (const char *dev, eibaddr_t addr, int flags,
			      Trace * tr);
   ~TPUARTSerialLayer2Driver ();
  bool init ();

  void Send_L_Data (LPDU * l);
  LPDU *Get_L_Data (pth_event_t stop);

  bool addAddress (eibaddr_t addr);
  bool addGroupAddress (eibaddr_t addr);
  bool removeAddress (eibaddr_t addr);
  bool removeGroupAddress (eibaddr_t addr);

  bool enterBusmonitor ();
  bool leaveBusmonitor ();
  bool openVBusmonitor ();
  bool closeVBusmonitor ();

  bool Open ();
  bool Close ();
  eibaddr_t getDefaultAddr ();
  bool Connection_Lost ();
  bool Send_Queue_Empty ();
};

#endif
