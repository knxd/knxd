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
{
  struct termios t1;
  t = tr;
  TRACEPRINTF (t, 2, this, "Open");

  pth_sem_init (&in_signal);
  pth_sem_init (&out_signal);

  ackallgroup = flags & FLAG_B_TPUARTS_ACKGROUP;
  ackallindividual = flags & FLAG_B_TPUARTS_ACKINDIVIDUAL;
  dischreset = flags & FLAG_B_TPUARTS_DISCH_RESET;

  getwait = pth_event (PTH_EVENT_SEM, &out_signal);

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

  t1.c_cflag = CS8 | CLOCAL | CREAD | PARENB;
  t1.c_iflag = IGNBRK | INPCK | ISIG;
  t1.c_oflag = 0;
  t1.c_lflag = 0;
  t1.c_cc[VTIME] = 1;
  t1.c_cc[VMIN] = 0;
  cfsetospeed (&t1, B19200);
  cfsetispeed (&t1, 0);

  if (tcsetattr (fd, TCSAFLUSH, &t1))
    {
      restore_low_latency (fd, &sold);
      close (fd);
      fd = -1;
      return;
    }

  setstat (fd, (getstat (fd) & ~TIOCM_RTS) | TIOCM_DTR);

  mode = 0;
  vmode = 0;
  addr = a;
  indaddr.resize (1);
  indaddr[0] = a;

  Start ();
  TRACEPRINTF (t, 2, this, "Openend");
}

