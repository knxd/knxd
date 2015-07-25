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
						    L2options *opt, Layer3 * l3)
	: Layer2::Layer2(l3, opt)
{
  struct termios t1;
  TRACEPRINTF (t, 2, this, "Open");

  pth_sem_init (&in_signal);

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

  Start ();
  TRACEPRINTF (t, 2, this, "Openend");
}

NCN5120SerialLayer2Driver::~NCN5120SerialLayer2Driver ()
{
  TRACEPRINTF (t, 2, this, "Close");
  Stop ();

  while (!inqueue.isempty ())
    delete inqueue.get ();

  if (fd != -1)
    {
      setstat (fd, (getstat (fd) & ~TIOCM_RTS) & ~TIOCM_DTR);
      tcsetattr (fd, TCSAFLUSH, &old);
      restore_low_latency (fd, &sold);
      close (fd);
    }

}

bool NCN5120SerialLayer2Driver::init ()
{
  if (fd == -1)
    return false;
  if (! layer2_is_bus())
    return false;
  return Layer2::init ();
}

bool
NCN5120SerialLayer2Driver::Send_Queue_Empty ()
{
  return inqueue.isempty ();
}

void
NCN5120SerialLayer2Driver::Send_L_Data (LPDU * l)
{
  TRACEPRINTF (t, 2, this, "Send %s", l->Decode ()());
  inqueue.put (l);
  pth_sem_inc (&in_signal, 1);
}

//Open

bool
NCN5120SerialLayer2Driver::enterBusmonitor ()
{
  if (! Layer2::enterBusmonitor ())
	return false;
  uchar c = 0x05;
  t->TracePacket (2, this, "openBusmonitor", 1, &c);
  write (fd, &c, 1);
  return true;
}

bool
NCN5120SerialLayer2Driver::leaveBusmonitor ()
{
  if (! Layer2::leaveBusmonitor ())
	return false;
  uchar c = 0x01;
  t->TracePacket (2, this, "leaveBusmonitor", 1, &c);
  write (fd, &c, 1);
  return true;
}

