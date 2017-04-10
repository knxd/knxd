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

#include "iobuf.h"
#include "lowlevel.h"
#include "emi_common.h"
#include "lowlatency.h"
#include "link.h"
#include "cemi.h"

/** FT12-specific CEMI backend (separate commands for setup) */
class FT12CEMIDriver : public CEMIDriver
{
  void cmdOpen(); 
public:
  FT12CEMIDriver (LowLevelIface* c, IniSectionPtr& s, LowLevelDriver * i = nullptr) : CEMIDriver(c,s,i)
    {
      t->setAuxName("ft12cemi");
    }
  virtual ~FT12CEMIDriver ();
};

/** FT1.2 lowlevel driver*/
DRIVER_(FT12Driver,LowLevelAdapter,ft12)
{
public:
  FT12Driver(const LinkConnectPtr_& c, IniSectionPtr& s) : LowLevelAdapter(c,s)
    {
      t->setAuxName("ft12dr");
    }
  virtual ~FT12Driver();

  bool setup();
  virtual EMIVer getVersion() { return vEMI2; }
private:
  bool make_EMI();
};

DRIVER_(FT12cemiDriver, FT12Driver, ft12cemi)
{
public:
  FT12cemiDriver(const LinkConnectPtr_& c, IniSectionPtr& s) : FT12Driver(c,s)
    {
      t->setAuxName("ft12drc");
    }
  virtual ~FT12cemiDriver();

  virtual EMIVer getVersion() { return vCEMI; }
};

class FT12wrap : public LowLevelFilter
{
  /** send msg sequence 1-bit counter */
  bool sendflag;
  /** receive msg sequence 1-bit counter */
  bool recvflag;

  /** packet send buffer */
  CArray out;
  /** frame in receiving */
  CArray akt;
  /** last received frame */
  CArray last;
  /** repeatcount of the transmitting frame */
  int repeatcount;

  /** waiting for ACK */
  bool send_wait;
  /** send_Next received */
  bool next_free = true;
  /** protect against recursive call of process_read() */
  bool in_reader = false;

  /** set up send and recv buffers, timers, etc. */
  void setup_buffers();

  ev::async trigger; void trigger_cb (ev::async &w, int revents);
  ev::timer timer; void timer_cb (ev::timer &w, int revents);
  ev::timer sendtimer; void sendtimer_cb (ev::timer &w, int revents);
  /** process incoming data */
  void process_read (bool is_timeout);
  void do_send_Next ();
  void do__send_Next ();
  void stop_ ();

public:
  FT12wrap (LowLevelIface* c, IniSectionPtr& s, LowLevelDriver *i = nullptr);
  virtual ~FT12wrap ();
  bool setup ();
  void start ();
  void stop ();

  void recv_Data (CArray &c);
  void send_Data (CArray& l);
  void do_send_Local (CArray& l, int raw = 0);

  void sendReset();
};

#endif
