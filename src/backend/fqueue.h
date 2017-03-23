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

#ifndef FQUEUE_H
#define FQUEUE_H
#include "link.h"
#include "queue.h"

enum QSTATE {
    Q_DOWN,    // not running
    Q_IDLE,    // no packet submitted
    Q_BUSY,    // packet submitted, not in send loop
    Q_SENDING, // packet submitted, in send loop
};

FILTER(QueueFilter,queue)
{
  Queue < LDataPtr > buf;
  enum QSTATE state;
  ev::async trigger;
  void trigger_cb (ev::async &w, int revents);

public:
  QueueFilter (const LinkConnectPtr_& c, IniSectionPtr& s);
  virtual ~QueueFilter ();

  virtual bool setup();
  virtual void send_L_Data (LDataPtr l);
  virtual void send_Next();

  virtual void started();
  virtual void stopped();

};


#endif
