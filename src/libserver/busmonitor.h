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

#ifndef BUSMONITOR_H
#define BUSMONITOR_H

#include "layer3.h"
#include "client.h"

/** implements busmonitor functions for a client */
class A_Busmonitor:public L_Busmonitor_CallBack, private Thread
{
  /** semaphore for the input queue */
  pth_sem_t sem;
  /** input queue */
  Queue < L_Busmonitor_PDU * >data;
  /** is virtual busmonitor */
  bool v;
  /** should provide timestamps */
  bool ts;

  void Run (pth_sem_t * stop);
  const char *Name() { return "busmonitor"; }
protected:
  /** Layer 3 Interface*/
  Layer3 * l3;
  /** client connection */
  ClientConnection *con;
  /** debug output */
  Trace *t;
  /** turns a busmonitor LPDU into a eibd packet and sends it */
  virtual int sendResponse (L_Busmonitor_PDU * p, pth_event_t stop);
public:
  /** initializes busmonitor
   * @param c client connection
   * @param tr debug output
   * @param l3 Layer 3
   * @param virt is virtual busmonitor
   * @param ts provide timestamps
   */
  A_Busmonitor (ClientConnection * c,
                bool virt = false, bool ts = false);
  virtual ~A_Busmonitor ();
  void Send_L_Busmonitor (L_Busmonitor_PDU * l);

  /** start processing */
  void Do (pth_event_t stop);
};

/** implements text busmonitor functions for a client */
class A_Text_Busmonitor:public A_Busmonitor
{
protected:
  int sendResponse (L_Busmonitor_PDU * p, pth_event_t stop);
public:
  /** initializes busmonitor
   * @param c client connection
   * @param tr debug output
   * @param l3 Layer 3
   * @param virt is virtual busmonitor
   */
  A_Text_Busmonitor (ClientConnection * c, bool virt = 0)
                   : A_Busmonitor (c, virt, false)
  {
  }
};

#endif
