/*
    EIBD eib bus access and management daemon
    Copyright (C) 2017 Matthias Urlichs <matthias@urlichs.de>

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

/**

This module implements a filter which limits the transmission rate of
packets in a link.

If the driver supports handshaking, the send_Next signal is delayed
by the configured time.

Otherwise, the filter signals that it supports handshaking,
and signals send_Next some time after a packet is sent.

If there is no queue in front of this filter, the rate limit acts globally.
This is probably not intentional, and thus warned about.
*/

#ifndef FPACE_H
#define FPACE_H
#include "link.h"

enum PSTATE {
    P_DOWN,    // not running, not marked as requiring a Pace
    P_IDLE,    // no packet submitted, no Pace
    P_BUSY,    // packet submitted, no Pace, wait before replying
};

FILTER(PaceFilter,pace)
{
  bool want_next = false;
  float delay;
  float byte_delay;
  int nr_in;
  int size_in;
  float factor_in;
  size_t last_len;
  enum PSTATE state;
  ev::timer timer; void timer_cb(ev::timer &w, int revents);

public:
  PaceFilter (const LinkConnectPtr_& c, IniSectionPtr& s);
  virtual ~PaceFilter ();

  virtual bool setup();
  virtual void send_L_Data (LDataPtr l);
  virtual void recv_L_Data (LDataPtr l);
  virtual void send_Next();

  virtual void start();
  virtual void started();
  virtual void stopped();

};

#endif
