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
#include "layer3.h"

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


NCN5120SerialLayer2Driver::NCN5120SerialLayer2Driver (const char *dev,
						    L2options *opt)
	: Layer2::Layer2(opt)
{
  struct termios t1;
  TRACEPRINTF (t, 2, this, "Open");

  ackallgroup = opt ? (opt->flags & FLAG_B_TPUARTS_ACKGROUP) : 0;
  ackallindividual = opt ? (opt->flags & FLAG_B_TPUARTS_ACKINDIVIDUAL) : 0;
  dischreset = opt ? (opt->flags & FLAG_B_TPUARTS_DISCH_RESET) : 0;

  if (opt)
    opt->flags &=~ (FLAG_B_TPUARTS_ACKGROUP |
                    FLAG_B_TPUARTS_ACKINDIVIDUAL |
		            FLAG_B_TPUARTS_DISCH_RESET);
  fd = open (dev, O_RDWR | O_NOCTTY | O_NDELAY | O_SYNC);
  if (fd == -1)
    return;
  set_low_latency (fd, &sold);

  close (fd);

  fd = open (dev, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd == -1)
    return;

  if (tcgetattr (fd, &old))
    {
      restore_low_latency (fd, &sold);
      close (fd);
      fd = -1;
      return;
    }

  if (tcgetattr (fd, &t1))
    {
      restore_low_latency (fd, &sold);
      close (fd);
      fd = -1;
      return;
    }

  t1.c_cflag = CS8 | CLOCAL | CREAD;
  t1.c_iflag = IGNBRK | INPCK | ISIG;
  t1.c_oflag = 0;
  t1.c_lflag = 0;
  t1.c_cc[VTIME] = 1;
  t1.c_cc[VMIN] = 0;
  cfsetospeed (&t1, B38400);
  cfsetispeed (&t1, 0);

  if (tcsetattr (fd, TCSAFLUSH, &t1))
    {
      restore_low_latency (fd, &sold);
      close (fd);
      fd = -1;
      return;
    }

  setstat (fd, (getstat (fd) & ~TIOCM_RTS) | TIOCM_DTR);

  TRACEPRINTF (t, 2, this, "Openend");
}

NCN5120SerialLayer2Driver::~NCN5120SerialLayer2Driver ()
{
  TRACEPRINTF (t, 2, this, "Close");
  stop();

  while (!send_q.isempty ())
    delete send_q.get ();

  if (fd != -1)
    {
      setstat (fd, (getstat (fd) & ~TIOCM_RTS) & ~TIOCM_DTR);
      tcsetattr (fd, TCSAFLUSH, &old);
      restore_low_latency (fd, &sold);
      close (fd);
    }
}

bool NCN5120SerialLayer2Driver::init (Layer3 *l3)
{
  if (fd == -1)
    return false;
  if (! addGroupAddress(0))
    return false;

  setup_buffers();
  return Layer2::init (l3);
}

void
NCN5120SerialLayer2Driver::setup_buffers()
{
  sendbuf.init(fd);
  recvbuf.init(fd);

  recvbuf.on_recv_cb.set<NCN5120SerialLayer2Driver,&NCN5120SerialLayer2Driver::read_cb>(this);
  recvbuf.on_error_cb.set<NCN5120SerialLayer2Driver,&NCN5120SerialLayer2Driver::error_cb>(this);
  sendbuf.on_error_cb.set<NCN5120SerialLayer2Driver,&NCN5120SerialLayer2Driver::error_cb>(this);
  timer.set <NCN5120SerialLayer2Driver,&NCN5120SerialLayer2Driver::timer_cb> (this);
  sendtimer.set <NCN5120SerialLayer2Driver,&NCN5120SerialLayer2Driver::sendtimer_cb> (this);
  watchdogtimer.set <NCN5120SerialLayer2Driver,&NCN5120SerialLayer2Driver::watchdogtimer_cb> (this);

  trigger.set<NCN5120SerialLayer2Driver,&NCN5120SerialLayer2Driver::trigger_cb>(this);
  trigger.start();

  sendbuf.start();
  recvbuf.start();
}

