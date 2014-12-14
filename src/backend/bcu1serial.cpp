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
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include "bcu1serial.h"

void
BCU1SerialLowLevelDriver::SendReset ()
{
}

bool
BCU1SerialLowLevelDriver::Connection_Lost ()
{
  return 0;
}

void
BCU1SerialLowLevelDriver::Send_Packet (CArray l)
{
  CArray pdu;
  t->TracePacket (1, this, "Send", l);
  inqueue.put (l);
  pth_sem_set_value (&send_empty, 0);
  pth_sem_inc (&in_signal, TRUE);
}

bool
BCU1SerialLowLevelDriver::Send_Queue_Empty ()
{
  return inqueue.isempty ();
}

pth_sem_t *
BCU1SerialLowLevelDriver::Send_Queue_Empty_Cond ()
{
  return &send_empty;
}

CArray *
BCU1SerialLowLevelDriver::Get_Packet (pth_event_t stop)
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

BCU1SerialLowLevelDriver::BCU1SerialLowLevelDriver (const char *dev,
						    Trace * tr)
{
  struct termios ti;

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
  fd = open (dev, O_RDWR | O_NOCTTY | O_NDELAY | O_SYNC);
  if (fd == -1)
    return;

  if (tcgetattr (fd, &told))
    {
      restore_low_latency (fd, &sold);
      close (fd);
      fd = -1;
      return;
    }

  if (tcgetattr (fd, &ti))
    {
      restore_low_latency (fd, &sold);
      close (fd);
      fd = -1;
      return;
    }
  ti.c_cflag = CS8 | CLOCAL | CREAD;
  ti.c_iflag = IGNBRK | INPCK | ISIG;
  ti.c_oflag = 0;
  ti.c_lflag = 0;
  ti.c_cc[VTIME] = 1;
  ti.c_cc[VMIN] = 0;
  cfsetospeed (&ti, B9600);
  cfsetispeed (&ti, 0);

  if (tcsetattr (fd, TCSAFLUSH, &ti))
    {
      restore_low_latency (fd, &sold);
      close (fd);
      fd = -1;
      return;
    }

  setstat (getstat () & ~(TIOCM_RTS | TIOCM_CTS));
  while ((getstat () & TIOCM_CTS));

  Start ();
  TRACEPRINTF (t, 1, this, "Opened");
}

BCU1SerialLowLevelDriver::~BCU1SerialLowLevelDriver ()
{
  TRACEPRINTF (t, 1, this, "Close");
  Stop ();
  pth_event_free (getwait, PTH_FREE_THIS);

  if (fd != -1)
    {
      restore_low_latency (fd, &sold);
      tcsetattr (fd, TCSAFLUSH, &told);
      close (fd);
    }
}

bool
BCU1SerialLowLevelDriver::init ()
{
  return fd != -1;
}

int
BCU1SerialLowLevelDriver::getstat ()
{
  int s;
  ioctl (fd, TIOCMGET, &s);
  return s;
}

void
BCU1SerialLowLevelDriver::setstat (int s)
{
  ioctl (fd, TIOCMSET, &s);
}

/** counts to number of set bits in x */
int
bitcount (unsigned char x)
{
  int i, j;
  for (i = j = 0; i < 8; i++)
    if (x & (1 << i))
      j++;
  return j;
}

bool
BCU1SerialLowLevelDriver::startsync ()
{
  int to = 0;
  int s = getstat ();
  setstat (s | TIOCM_RTS);
  while (!(getstat () & TIOCM_CTS))
    {
      to++;
      if (to > 0x10000)
	return false;
    }
  TRACEPRINTF (t, 0, this, "Startsync");
  return true;
}

bool
BCU1SerialLowLevelDriver::endsync ()
{
  int to = 0;
  int s = getstat ();

  setstat (s & ~TIOCM_RTS);
  while ((getstat () & TIOCM_CTS))
    {
      to++;
      if (to > 0x10000)
	return false;
    }
  TRACEPRINTF (t, 0, this, "Endsync");
  return true;
}

