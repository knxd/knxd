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

NCN5120SerialLayer2Driver::NCN5120SerialLayer2Driver (const char *dev,
                                                      eibaddr_t a, int flags,
                                                      Trace * tr)
    : TPUARTBASESerialLayer2Driver(dev, a, flags, tr)
{
    m_rmn = false;
}

NCN5120SerialLayer2Driver::~NCN5120SerialLayer2Driver ()
{

}

termios NCN5120SerialLayer2Driver::getTermiosSettings()
{
    struct termios t1;
    t1.c_cflag = CS8 | CLOCAL | CREAD;
    t1.c_iflag = IGNBRK | INPCK | ISIG;
    t1.c_oflag = 0;
    t1.c_lflag = 0;
    t1.c_cc[VTIME] = 1;
    t1.c_cc[VMIN] = 0;
    return t1;
}

int NCN5120SerialLayer2Driver::getBaudRate()
{
    return B38400;
}

void NCN5120SerialLayer2Driver::afterLPDUReceived()
{
    m_rmn = true;
    TRACEPRINTF(t, 0, this, "AfterLPDUReceived");
}

bool NCN5120SerialLayer2Driver::beforeLoop(CArray &in)
{
    if(m_rmn) {
        TRACEPRINTF (t, 0, this, "Remove next");
        in.deletepart(0, 1);
        m_rmn = false;
        return true;
    }
    m_rmn = false;
    return false;
}
