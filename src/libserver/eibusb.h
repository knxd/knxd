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
class USBConverterInterface:public LowLevelDriver
{
  LowLevelDriver *i;
  EMIVer v;
public:
  USBConverterInterface (LowLevelDriver * iface, TracePtr tr,
                          EMIVer ver);
  virtual ~USBConverterInterface ();
  bool init ();

  void Send_Packet (CArray l);

  void SendReset ();

  EMIVer getEMIVer ();

private:
  void on_recv_cb(CArray *p);

};

LowLevelDriver *initUSBDriver (LowLevelDriver * i,
					TracePtr tr);

/** USB backend */
class USBLayer2:public Layer2
{
  /** EMI */
  Layer2Ptr emi;

public:
  USBLayer2 (LowLevelDriver * i, L2options *opt);
  bool init (Layer3 * l3);

  void send_L_Data (L_Data_PDU * l);

  bool enterBusmonitor ();
  bool leaveBusmonitor ();

  bool Open ();
  bool Close ();
};

#endif
