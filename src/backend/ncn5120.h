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

#ifndef SNCN5120_SERIAL_H
#define SNCN5120_SERIAL_H
#include <termios.h>
#include "iobuf.h"
#include "lowlatency.h"
#include "layer2.h"

/** TPUART user mode driver */
class NCN5120SerialLayer2Driver : public Layer2
{
  /** old serial config */
  low_latency_save sold;
  /** old termios state */
  struct termios old;
  /** file descriptor */
  int fd;
  RecvBuf recvbuf;
  SendBuf sendbuf;
  size_t read_cb(uint8_t *buf, size_t len);
  void error_cb();

  void stop();

  /** input queue */
  Queue < LPDU * >send_q;

  bool ackallgroup;
  bool ackallindividual;
  bool dischreset;

  /** process a received frame */
  void RecvLPDU (const uchar * data, int len);
  const char *Name() { return "ncn5120"; }

  /** set up send and recv buffers, timers, etc. */
  void setup_buffers();

  ev::async trigger; void trigger_cb (ev::async &w, int revents);
  ev::timer timer; void timer_cb (ev::timer &w, int revents);
  ev::timer sendtimer; void sendtimer_cb (ev::timer &w, int revents);
  ev::timer watchdogtimer; void watchdogtimer_cb (ev::timer &w, int revents);
  /** process incoming data */
  void process_read(bool is_timeout);

  CArray in;
  bool to = false;
  bool waitconfirm = false;
  bool acked = false;
  int retry = 0;
  int watch = 0;
  bool rmn = false;

public:
  NCN5120SerialLayer2Driver (const char *dev, L2options *opt);
  ~NCN5120SerialLayer2Driver ();
  bool init (Layer3 *l3);

  void send_L_Data (L_Data_PDU * l);

  bool enterBusmonitor ();
  bool leaveBusmonitor ();

  bool Open ();
};

#endif
