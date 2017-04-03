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
#include "lowlevel.h"

LowLevelAdapter::~LowLevelAdapter() {}
LowLevelDriver::~LowLevelDriver() {}
LowLevelIface::~LowLevelIface()
{
  local_timeout.stop();
}

LowLevelIface::LowLevelIface()
{
  sendLocal_done.set<LowLevelIface,&LowLevelIface::done_aborter>(this);
  local_timeout.set<LowLevelIface,&LowLevelIface::local_timeout_cb>(this);
}

LowLevelFilter::~LowLevelFilter()
{
  delete iface;
}

void
LowLevelIface::done_aborter(bool success)
{
  ERRORPRINTF (tr(), E_FATAL, "non-initialized send_Local callback");
  abort();
}

void
LowLevelIface::send_Local(CArray &d, int raw)
{
  assert(!is_local);
  is_local = true;
  local_timeout.start(0.9, 0);
  do_send_Local(d, raw);
}

void
LowLevelFilter::do_send_Local(CArray &d, int raw)
{
  if (raw)
    iface->do_send_Local(d, raw-1);
  else
    send_Data(d);
}

void
LowLevelIface::local_timeout_cb(ev::timer &w UNUSED, int revents UNUSED)
{
  ERRORPRINTF (tr(), E_ERROR, "send_Local timed out!");
  is_local = false;
  sendLocal_done(false);
}

void
LowLevelIface::send_Next()
{
  if (is_local)
    {
      local_timeout.stop();
      is_local = false;
      sendLocal_done(true);
    }
  else
    do_send_Next();
}

void
LowLevelAdapter::send_L_Data(LDataPtr l)
{
  if (!iface)
    {
      ERRORPRINTF (t, E_ERROR, "Send: not running??");
      errored();
      return;
    }
  iface->send_L_Data(std::move(l));
}

FDdriver::FDdriver (LowLevelIface* p, IniSectionPtr& s)
	: sendbuf(), recvbuf(), LowLevelDriver (p,s)
{
  t->setAuxName("FD");
}

bool
FDdriver::setup()
{
  if(!LowLevelDriver::setup())
    return false;

  return true;
}

void
FDdriver::setup_buffers()
{
  TRACEPRINTF (t, 2, "Buffer Setup on fd %d", fd);
  sendbuf.init(fd);
  recvbuf.init(fd);
  recvbuf.low_latency();

  recvbuf.on_read.set<FDdriver,&FDdriver::read_cb>(this);
  recvbuf.on_error.set<FDdriver,&FDdriver::error_cb>(this);
  sendbuf.on_error.set<FDdriver,&FDdriver::error_cb>(this);

  sendbuf.start();
  recvbuf.start();
}

void
FDdriver::error_cb()
{
  ERRORPRINTF (t, E_ERROR | 23, "Communication error: %s", strerror(errno));
  errored();
}

FDdriver::~FDdriver ()
{
  TRACEPRINTF (t, 2, "Close");

  if (fd != -1)
    {
      sendbuf.stop(true);
      recvbuf.stop(true);
      close (fd);
    }
}

void
FDdriver::send_Data(CArray &c)
{
  CArray *cp = new CArray(c);
  t->TracePacket (0, "Write", c);
  sendbuf.write(cp);
}

void
FDdriver::start()
{
  setup_buffers();
}

void
FDdriver::stop()
{
  if (fd >= -1)
    {
      sendbuf.stop(true);
      recvbuf.stop(true);

      close(fd);
      fd = -1;
    }
  LowLevelDriver::stop();
}

size_t
FDdriver::read_cb(uint8_t *buf, size_t len)
{
  t->TracePacket (0, "Read", len, buf);

  CArray c(buf,len);
  recv_Data(c);
  return len;
}

