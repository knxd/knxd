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

This module implements a filter which buffers packets if the driver
supports it.

*/

#ifndef FRETRY_H
#define FRETRY_H
#include "link.h"

enum RSTATE
{
  R_DOWN,       // not running
  R_GOING_UP,   // in startup
  R_UP,         // running, no packet submitted
  R_GOING_DOWN, // in shutdown
  R_GOING_ERROR,// in shutdown after error
  R_ERROR,      // not running, had error
};

FILTER(RetryFilter, retry)
{
  friend class LinkConnect;

  enum RSTATE state;
  ev::async trigger;
  void trigger_cb (ev::async &w, int revents);
  ev::timer timeout;
  void timeout_cb (ev::timer &w, int revents);

  // if true, throw away messages while starting up
  bool flush = false;
  // if set, retry on initial startup error
  // if ==2, "started" has been sent to the router
  uint8_t may_fail = 0;
  int max_retry = 0;
  float retry_delay = 1;
  float start_timeout = 10;
  float send_timeout = 5;

  // signalled from router?
  bool want_up = false;
  int retries = 0;
  // storage for in-flight transmitted message
  LDataPtr msg = nullptr;

  // internal stop handler
  void stop_(bool err);

public:
  RetryFilter (const LinkConnectPtr_& c, IniSectionPtr& s);
  virtual ~RetryFilter ();

  virtual bool setup();
  virtual void send_L_Data (LDataPtr l);
  virtual void send_Next();

  virtual void start();
  virtual void stop(bool err);

  virtual void started();
  virtual void stopped(bool err);

};


#endif