void
NCN5120SerialLayer2Driver::Send_L_Data (LPDU * l)
{
  TRACEPRINTF (t, 2, this, "Send %s", l->Decode ().c_str());
  send_q.put (l);
  trigger.send();
}

void
NCN5120SerialLayer2Driver::error_cb()
{
  TRACEPRINTF (t, 2, this, "ERROR");
  stop();
}

void
NCN5120SerialLayer2Driver::stop()
{
  sendbuf.stop();
  recvbuf.stop();
  timer.stop();
  sendtimer.stop();
  watchdogtimer.stop();
  trigger.stop();
}

bool
NCN5120SerialLayer2Driver::enterBusmonitor ()
{
  if (! Layer2::enterBusmonitor ())
	return false;
  uchar c = 0x05;
  t->TracePacket (2, this, "openBusmonitor", 1, &c);
  sendbuf.write (&c, 1);
  return true;
}

bool
NCN5120SerialLayer2Driver::leaveBusmonitor ()
{
  if (! Layer2::leaveBusmonitor ())
	return false;
  uchar c = 0x01;
  t->TracePacket (2, this, "leaveBusmonitor", 1, &c);
  sendbuf.write (&c, 1);
  return true;
}

bool
NCN5120SerialLayer2Driver::Open ()
{
  if (! Layer2::Open ())
	return false;
  uchar c = 0x01;
  t->TracePacket (2, this, "open-reset", 1, &c);
  sendbuf.write (&c, 1);
  return true;
}

void
NCN5120SerialLayer2Driver::RecvLPDU (const uchar * data, int len)
{
  t->TracePacket (1, this, "Recv", len, data);
  if (mode & BUSMODE_MONITOR)
    {
      L_Busmonitor_PDU *l = new L_Busmonitor_PDU (shared_from_this());
      l->pdu.set (data, len);
      l3->recv_L_Data (l);
    }
  if (mode != BUSMODE_MONITOR)
    {
      LPDU *l = LPDU::fromPacket (CArray (data, len), shared_from_this());
      if (l->getType () == L_Data && ((L_Data_PDU *) l)->valid_checksum)
	{
	  l3->recv_L_Data (l);
	}
      else
	delete l;
    }
}

void
NCN5120SerialLayer2Driver::watchdogtimer_cb(ev::timer &w, int revents)
{
  if (watch == 4)
    {
      setstat (fd, (getstat (fd) & ~TIOCM_RTS) | TIOCM_DTR);
      watch = 5;
    }
  else if (watch == 5)
    {
  reset_now:
      uchar c = 0x01;
      t->TracePacket (2, this, "Watchdog Reset", 1, &c);
      sendbuf.write(&c, 1);
      watch = 0;
    }
  else if (watch == 1 && mode != BUSMODE_MONITOR)
    {
      if (dischreset)
	{
	  setstat (fd, (getstat (fd) & ~TIOCM_RTS) & ~TIOCM_DTR);
	  watch = 4;
	  watchdogtimer.start(0.002);
	}
      else
	goto reset_now;
    }
  else if (watch == 1 && mode == BUSMODE_MONITOR)
    watch = 0;
  else if (watch == 2)
    watch = 0;
}

void
NCN5120SerialLayer2Driver::sendtimer_cb (ev::timer &w, int revents)
{
  if (!waitconfirm)
    return;
  retry++;
  waitconfirm = false;
  if (retry >= 3)
    {
      TRACEPRINTF (t, 0, this, "Drop Send");
      delete send_q.get ();
    }
}

void
NCN5120SerialLayer2Driver::trigger_cb (ev::async &w, int revents)
{
  if (waitconfirm)
    return;
  if (in.size() != 0)
    return;
  if (send_q.isempty())
    {
      LPDU *l = (LPDU *) send_q.top ();
      CArray d = l->ToPacket ();
      CArray w;
      unsigned i;
      w.resize (d.size() * 2);
      for (i = 0; i < d.size(); i++)
	{
	  w[2 * i] = 0x80 | (i & 0x3f);
	  w[2 * i + 1] = d[i];
	}
      w[(d.size() * 2) - 2] = (w[(d.size() * 2) - 2] & 0x3f) | 0x40;
      t->TracePacket (0, this, "Write", w);
      sendbuf.write (w.data(), w.size());
      waitconfirm = true;
      sendtimer.start(0.6,0);
    }
  else if (!watch && mode != BUSMODE_MONITOR && !to)
    {
      watchdogtimer.start(10,0);
      watch = 1;
      uchar c = 0x02;
      t->TracePacket (2, this, "Watchdog Status", 1, &c);
      sendbuf.write (&c, 1);
    }
}

