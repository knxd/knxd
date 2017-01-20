/*
    SystemTera eib bus access driver
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

NCN5120SerialLayer2Driver::NCN5120SerialLayer2Driver (const char *dev, L2options *opt)
    : TPUARTSerialLayer2Driver(dev, opt)
{
}

NCN5120SerialLayer2Driver::~NCN5120SerialLayer2Driver ()
{
}

void
NCN5120SerialLayer2Driver::termios_settings(struct termios &t1)
{
    t1.c_cflag = CS8 | CLOCAL | CREAD;
    t1.c_iflag = IGNBRK | INPCK | ISIG;
    t1.c_oflag = 0;
    t1.c_lflag = 0;
    t1.c_cc[VTIME] = 1;
    t1.c_cc[VMIN] = 0;
}

unsigned int NCN5120SerialLayer2Driver::default_baudrate()
{
    return 38400;
}

void NCN5120SerialLayer2Driver::RecvLPDU (const uchar * data, int len)
{
    skip_char = true;
    TPUARTSerialLayer2Driver::RecvLPDU (data, len);
}
