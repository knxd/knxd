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

#ifndef EIB_FT12_H
#define EIB_FT12_H

#include <termios.h>

#include "threads.h"
#include "lowlevel.h"
#include "lowlatency.h"

/** FT1.2 lowlevel driver*/
class FT12LowLevelDriver:public LowLevelDriverInterface, private Thread
{
  /** old serial config */
  low_latency_save sold;
  /** file descriptor */
  int fd;
  /** saved termios */
  struct termios old;
  /** send state */
  int sendflag;
  /** recevie state */
  int recvflag;
  /** debug output */
  Trace *t;
  /** semaphore for inqueue */
  pth_sem_t in_signal;
  /** semaphore for outqueue */
  pth_sem_t out_signal;
  /** input queue */
    Queue < CArray > inqueue;
    /** output queue */
    Queue < CArray * >outqueue;
    /** frame in receiving */
  CArray akt;
  /** repeatcount of the transmitting frame */
  int repeatcount;
  /** state */
  int mode;
  /** event for waiting on outqueue */
  pth_event_t getwait;
  /** semaphore to signal that inqueu is empty */
  pth_sem_t send_empty;

  void Run (pth_sem_t * stop);
public:
    FT12LowLevelDriver (const char *device, Trace * tr);
   ~FT12LowLevelDriver ();
  bool init ();

  void Send_Packet (CArray l);
  bool Send_Queue_Empty ();
  pth_sem_t *Send_Queue_Empty_Cond ();
  CArray *Get_Packet (pth_event_t stop);
  void SendReset ();
  bool Connection_Lost ();
  EMIVer getEMIVer ();
};

#endif