bool
NCN5120SerialLayer2Driver::Open ()
{
  if (! Layer2::Open ())
	return false;
  uchar c = 0x01;
  t->TracePacket (2, this, "open-reset", 1, &c);
  write (fd, &c, 1);
  return 1;
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
NCN5120SerialLayer2Driver::Run (pth_sem_t * stop1)
{
  uchar buf[255];
  int i;
  CArray in;
  int to = 0;
  int waitconfirm = 0;
  int acked = 0;
  int retry = 0;
  int watch = 0;
  bool rmn = false;
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  pth_event_t input = pth_event (PTH_EVENT_SEM, &in_signal);
  pth_event_t timeout = pth_event (PTH_EVENT_RTIME, pth_time (0, 0));
  pth_event_t watchdog = pth_event (PTH_EVENT_RTIME, pth_time (0, 0));
  pth_event_t sendtimeout = pth_event (PTH_EVENT_RTIME, pth_time (0, 0));
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      if (in () == 0 && !waitconfirm)
	pth_event_concat (stop, input, NULL);
      if (to)
	pth_event_concat (stop, timeout, NULL);
      if (waitconfirm)
	pth_event_concat (stop, sendtimeout, NULL);
      if (watch)
	pth_event_concat (stop, watchdog, NULL);
      i = pth_read_ev (fd, buf, sizeof (buf), stop);
      pth_event_isolate (stop);
      pth_event_isolate (timeout);
      pth_event_isolate (sendtimeout);
      pth_event_isolate (watchdog);
      if (i > 0)
	{
	  t->TracePacket (0, this, "Recv", i, buf);
	  in.setpart (buf, in (), i);
	}
      while (in () > 0)
	{
	  if(rmn)
	    {
	      TRACEPRINTF (t, 0, this, "Remove next");
	      in.deletepart(0, 1);
	      rmn = false;
	      continue;
	    }
	  rmn = false;
	  if (in[0] == 0x8B)
	    {
	      if (waitconfirm)
		{
		  waitconfirm = 0;
		  delete inqueue.get ();
		  pth_sem_dec (&in_signal);
		  retry = 0;
		}
	      in.deletepart (0, 1);
	    }
	  else if (in[0] == 0x0B)
	    {
	      if (waitconfirm)
		{
		  retry++;
		  waitconfirm = 0;
		  TRACEPRINTF (t, 0, this, "NACK");
		  if (retry > 3)
		    {
		      TRACEPRINTF (t, 0, this, "Drop NACK");
		      delete inqueue.get ();
		      pth_sem_dec (&in_signal);
		      retry = 0;
		    }
		}
	      in.deletepart (0, 1);
	    }
	  else if ((in[0] & 0x07) == 0x07)
	    {
	      TRACEPRINTF (t, 0, this, "RecvWatchdog: %02X", in[0]);
	      watch = 2;
	      pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, watchdog,
			 pth_time (10, 0));
	      in.deletepart (0, 1);
	    }
	  else if (in[0] == 0xCC || in[0] == 0xC0 || in[0] == 0x0C)
	    {
	      RecvLPDU (in.array (), 1);
	      rmn=true;
	      in.deletepart (0, 1);
	    }
	  else if ((in[0] & 0xD0) == 0x90)
	    {
	      if (in () < 6)
		{
		  if (!to)
		    {
		      to = 1;
		      pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout,
				 pth_time (0, 300000));
		    }
		  if (pth_event_status (timeout) != PTH_STATUS_OCCURRED)
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
		  pth_write_ev (fd, &c, 1, stop);
		  acked = 1;
		}
	      unsigned len = in[5] & 0x0f;
	      len += 6 + 2;
	      if (in () < len)
		{
		  if (!to)
		    {
		      to = 1;
		      pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout,
				 pth_time (0, 300000));
		    }
		  if (pth_event_status (timeout) != PTH_STATUS_OCCURRED)
		    break;
		  TRACEPRINTF (t, 0, this, "Remove2 %02X", in[0]);
		  in.deletepart (0, 1);
		  continue;
		}
	      acked = 0;
	      RecvLPDU (in.array (), len);
	      rmn=true;
	      in.deletepart (0, len);
	    }
	  else
	    {
	      acked = 0;
	      TRACEPRINTF (t, 0, this, "Remove %02X", in[0]);
	      in.deletepart (0, 1);
	    }
	  to = 0;
	}
      if (waitconfirm
	  && pth_event_status (sendtimeout) == PTH_STATUS_OCCURRED)
	{
	  retry++;
	  waitconfirm = 0;
	  if (retry >= 3)
	    {
	      TRACEPRINTF (t, 0, this, "Drop Send");
	      delete inqueue.get ();
	      pth_sem_dec (&in_signal);
	    }
	}
      if (watch == 1 && pth_event_status (watchdog) == PTH_STATUS_OCCURRED
	  && mode != BUSMODE_MONITOR)
	{
	  if (dischreset)
	    {
	      setstat (fd, (getstat (fd) & ~TIOCM_RTS) & ~TIOCM_DTR);
	      pth_usleep (2000);
	      setstat (fd, (getstat (fd) & ~TIOCM_RTS) | TIOCM_DTR);
	      pth_usleep (1000);
	    }

	  uchar c = 0x01;
	  t->TracePacket (2, this, "Watchdog Reset", 1, &c);
	  write (fd, &c, 1);
	  watch = 0;
	}
      if (watch == 1 && pth_event_status (watchdog) == PTH_STATUS_OCCURRED
	  && mode == BUSMODE_MONITOR)
	watch = 0;
      if (watch == 2 && pth_event_status (watchdog) == PTH_STATUS_OCCURRED)
	watch = 0;
      if (in () == 0 && !inqueue.isempty () && !waitconfirm)
	{
	  LPDU *l = (LPDU *) inqueue.top ();
	  CArray d = l->ToPacket ();
	  CArray w;
	  unsigned i;
	  w.resize (d () * 2);
	  for (i = 0; i < d (); i++)
	    {
	      w[2 * i] = 0x80 | (i & 0x3f);
	      w[2 * i + 1] = d[i];
	    }
	  w[(d () * 2) - 2] = (w[(d () * 2) - 2] & 0x3f) | 0x40;
	  t->TracePacket (0, this, "Write", w);
	  (void) pth_write_ev (fd, w.array (), w (), stop);
	  waitconfirm = 1;
	  pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, sendtimeout,
		     pth_time (0, 600000));
	}
      else if (in () == 0 && !waitconfirm && !watch && mode != BUSMODE_MONITOR && !to)
	{
	  pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, watchdog,
		     pth_time (10, 0));
	  watch = 1;
	  uchar c = 0x02;
	  t->TracePacket (2, this, "Watchdog Status", 1, &c);
	  write (fd, &c, 1);
	}
    }
  pth_event_free (stop, PTH_FREE_THIS);
  pth_event_free (input, PTH_FREE_THIS);
  pth_event_free (timeout, PTH_FREE_THIS);
  pth_event_free (watchdog, PTH_FREE_THIS);
  pth_event_free (sendtimeout, PTH_FREE_THIS);
}
