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

#ifndef TPUART_SERIAL_H
#define TPUART_SERIAL_H
#include <termios.h>
#include "lowlatency.h"
#include "tpuart.h"

/** TPUART user mode driver */
class TPUARTSerialLayer2Driver:public TPUART_Base
{
  /** old serial config */
  low_latency_save sold;
  /** old termios state */
  struct termios old;
  bool dischreset;

  const char *Name() { return "tpuarts"; }
  void setstate(enum TSTATE state);
  void dev_timer();
  virtual void termios_settings (struct termios &t);
  virtual unsigned int default_baudrate () { return 19200; }

  const char *dev;

public:
  TPUARTSerialLayer2Driver (const char *dev, L2options *opt);
  ~TPUARTSerialLayer2Driver();
};

#endif