bool
  BCU1SerialLowLevelDriver::exchange (uchar c, uchar & result,
				      pth_event_t stop)
{
  uchar s;
  int i;
  int to = 0;

  write (fd, &c, 1);
  while ((i = read (fd, &s, 1)) == -1)
    {
      to++;
      if (to > 0x10000)
	return false;
    }
  TRACEPRINTF (t, 0, this, "exchange %02X <->%02X", c, s);
  result = s;
  return true;
}

void
BCU1SerialLowLevelDriver::Run (pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  pth_event_t input = pth_event (PTH_EVENT_SEM, &in_signal);
  pth_event_t timeout = pth_event (PTH_EVENT_RTIME, pth_time (0, 10));
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      int error;
      timeout =
	pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout,
		   pth_time (0, 150));
      pth_event_concat (stop, input, timeout, NULL);
      pth_wait (stop);
      pth_event_isolate (stop);
      pth_event_isolate (input);
      timeout =
	pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout,
		   pth_time (0, 200));
      pth_event_concat (stop, timeout, NULL);

      struct timeval v1, v2;
      gettimeofday (&v1, 0);

      CArray e;
      CArray r;
      uchar s;
      if (!inqueue.isempty ())
	{
	  const CArray & c = inqueue.top ();
	  e.resize (c () + 1);
	  s = c () & 0x1f;
	  s |= 0x20;
	  s |= 0x80 * bitcount (s);
	  e[0] = s;
	  e.setpart (c, 1);
	}
      else
	{
	  e.resize (1);
	  e[0] = 0xff;
	}
      if (!startsync ())
	{
	  error = 1;
	  goto err;
	}
      if (!exchange (e[0], s, stop))
	{
	  error = 3;
	  goto err;
	}
      if (!endsync ())
	{
	  error = 2;
	  goto err;
	}
      if (s == 0xff && e[0] != 0xff)
	{
	  for (unsigned i = 1; i < e (); i++)
	    {
	      if (!startsync ())
		{
		  error = 1;
		  goto err;
		}
	      if (!exchange (e[i], s, stop))
		{
		  error = 3;
		  goto err;
		}
	      if (endsync ())
		{
		  error = 2;
		  goto err;
		}
	    }
	  if (s != 0x00)
	    {
	      error = 10;
	      goto err;
	    }
	  inqueue.get ();
	  TRACEPRINTF (t, 0, this, "Sent");
	  pth_sem_dec (&in_signal);
	  if (inqueue.isempty ())
	    pth_sem_set_value (&send_empty, 1);
	}
      else if (s != 0xff)
	{
	  r.resize ((s & 0x1f));
	  for (unsigned i = 0; i < (s & 0x1f); i++)
	    {
	      if (!startsync ())
		{
		  error = 1;
		  goto err;
		}
	      if (!exchange (0, r[i], stop))
		{
		  error = 3;
		  goto err;
		}
	      if (!endsync ())
		{
		  error = 2;
		  goto err;
		}
	    }
	  TRACEPRINTF (t, 0, this, "Recv");
	  outqueue.put (new CArray (r));
	  pth_sem_inc (&out_signal, 1);
	}
      gettimeofday (&v2, 0);
      TRACEPRINTF (t, 1, this, "Recvtime: %d",
		   v2.tv_sec * 1000000L + v2.tv_usec -
		   (v1.tv_sec * 1000000L + v1.tv_usec));

      if (0)
	{
	err:
	  gettimeofday (&v2, 0);
	  TRACEPRINTF (t, 1, this, "ERecvtime: %d",
		       v2.tv_sec * 1000000L + v2.tv_usec -
		       (v1.tv_sec * 1000000L + v1.tv_usec));
	  setstat (getstat () & ~(TIOCM_RTS | TIOCM_CTS));
	  pth_usleep (2000);
	  while ((getstat () & TIOCM_CTS));
	  TRACEPRINTF (t, 0, this, "Restart %d", error);
	}

      pth_event_isolate (timeout);
    }
  pth_event_free (timeout, PTH_FREE_THIS);
  pth_event_free (stop, PTH_FREE_THIS);
  pth_event_free (input, PTH_FREE_THIS);
}

LowLevelDriverInterface::EMIVer BCU1SerialLowLevelDriver::getEMIVer ()
{
  return vEMI1;
}
