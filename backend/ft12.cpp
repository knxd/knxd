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

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "ft12.h"

FT12LowLevelDriver::FT12LowLevelDriver (const char *dev, Trace * tr)
{
  struct termios t1;
  t = tr;
  pth_sem_init (&in_signal);
  pth_sem_init (&out_signal);
  pth_sem_init (&send_empty);
  pth_sem_set_value (&send_empty, 1);
  getwait = pth_event (PTH_EVENT_SEM, &out_signal);

  TRACEPRINTF (t, 1, this, "Open");

  fd = open (dev, O_RDWR | O_NOCTTY | O_NDELAY | O_SYNC);
  if (fd == -1)
    return;
  set_low_latency (fd, &sold);

  close (fd);

  fd = open (dev, O_RDWR | O_NOCTTY);
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
  t1.c_cflag = CS8 | PARENB | CLOCAL | CREAD;
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
  sendflag = 0;
  recvflag = 0;
  repeatcount = 0;
  mode = 0;
  Start ();
  TRACEPRINTF (t, 1, this, "Opened");
}

FT12LowLevelDriver::~FT12LowLevelDriver ()
{
  Stop ();
  pth_event_free (getwait, PTH_FREE_THIS);

  while (!outqueue.isempty ())
    delete outqueue.get ();

  TRACEPRINTF (t, 1, this, "Close");
  if (fd != -1)
    {
      tcsetattr (fd, TCSAFLUSH, &old);
      restore_low_latency (fd, &sold);
      close (fd);
    }
}

bool FT12LowLevelDriver::init ()
{
  return fd != -1;
}

void
FT12LowLevelDriver::Send_Packet (CArray l)
{
  CArray pdu;
  uchar c;
  unsigned i;
  t->TracePacket (1, this, "Send", l);

  assert (l () <= 32);
  pdu.resize (l () + 7);
  pdu[0] = 0x68;
  pdu[1] = l () + 1;
  pdu[2] = l () + 1;
  pdu[3] = 0x68;
  if (sendflag)
    pdu[4] = 0x53;
  else
    pdu[4] = 0x73;
  sendflag = !sendflag;

  pdu.setpart (l.array (), 5, l ());
  c = pdu[4];
  for (i = 0; i < l (); i++)
    c += l[i];
  pdu[pdu () - 2] = c;
  pdu[pdu () - 1] = 0x16;

  inqueue.put (pdu);
  pth_sem_set_value (&send_empty, 0);
  pth_sem_inc (&in_signal, TRUE);
}

void
FT12LowLevelDriver::SendReset ()
{
  CArray pdu;
  TRACEPRINTF (t, 1, this, "SendReset");
  pdu.resize (4);
  pdu[0] = 0x10;
  pdu[1] = 0x40;
  pdu[2] = 0x40;
  pdu[3] = 0x16;
  sendflag = 0;
  recvflag = 0;
  inqueue.put (pdu);
  pth_sem_set_value (&send_empty, 0);
  pth_sem_inc (&in_signal, TRUE);
}

bool
FT12LowLevelDriver::Send_Queue_Empty ()
{
  return inqueue.isempty ();
}

pth_sem_t *
FT12LowLevelDriver::Send_Queue_Empty_Cond ()
{
  return &send_empty;
}

CArray *
FT12LowLevelDriver::Get_Packet (pth_event_t stop)
{
  if (stop != NULL)
    pth_event_concat (getwait, stop, NULL);

  pth_wait (getwait);

  if (stop)
    pth_event_isolate (getwait);

  if (pth_event_status (getwait) == PTH_STATUS_OCCURRED)
    {
      pth_sem_dec (&out_signal);
      CArray *c = outqueue.get ();
      t->TracePacket (1, this, "Recv", *c);
      return c;
    }
  else
    return 0;
}

bool
FT12LowLevelDriver::Connection_Lost ()
{
  return repeatcount > 10;
}

