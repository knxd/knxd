/* SystemTera eib bus access driver
    Copyright (C) 2014 Patrik Pfaffenbauer <patrik.pfaffenbauer@p3.co.at>

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

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "ncn5120.h"
#include "llserial.h"

NCN5120::~NCN5120() { }

class NCN5120wrap : public TPUARTwrap
{
public:
  NCN5120wrap (LowLevelIface* parent, IniSectionPtr& s, LowLevelDriver* i = nullptr) : TPUARTwrap(parent,s,i) {}
  virtual ~NCN5120wrap() {}

protected:
  void termios_settings(struct termios &t);
  unsigned int default_baudrate();
  void setstate(enum TSTATE state);
  
  void RecvLPDU (const uchar * data, int len);
  virtual FDdriver * create_serial(LowLevelIface* parent, IniSectionPtr& s);
};

class NCN5120serial : public LLserial
{
public:
  NCN5120serial(LowLevelIface* a, IniSectionPtr& b) : LLserial(a,b) { t->setAuxName("NCN_ser"); }
  virtual ~NCN5120serial () {};

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

LowLevelFilter * 
NCN5120::create_wrapper(LowLevelIface* parent, IniSectionPtr& s, LowLevelDriver* i)
{
  return new NCN5120wrap(parent,s,i);
}


FDdriver *
NCN5120wrap::create_serial(LowLevelIface* parent, IniSectionPtr& s)
{
  return new NCN5120serial(parent,s);
}

void NCN5120wrap::RecvLPDU (const uchar * data, int len)
{
    skip_char = true;
    TPUARTwrap::RecvLPDU (data, len);
}

void
NCN5120wrap::setstate(enum TSTATE new_state)
{
  if (new_state == T_in_setaddr && my_addr != 0)
    {
      uint8_t addrbuf[4] = { 0xF1, (uint8_t)((my_addr>>8)&0xFF), (uint8_t)(my_addr&0xFF), 0x00 };
      TRACEPRINTF (t, 0, "SendAddr %02X%02X%02X", addrbuf[0],addrbuf[1],addrbuf[2]);
      LowLevelIface::send_Data(addrbuf, sizeof(addrbuf));
      new_state = T_in_getstate;
    }
  TPUARTwrap::setstate(new_state);
}

