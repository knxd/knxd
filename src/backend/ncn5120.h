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

#ifndef SNCN5120_SERIAL_H
#define SNCN5120_SERIAL_H
#include <termios.h>
#include "lowlatency.h"
#include "layer2.h"

/** TPUART user mode driver */
class NCN5120SerialLayer2Driver : public Layer2Interface, private Thread
{
  /** old serial config */
  low_latency_save sold;
  /** old termios state */
  struct termios old;
  /** file descriptor */
  int fd;
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
  bool ackallgroup;
  bool ackallindividual;
  bool dischreset;

  /** process a recevied frame */
  void RecvLPDU (const uchar * data, int len);
  void Run (pth_sem_t * stop);
public:
  NCN5120SerialLayer2Driver (const char *dev, eibaddr_t addr, int flags,
                             Layer3 *l3);
  ~NCN5120SerialLayer2Driver ();
  bool init ();

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