void
FT12LowLevelDriver::Run (pth_sem_t * stop1)
{
  CArray last;
  int i;
  uchar buf[255];

  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  pth_event_t input = pth_event (PTH_EVENT_SEM, &in_signal);
  pth_event_t timeout = pth_event (PTH_EVENT_RTIME, pth_time (0, 100000));
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      pth_event_isolate (input);
      pth_event_isolate (timeout);
      if (mode == 0)
	pth_event_concat (stop, input, NULL);
      if (mode == 1)
	pth_event_concat (stop, timeout, NULL);

      i = pth_read_ev (fd, buf, sizeof (buf), stop);
      if (i > 0)
	{
	  t->TracePacket (0, this, "Recv", i, buf);
	  akt.setpart (buf, akt (), i);
	}

      while (akt.len () > 0)
	{
	  if (akt[0] == 0xE5 && mode == 1)
	    {
	      pth_sem_dec (&in_signal);
	      inqueue.get ();
	      if (inqueue.isempty ())
		pth_sem_set_value (&send_empty, 1);
	      akt.deletepart (0, 1);
	      mode = 0;
	      repeatcount = 0;
	    }
	  else if (akt[0] == 0x10)
	    {
	      if (akt () < 4)
		break;
	      if (akt[1] == akt[2] && akt[3] == 0x16)
		{
		  uchar c1 = 0xE5;
		  t->TracePacket (0, this, "Send Ack", 1, &c1);
		  write (fd, &c1, 1);
		  if ((akt[1] == 0xF3 && !recvflag) ||
		      (akt[1] == 0xD3 && recvflag))
		    {
		      //right sequence number
		      recvflag = !recvflag;
		    }
		  if ((akt[1] & 0x0f) == 0)
		    {
		      const uchar reset[1] = { 0xA0 };
		      CArray *c = new CArray (reset, sizeof (reset));
		      t->TracePacket (0, this, "RecvReset", *c);
		      outqueue.put (c);
		      pth_sem_inc (&out_signal, TRUE);
		    }
		}
	      akt.deletepart (0, 4);
	    }
	  else if (akt[0] == 0x68)
	    {
	      int len;
	      uchar c1;
	      if (akt () < 7)
		break;
	      if (akt[1] != akt[2] || akt[3] != 0x68)
		{
		  //receive error, try to resume
		  akt.deletepart (0, 1);
		  continue;
		}
	      if (akt () < akt[1] + 6)
		break;

	      c1 = 0;
	      for (i = 4; i < akt[1] + 4; i++)
		c1 += akt[i];
	      if (akt[akt[1] + 4] != c1 || akt[akt[1] + 5] != 0x16)
		{
		  len = akt[1] + 6;
		  //Forget wrong short frame
		  akt.deletepart (0, len);
		  continue;
		}

	      c1 = 0xE5;
	      t->TracePacket (0, this, "Send Ack", 1, &c1);
	      i = write (fd, &c1, 1);

	      if ((akt[4] == 0xF3 && recvflag) ||
		  (akt[4] == 0xD3 && !recvflag))
		{
		  if (CArray (akt.array () + 5, akt[1] - 1) != last)
		    {
		      TRACEPRINTF (t, 0, this, "Sequence jump");
		      recvflag = !recvflag;
		    }
		  else
		    TRACEPRINTF (t, 0, this, "Wrong Sequence");
		}

	      if ((akt[4] == 0xF3 && !recvflag) ||
		  (akt[4] == 0xD3 && recvflag))
		{
		  recvflag = !recvflag;
		  CArray *c = new CArray;
		  len = akt[1] + 6;
		  c->setpart (akt.array () + 5, 0, len - 7);
		  last = *c;
		  outqueue.put (c);
		  pth_sem_inc (&out_signal, TRUE);
		}
	      akt.deletepart (0, len);
	    }
	  else
	    //Forget unknown byte
	    akt.deletepart (0, 1);
	}

      if (mode == 1 && pth_event_status (timeout) == PTH_STATUS_OCCURRED)
	mode = 0;

      if (mode == 0 && !inqueue.isempty ())
	{
	  const CArray & c = inqueue.top ();
	  t->TracePacket (0, this, "Send", c);
	  repeatcount++;
	  i = pth_write_ev (fd, c.array (), c (), stop);
	  if (i == c ())
	    {
	      mode = 1;
	      timeout =
		pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout,
			   pth_time (0, 100000));
	    }
	}
    }
  pth_event_free (stop, PTH_FREE_THIS);
  pth_event_free (timeout, PTH_FREE_THIS);
  pth_event_free (input, PTH_FREE_THIS);
}

LowLevelDriverInterface::EMIVer FT12LowLevelDriver::getEMIVer ()
{
  return vEMI2;
}
