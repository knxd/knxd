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

#include "link.h"
#include "lowlevel.h"
#include "emi_common.h"

/*
 * The driver stack is: USB driver > [C]EMI[12] wrapper > USBConverterInterface > USBLowLevelDriver
 */

/** The USBConverterInterface's job is to add the appropriate USB header to
 * the [C]EMI[12] frame on sending / remove that from incoming data
 */
class USBConverterInterface : public LowLevelFilter
{
public:
  USBConverterInterface (LowLevelIface* p, IniSectionPtr& s);
  virtual ~USBConverterInterface ();

  bool setup (DriverPtr master);
  //void start ();
  //void stop ();

  void send_Data (CArray& l);
  void recv_Data (CArray& l);

  void send_Init();
  void sendLocal_done_cb(bool success);

  EMIVer version = vRaw;
};

/** USB backend */
DRIVER_(USBDriver,LowLevelAdapter,usb)
{
  // for EMI version discovery
  ev::timer timeout;
  int cnt = 0;
  void timeout_cb(ev::timer &w, int revents);
  void xmit();
  void recv(CArray *r1);
  void recv_Data(CArray& c);
  bool wait_make = false;
  USBConverterInterface *usb_iface;

  void sendLocal_done_cb(bool success);
public:
  EMIVer version = vUnknown;

  USBDriver (const LinkConnectPtr_& c, IniSectionPtr& s);
  bool setup();
  //void start();
  //void stop();
  void started();
  void stopped();
  void do_send_Next();
  bool make_EMI();

};

#endif
