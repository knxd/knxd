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
#include "lowlatency.h"
#include "tpuartserial.h"

/** TPUART user mode driver */
DRIVER_(NCN5120Serial,TPUARTSerial,ncn5120)
{
public:
  NCN5120Serial (const LinkConnectPtr& c, IniSection& s);
  virtual ~NCN5120Serial ();

protected:
  void termios_settings(struct termios &t);
  unsigned int default_baudrate();
  void setstate(enum TSTATE state);

  void RecvLPDU (const uchar * data, int len);
};

#endif
