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

#ifndef TPUART_H
#define TPUART_H
#include "iobuf.h"
#include "eibnetip.h"
#include "link.h"
#include "lpdu.h"
#include "lowlevel.h"

// also update SN() in tpuart.cpp
enum TSTATE {
  T_new = 0,
  T_error,
  T_start = 11,
  T_in_reset,
  T_in_setaddr,
  T_in_getstate,
  T_is_online = 19,
  T_wait = 20,
  T_wait_more,
  T_wait_keepalive,
  T_busmonitor = 30,
};

DRIVER_(TPUART,LowLevelAdapter,tpuart)
{
public:
  TPUART(const LinkConnectPtr_& c, IniSectionPtr& s) : LowLevelAdapter(c,s)
    {
      t->setAuxName("tpuart");
    }
  virtual ~TPUART();

  bool setup();
protected:
  virtual LowLevelFilter * create_wrapper(LowLevelIface* parent, IniSectionPtr& s, LowLevelDriver* i = nullptr);
};

class TPUARTwrap : public LowLevelFilter
{
public:
  TPUARTwrap (LowLevelIface* parent, IniSectionPtr& s, LowLevelDriver* i = nullptr);
  virtual ~TPUARTwrap();

protected:
  void recv_Data(CArray &c);

  bool ackallgroup;
  bool ackallindividual;

  /** process a received frame */
  virtual void RecvLPDU (const uchar * data, int len);

  virtual void do_send_Next();
  void do__send_Next();
  void send_again();
  void in_check();

  /** OK to send next packet */
  bool next_free = true;
  /** waiting for OK to send next packet */
  bool send_wait = false;

  /** main loop state */
  ev::timer timer; void timer_cb(ev::timer &w, int revents);
  ev::timer sendtimer; void sendtimer_cb(ev::timer &w, int revents);

  LPDUPtr sending;
  CArray in, out;
  unsigned int retry = 0;
  unsigned int send_retry = 0;
  bool acked = false;
  bool recvecho = false;
  bool skip_char = false;
  bool monitor = false;
  eibaddr_t my_addr = 0;

  enum TSTATE state = T_new;
  virtual void setstate(enum TSTATE new_state);

public:
  bool setup();
  void started();
  void stopped();

  void send_L_Data (LDataPtr l);

protected:
  virtual FDdriver * create_serial(LowLevelIface* parent, IniSectionPtr& s);

};

#endif
