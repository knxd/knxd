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

#include "lowlatency.h"

#include <errno.h>

#include <sys/ioctl.h>
#include <string.h> // memcpy

bool
set_low_latency (int fd, low_latency_save * save)
{
  struct termios opts;

#ifdef HAVE_LINUX_LOWLATENCY
  struct serial_struct snew;
  ioctl (fd, TIOCGSERIAL, &save->ser);
  memcpy(&snew, &save->ser, sizeof(snew));
  snew.flags |= ASYNC_LOW_LATENCY;
  // not all serial drivers support this call, so don't bail out on failure with ENOTTY
  if(ioctl (fd, TIOCSSERIAL, &snew) < 0) {
    if (errno != ENOTTY && errno != EOPNOTSUPP)
      return false;
  }
#endif

  tcgetattr(fd, &save->term);
  memcpy(&opts, &save->term, sizeof(opts));
  opts.c_cc[VTIME] = 1;
  opts.c_cc[VMIN] = 1;
  if (tcsetattr(fd, TCSANOW, &opts) < 0)
    return false;

  return true;
}

void
restore_low_latency (int fd, low_latency_save * save)
{
#ifdef HAVE_LINUX_LOWLATENCY
  ioctl (fd, TIOCSSERIAL, &save->ser);
#endif
  ioctl (fd, TCSANOW, &save->term);
}

