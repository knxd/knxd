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

#ifndef EIB_BCU1_H
#define EIB_BCU1_H

#include "lowlevel.h"

/** PEI16 / BCU1 kernel driver backend */
class BCU1DriverLowLevelDriver:public LowLevelDriverInterface, private Thread
{
  /** file descriptor */
  int fd;
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
    /** event to wait for outqueue */
  pth_event_t getwait;
  /** semaphore to signal empty sendqueue */
  pth_sem_t send_empty;
    /** event to wait for send finished */
  pth_event_t send_done;

  void Run (pth_sem_t * stop);
public:
    BCU1DriverLowLevelDriver (const char *device, Trace * tr);
   ~BCU1DriverLowLevelDriver ();
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
