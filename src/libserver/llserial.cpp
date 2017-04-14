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

#include "config.h"

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>
#ifdef HAVE_LINUX_LOWLATENCY
#include <sys/ioctl.h>
#include <string.h> // memcpy
#endif

#include "llserial.h"

static speed_t getbaud(int baud) {
  switch(baud)
    {
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

LLserial::~LLserial () {}

bool
LLserial::setup()
{
  if(!FDdriver::setup())
    return false;

  dev = cfg->value("device","");
  if(dev.size() == 0) 
    {
      ERRORPRINTF (t, E_ERROR | 22, "Missing device= config");
      return false;
    }
  baudrate = cfg->value("baudrate", (int)default_baudrate());
  if (getbaud(baudrate) == 0)
    {
      ERRORPRINTF (t, E_ERROR | 22, "Wrong baudrate= config");
      return false;
    }

  return true;
}

void
LLserial::start()
{
  struct termios t1;
  int term_baudrate;

  fd = open (dev.c_str(), O_RDWR | O_NOCTTY | O_NDELAY | O_SYNC);
  if (fd == -1)
    {
      ERRORPRINTF (t, E_ERROR | 22, "Opening %s failed: %s", dev, strerror(errno));
      goto ex1;
    }

  if (!set_low_latency (fd, &sold))
    {
      ERRORPRINTF (t, E_ERROR | 22, "low_latency %s failed: %s", dev, strerror(errno));
      goto ex2;
    }

  close (fd);

  fd = open (dev.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
  if (fd == -1)
    {
      ERRORPRINTF (t, E_ERROR | 23, "Opening %s failed: %s", dev, strerror(errno));
      goto ex1;
    }

  if (tcgetattr (fd, &old))
    {
      ERRORPRINTF (t, E_ERROR | 24, "tcgetattr %s failed: %s", dev, strerror(errno));
      goto ex3;
    }

  if (tcgetattr (fd, &t1))
    {
      ERRORPRINTF (t, E_ERROR | 25, "tcgetattr %s failed: %s", dev, strerror(errno));
      goto ex3;
    }

  termios_settings(t1);
  term_baudrate = getbaud(baudrate);
  if (term_baudrate == -1)
    {
      ERRORPRINTF (t, E_ERROR | 58, "baudrate %d not recognized", baudrate);
      goto ex3;
    }
  TRACEPRINTF(t, 0, "Opened %s with baud %d", dev, baudrate);
  cfsetospeed (&t1, term_baudrate);
  cfsetispeed (&t1, 0);

  if (tcsetattr (fd, TCSAFLUSH, &t1))
    {
      ERRORPRINTF (t, E_ERROR | 26, "tcsetattr %s failed: %s", dev, strerror(errno));
      goto ex3;
    }

  TRACEPRINTF (t, 2, "Opened");
  FDdriver::start();
  return;

ex3:
  restore_low_latency (fd, &sold);
ex2:
  close (fd);
  fd = -1;
ex1:
  stopped();
}

void
LLserial::stop()
{
  if (fd >= 0)
    restore_low_latency (fd, &sold);
  FDdriver::stop();
}