TPUARTSerialLayer2Driver::~TPUARTSerialLayer2Driver ()
{
  TRACEPRINTF (t, 2, this, "Close");
  Stop ();
  pth_event_free (getwait, PTH_FREE_THIS);

  while (!outqueue.isempty ())
    delete outqueue.get ();
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

bool TPUARTSerialLayer2Driver::init ()
{
  return fd != -1;
}

bool
TPUARTSerialLayer2Driver::addAddress (eibaddr_t addr)
{
  unsigned i;
  for (i = 0; i < indaddr (); i++)
    if (indaddr[i] == addr)
      return 0;
  indaddr.resize (indaddr () + 1);
  indaddr[indaddr () - 1] = addr;
  return 1;
}

bool
TPUARTSerialLayer2Driver::addGroupAddress (eibaddr_t addr)
{
  unsigned i;
  for (i = 0; i < groupaddr (); i++)
    if (groupaddr[i] == addr)
      return 0;
  groupaddr.resize (groupaddr () + 1);
  groupaddr[groupaddr () - 1] = addr;
  return 1;
}

bool
TPUARTSerialLayer2Driver::removeAddress (eibaddr_t addr)
{
  unsigned i;
  for (i = 0; i < indaddr (); i++)
    if (indaddr[i] == addr)
      {
	indaddr.deletepart (i, 1);
	return 1;
      }
  return 0;
}

bool
TPUARTSerialLayer2Driver::removeGroupAddress (eibaddr_t addr)
{
  unsigned i;
  for (i = 0; i < groupaddr (); i++)
    if (groupaddr[i] == addr)
      {
	groupaddr.deletepart (i, 1);
	return 1;
      }
  return 0;
}

bool TPUARTSerialLayer2Driver::openVBusmonitor ()
{
  vmode = 1;
  return 1;
}

bool TPUARTSerialLayer2Driver::closeVBusmonitor ()
{
  vmode = 0;
  return 1;
}

eibaddr_t
TPUARTSerialLayer2Driver::getDefaultAddr ()
{
  return addr;
}

bool
TPUARTSerialLayer2Driver::Connection_Lost ()
{
  return 0;
}

bool
TPUARTSerialLayer2Driver::Send_Queue_Empty ()
{
  return inqueue.isempty ();
}

void
TPUARTSerialLayer2Driver::Send_L_Data (LPDU * l)
{
  TRACEPRINTF (t, 2, this, "Send %s", l->Decode ()());
  inqueue.put (l);
  pth_sem_inc (&in_signal, 1);
}

LPDU *
TPUARTSerialLayer2Driver::Get_L_Data (pth_event_t stop)
{
  if (stop != NULL)
    pth_event_concat (getwait, stop, NULL);

  pth_wait (getwait);

  if (stop)
    pth_event_isolate (getwait);

  if (pth_event_status (getwait) == PTH_STATUS_OCCURRED)
    {
      pth_sem_dec (&out_signal);
      LPDU *l = outqueue.get ();
      TRACEPRINTF (t, 2, this, "Recv %s", l->Decode ()());
      return l;
    }
  else
    return 0;
}


//Open

bool
TPUARTSerialLayer2Driver::enterBusmonitor ()
{
  uchar c = 0x05;
  t->TracePacket (2, this, "openBusmonitor", 1, &c);
  write (fd, &c, 1);
  mode = 1;
  return 1;
}

bool
TPUARTSerialLayer2Driver::leaveBusmonitor ()
{
  uchar c = 0x01;
  t->TracePacket (2, this, "leaveBusmonitor", 1, &c);
  write (fd, &c, 1);
  mode = 0;
  return 1;
}

bool
TPUARTSerialLayer2Driver::Open ()
{
  uchar c = 0x01;
  t->TracePacket (2, this, "open-reset", 1, &c);
  write (fd, &c, 1);
  return 1;
}

bool
TPUARTSerialLayer2Driver::Close ()
{
  return 1;
}

void
TPUARTSerialLayer2Driver::RecvLPDU (const uchar * data, int len)
{
  t->TracePacket (1, this, "Recv", len, data);
  if (mode || vmode)
    {
      L_Busmonitor_PDU *l = new L_Busmonitor_PDU;
      l->pdu.set (data, len);
      outqueue.put (l);
      pth_sem_inc (&out_signal, 1);
    }
  if (!mode)
    {
      LPDU *l = LPDU::fromPacket (CArray (data, len));
      if (l->getType () == L_Data && ((L_Data_PDU *) l)->valid_checksum)
	{
	  outqueue.put (l);
	  pth_sem_inc (&out_signal, 1);
	}
      else
	delete l;
    }
}

void
TPUARTSerialLayer2Driver::Run (pth_sem_t * stop1)
{
  uchar buf[255];
  int i;
  CArray in;
  int to = 0;
  int waitconfirm = 0;
  int acked = 0;
  int retry = 0;
  int watch = 0;
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
	  if (in[0] == 0x8B)
	    {
	      if (!mode && vmode)
		{
		  const uchar pkt[1] = { 0xCC };
		  RecvLPDU (pkt, 1);
		}
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
	      if (!mode && vmode)
		{
		  const uchar pkt[1] = { 0x0C };
		  RecvLPDU (pkt, 1);
		}
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
		      if (ackallindividual)
			c |= 0x1;
		      else
			for (unsigned i = 0; i < indaddr (); i++)
			  if (indaddr[i] == ((in[3] << 8) | in[4]))
			    c |= 0x1;
		    }
		  else
		    {
		      if (ackallgroup)
			c |= 0x1;
		      else
			for (unsigned i = 0; i < groupaddr (); i++)
			  if (groupaddr[i] == ((in[3] << 8) | in[4]))
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
	      in.deletepart (0, len);
	    }
	  else if ((in[0] & 0xD0) == 0x10)
	    {
	      if (in () < 7)
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
		  if ((in[1] & 0x80) == 0)
		    {
		      if (ackallindividual)
			c |= 0x1;
		      else
			for (unsigned i = 0; i < indaddr (); i++)
			  if (indaddr[i] == ((in[4] << 8) | in[5]))
			    c |= 0x1;
		    }
		  else
		    {
		      if (ackallgroup)
			c |= 0x1;
		      else
			for (unsigned i = 0; i < groupaddr (); i++)
			  if (groupaddr[i] == ((in[4] << 8) | in[5]))
			    c |= 0x1;
		    }
		  TRACEPRINTF (t, 0, this, "SendAck %02X", c);
		  pth_write_ev (fd, &c, 1, stop);
		  acked = 1;
		}
	      unsigned len = in[6] & 0xff;
	      len += 7 + 2;
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
	  && mode == 0)
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
	  && mode)
	watch = 0;
      if (watch == 2 && pth_event_status (watchdog) == PTH_STATUS_OCCURRED)
	watch = 0;
      if (in () == 0 && !inqueue.isempty () && !waitconfirm)
	{
	  LPDU *l = (LPDU *) inqueue.top ();
	  CArray d = l->ToPacket ();
	  CArray w;
	  unsigned i;
	  int j;
	  w.resize (d () * 2);
	  for (i = 0; i < d (); i++)
	    {
	      w[2 * i] = 0x80 | (i & 0x3f);
	      w[2 * i + 1] = d[i];
	    }
	  w[(d () * 2) - 2] = (w[(d () * 2) - 2] & 0x3f) | 0x40;
	  t->TracePacket (0, this, "Write", w);
	  j = pth_write_ev (fd, w.array (), w (), stop);
	  waitconfirm = 1;
	  pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, sendtimeout,
		     pth_time (0, 600000));
	}
      else if (in () == 0 && !waitconfirm && !watch && mode == 0 && !to)
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
