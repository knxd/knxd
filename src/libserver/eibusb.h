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

#ifndef EIB_USB_EMI_H
#define EIB_USB_EMI_H

#include "layer2.h"
#include "lowlevel.h"

/** USBConverterInterface */
class USBConverterInterface:public LowLevelDriverInterface
{
  Trace *t;
  LowLevelDriverInterface *i;
  EMIVer v;
public:
  USBConverterInterface (LowLevelDriverInterface * iface, Trace * tr,
                          EMIVer ver);
  virtual ~USBConverterInterface ();
  bool init ();

  void Send_Packet (CArray l);
  bool Send_Queue_Empty ();
  pth_sem_t *Send_Queue_Empty_Cond ();
  CArray *Get_Packet (pth_event_t stop);

  void SendReset ();
  bool Connection_Lost ();

  EMIVer getEMIVer ();
};

LowLevelDriverInterface *initUSBDriver (LowLevelDriverInterface * i,
					Trace * tr);

/** USB backend */
class USBLayer2Interface:public Layer2Interface
{
  /** EMI */
  Layer2Interface *emi;

public:
  USBLayer2Interface (LowLevelDriverInterface * i, Trace * tr, int flags);
  ~USBLayer2Interface ();
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
  bool Connection_Lost ();
  bool Send_Queue_Empty ();
};

#endif
