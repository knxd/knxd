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
#include <fcntl.h>
#include <sys/ioctl.h>
#include "tpuartserial.h"

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


TPUARTSerialLayer2Driver::TPUARTSerialLayer2Driver (const char *dev,
                                                    eibaddr_t a, int flags,
                                                    Trace * tr)
    : TPUARTBASESerialLayer2Driver(dev, a, flags, tr)
{

}

TPUARTSerialLayer2Driver::~TPUARTSerialLayer2Driver ()
{


}

termios TPUARTSerialLayer2Driver::getTermiosSettings()
{
    struct termios t1;
    t1.c_cflag = CS8 | CLOCAL | CREAD | PARENB;
    t1.c_iflag = IGNBRK | INPCK | ISIG;
    t1.c_oflag = 0;
    t1.c_lflag = 0;
    t1.c_cc[VTIME] = 1;
    t1.c_cc[VMIN] = 0;
    return t1;
}

int TPUARTSerialLayer2Driver::getBaudRate()
{
    return B19200;
}
