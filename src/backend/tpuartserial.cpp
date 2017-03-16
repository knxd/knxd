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

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "tpuartserial.h"
#include <stdlib.h>

/** get serial status lines */
static int
getstat (int fd)
{
  int s;
  ioctl (fd, TIOCMGET, &s);
  return s;
}

/** set serial status lines */
static void
setstat (int fd, int s)
{
  ioctl (fd, TIOCMSET, &s);
}

static speed_t getbaud(int baud) {
    switch(baud) {
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 115200:
            return B115200;
        default:
            return 0;
    }
}

TPUARTSerial::TPUARTSerial (const LinkConnectPtr_& c, IniSection& s)
	: TPUART_Base (c,s)
{
}

bool
TPUARTSerial::setup()
{
  if(!TPUART_Base::setup())
    return false;

  dev = cfg.value("device","");
  if(dev.size() == 0) 
    {
      ERRORPRINTF (t, E_ERROR | 22, "Missing device= config");
      return false;
    }
  baudrate = cfg.value("baudrate", 0);
  if (baudrate == 0)
      baudrate = default_baudrate();
  if (getbaud(baudrate) == 0)
    {
      ERRORPRINTF (t, E_ERROR | 22, "Wrong baudrate= config");
      return false;
    }

  dischreset = cfg.value("reset",false);
  return true;
}

void
TPUARTSerial::setstate(enum TSTATE new_state)
{
  TRACEPRINTF (t, 8, "ser state %d>%d",state,new_state);
  switch(new_state)
    {
    case T_dev_start:
      {
        struct termios t1;
        int term_baudrate;

        fd = open (dev.c_str(), O_RDWR | O_NOCTTY | O_NDELAY | O_SYNC);
        if (fd == -1)
          {
            ERRORPRINTF (t, E_ERROR | 22, "Opening %s failed: %s", dev, strerror(errno));
            return;
          }
        set_low_latency (fd, &sold);

        close (fd);

        fd = open (dev.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
        if (fd == -1)
          {
            ERRORPRINTF (t, E_ERROR | 23, "Opening %s failed: %s", dev, strerror(errno));
            return;
          }

        if (tcgetattr (fd, &old))
          {
            ERRORPRINTF (t, E_ERROR | 24, "tcgetattr %s failed: %s", dev, strerror(errno));
            restore_low_latency (fd, &sold);
            close (fd);
            fd = -1;
            return;
          }

        if (tcgetattr (fd, &t1))
          {
            ERRORPRINTF (t, E_ERROR | 25, "tcgetattr %s failed: %s", dev, strerror(errno));
            restore_low_latency (fd, &sold);
            close (fd);
            fd = -1;
            return;
          }

        termios_settings(t1);
        term_baudrate = getbaud(baudrate);
        if (term_baudrate == -1)
          {
            ERRORPRINTF (t, E_ERROR | 58, "baudrate %d not recognized", baudrate);
            restore_low_latency (fd, &sold);
            close (fd);
            fd = -1;
            return;
          }
        TRACEPRINTF(t, 0, "Opened %s with baud %d", dev, baudrate);
        cfsetospeed (&t1, term_baudrate);
        cfsetispeed (&t1, 0);

        if (tcsetattr (fd, TCSAFLUSH, &t1))
          {
            ERRORPRINTF (t, E_ERROR | 26, "tcsetattr %s failed: %s", dev, strerror(errno));
            restore_low_latency (fd, &sold);
            close (fd);
            fd = -1;
            return;
          }

        TRACEPRINTF (t, 2, "Openend");

        setup_buffers();
      }
      // FALL THRU
    case T_in_reset:
      if (state == T_dev_start+3)
        TPUART_Base::setstate(new_state);
      else if (dischreset)
        setstate((enum TSTATE)(T_dev_start+2));
      else
        setstate((enum TSTATE)(T_dev_start+3));
      return;

    case T_dev_start+2: // reset wait
      setstat (fd, getstat (fd) & ~(TIOCM_RTS | TIOCM_DTR));
      timer.start(0.002,0);
      break;

    case T_dev_start+3: // reset wait
      setstat (fd, (getstat (fd) & ~TIOCM_RTS) | TIOCM_DTR);
      timer.start(0.1,0);
      break;

    default:
      TPUART_Base::setstate(new_state);
      return;
    }
  state = new_state;
}

TPUARTSerial::~TPUARTSerial ()
{
}

void
TPUARTSerial::dev_timer()
{
  switch(state)
    {
    case T_dev_start:
      setstate(T_start);
      break;
    case T_dev_start+2:
      setstate((enum TSTATE)(T_start+3));
      break;
    case T_dev_start+3:
      setstate(T_in_reset);
      break;
    default:
      TRACEPRINTF (t, 8, "Unknown state %d",state);
      break;
    }
}

void
TPUARTSerial::termios_settings (struct termios &t1)
{
  t1.c_cflag = CS8 | CLOCAL | CREAD | PARENB;
  t1.c_iflag = IGNBRK | INPCK | ISIG;
  t1.c_oflag = 0;
  t1.c_lflag = 0;
  t1.c_cc[VTIME] = 1;
  t1.c_cc[VMIN] = 0;
}

