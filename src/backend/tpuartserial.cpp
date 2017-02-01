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
#include "layer3.h"
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
        case 115200:
            return B115200;
        default:
            return -1;
    }
}

TPUARTSerialLayer2Driver::TPUARTSerialLayer2Driver (const char *dev,
						    L2options *opt)
	: TPUART_Base (opt)
{
  struct termios t1;
  TRACEPRINTF (t, 2, this, "Open");

  char *pch;
  int baudrate = default_baudrate();
  int term_baudrate;
  pch = strtok((char*)dev, ":");
  int i = 0;

  while(pch != NULL) {

      switch(i) {
      case 0:
        break;
      case 1:
          baudrate = atoi(pch);
          break;
      }

      pch = strtok(NULL, ":");
      i++;
  }

  dischreset = opt ? (opt->flags & FLAG_B_TPUARTS_DISCH_RESET) : 0;

  if (opt)
	opt->flags &=~ FLAG_B_TPUARTS_DISCH_RESET;

  fd = open (dev, O_RDWR | O_NOCTTY | O_NDELAY | O_SYNC);
  if (fd == -1)
    {
      ERRORPRINTF (t, E_ERROR | 22, this, "Opening %s failed: %s", dev, strerror(errno));
      return;
    }
  set_low_latency (fd, &sold);

  close (fd);

  fd = open (dev, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd == -1)
    {
      ERRORPRINTF (t, E_ERROR | 23, this, "Opening %s failed: %s", dev, strerror(errno));
      return;
    }

  if (tcgetattr (fd, &old))
    {
      ERRORPRINTF (t, E_ERROR | 24, this, "tcgetattr %s failed: %s", dev, strerror(errno));
      restore_low_latency (fd, &sold);
      close (fd);
      fd = -1;
      return;
    }

  if (tcgetattr (fd, &t1))
    {
      ERRORPRINTF (t, E_ERROR | 25, this, "tcgetattr %s failed: %s", dev, strerror(errno));
      restore_low_latency (fd, &sold);
      close (fd);
      fd = -1;
      return;
    }

  termios_settings(t1);
  term_baudrate = getbaud(baudrate);
  if (term_baudrate == -1)
    {
      ERRORPRINTF (t, E_ERROR | 58, this, "baudrate %d not recognized", baudrate);
      restore_low_latency (fd, &sold);
      close (fd);
      fd = -1;
      return;
    }
  TRACEPRINTF(t, 0, this, "Opened %s with baud %d", dev, baudrate);
  cfsetospeed (&t1, term_baudrate);
  cfsetispeed (&t1, 0);

  if (tcsetattr (fd, TCSAFLUSH, &t1))
    {
      ERRORPRINTF (t, E_ERROR | 26, this, "tcsetattr %s failed: %s", dev, strerror(errno));
      restore_low_latency (fd, &sold);
      close (fd);
      fd = -1;
      return;
    }

  setstat (fd, (getstat (fd) & ~TIOCM_RTS) | TIOCM_DTR);
  setup_buffers();

  TRACEPRINTF (t, 2, this, "Openend");
}

TPUARTSerialLayer2Driver::~TPUARTSerialLayer2Driver ()
{
  resettimer.stop();
}

void
TPUARTSerialLayer2Driver::termios_settings (struct termios &t1)
{
  t1.c_cflag = CS8 | CLOCAL | CREAD;
  t1.c_iflag = IGNBRK | INPCK | ISIG;
  t1.c_oflag = 0;
  t1.c_lflag = 0;
  t1.c_cc[VTIME] = 1;
  t1.c_cc[VMIN] = 0;
}


void
TPUARTSerialLayer2Driver::resettimer_cb(ev::timer &w, int revents)
{
  if (watch == 5)
    {
      watch = 6;
      setstat (fd, (getstat (fd) & ~TIOCM_RTS) | TIOCM_DTR);
      resettimer.start(0.001,0);
    }
  else
    TPUART_Base::send_reset();
}

void TPUARTSerialLayer2Driver::send_reset()
{
  if (dischreset)
    {
      setstat (fd, (getstat (fd) & ~TIOCM_RTS) & ~TIOCM_DTR);
      watch = 5;
      resettimer.start(0.002,0);
    }
  else
    TPUART_Base::send_reset();
}
