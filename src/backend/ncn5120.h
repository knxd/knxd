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

#define NO_MAP
#include "tpuart.h"

class NCN5120serial : public TPUARTserial
{
public:
  NCN5120serial(LowLevelIface* a, IniSectionPtr& b) : TPUARTserial(a,b) { t->setAuxName("NCN_ser"); }
  virtual ~NCN5120serial ();

protected:
  unsigned int default_baudrate() { return 38400; }
  void termios_settings(struct termios &t1)
    {
      t1.c_cflag = CS8 | CLOCAL | CREAD;
      t1.c_iflag = IGNBRK | INPCK | ISIG;
      t1.c_oflag = 0;
      t1.c_lflag = 0;
      t1.c_cc[VTIME] = 1;
      t1.c_cc[VMIN] = 0;
    }
};

/** TPUART-derived driver */
DRIVER_(NCN5120,TPUART,ncn5120)
{
public:
  NCN5120 (const LinkConnectPtr_& c, IniSectionPtr& s) : TPUART(c,s) { t->setAuxName("NCN"); } ;
  virtual ~NCN5120 ();

protected:
  void termios_settings(struct termios &t);
  unsigned int default_baudrate();
  void setstate(enum TSTATE state);

  void RecvLPDU (const uchar * data, int len);
  virtual LLserial * create_serial(LowLevelIface* parent, IniSectionPtr& s)
    {
      return new NCN5120serial(parent,s);
    }
};

#endif