size_t
NCN5120SerialLayer2Driver::read_cb(uint8_t *buf, size_t len)
{
  in.setpart (buf, in.size(), len);
  process_read(false);
}


void
NCN5120SerialLayer2Driver::timer_cb (ev::timer &w, int revents)
{
  to = false;
  process_read(true);
}

void
NCN5120SerialLayer2Driver::process_read(bool in_timeout)
{
  while (in.size() > 0)
    {
      if(rmn)
	{
	  TRACEPRINTF (t, 0, this, "Remove next");
	  in.deletepart(0, 1);
	  rmn = false;
	  continue;
	}
      if (in[0] == 0x8B)
	{
	  if (waitconfirm)
	    {
	      sendtimer.stop();
	      waitconfirm = false;
	      delete send_q.get ();
	      retry = 0;
	      if (!send_q.isempty())
		trigger.send();
	    }
	  in.deletepart (0, 1);
	}
      else if (in[0] == 0x0B)
	{
	  if (waitconfirm)
	    {
	      retry++;
	      waitconfirm = false;
	      TRACEPRINTF (t, 0, this, "NACK");
	      if (retry > 3)
		{
		  TRACEPRINTF (t, 0, this, "Drop NACK");
		  delete send_q.get ();
		  retry = 0;
		}
	      if (!send_q.isempty())
		trigger.send();
	    }
	  in.deletepart (0, 1);
	}
      else if ((in[0] & 0x07) == 0x07)
	{
	  TRACEPRINTF (t, 0, this, "RecvWatchdog: %02X", in[0]);
	  watch = 2;
	  watchdogtimer.start(10,0);
	}
      else if (in[0] == 0xCC || in[0] == 0xC0 || in[0] == 0x0C)
	{
	  RecvLPDU (in.data(), 1);
	  rmn=true;
	  in.deletepart (0, 1);
	}
      else if ((in[0] & 0xD0) == 0x90)
	{
	  if (in.size() < 6)
	    {
	      if (!to)
		{
		  to = true;
		  timer.start(0.3,0);
		}
	      if (!in_timeout)
		break;
	      TRACEPRINTF (t, 0, this, "Remove1 %02X", in[0]);
	      in.deletepart (0, 1);
	      continue;
	    }
	  if (!acked)
	    {
	      uchar c = 0x10;
	      if ((in[5] & 0x80) == 0)
		{
		  if (ackallindividual || l3->hasAddress ((in[3] << 8) | in[4], shared_from_this()))
		    c |= 0x1;
		}
	      else
		{
		  if (ackallgroup || l3->hasGroupAddress ((in[3] << 8) | in[4], shared_from_this()))
		    c |= 0x1;
		}
	      TRACEPRINTF (t, 0, this, "SendAck %02X", c);
	      sendbuf.write (&c, 1);
	      acked = true;
	    }
	  unsigned len = in[5] & 0x0f;
	  len += 6 + 2;
	  if (in.size() < len)
	    {
	      if (!to)
		{
		  to = true;
		  timer.start(0.3,0);
		}
	      if (!in_timeout)
		break;
	      TRACEPRINTF (t, 0, this, "Remove2 %02X", in[0]);
	      in.deletepart (0, 1);
	      continue;
	    }
	  acked = false;
	  RecvLPDU (in.data(), len);
	  rmn = true;
	  in.deletepart (0, len);
	}
      else
	{
	  acked = false;
	  TRACEPRINTF (t, 0, this, "Remove %02X", in[0]);
	  in.deletepart (0, 1);
	}
    }

  if (to)
    {
      to = false;
      timer.stop();
    }
}
