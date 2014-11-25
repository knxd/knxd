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
#include <errno.h>
#include "bcu1.h"

BCU1DriverLowLevelDriver::BCU1DriverLowLevelDriver (const char *dev,
						    Trace * tr)
{
  t = tr;
  pth_sem_init (&in_signal);
  pth_sem_init (&out_signal);
  pth_sem_init (&send_empty);
  pth_sem_set_value (&send_empty, 1);
  getwait = pth_event (PTH_EVENT_SEM, &out_signal);
  send_done = 0;

  TRACEPRINTF (t, 1, this, "Open");
  fd = open (dev, O_RDWR);
  if (fd == -1)
    return;

  pth_event_t timeout = pth_event (PTH_EVENT_RTIME, pth_time (0, 1));
  send_done = pth_event (PTH_EVENT_FD | PTH_UNTIL_FD_EXCEPTION, fd);
  pth_event_concat (send_done, timeout, NULL);

  pth_wait (send_done);

  pth_event_isolate (send_done);
  pth_event_free (timeout, PTH_FREE_THIS);

  if (pth_event_status (send_done) != PTH_STATUS_OCCURRED)
    {
      TRACEPRINTF (t, 1, this, "Driver select extension missing");
      pth_event_free (send_done, PTH_FREE_THIS);
      send_done = 0;
    }

  Start ();
  TRACEPRINTF (t, 1, this, "Opened");
}

BCU1DriverLowLevelDriver::~BCU1DriverLowLevelDriver ()
{
  TRACEPRINTF (t, 1, this, "Close");
  Stop ();
  if (send_done)
    pth_event_free (send_done, PTH_FREE_THIS);

  pth_event_free (getwait, PTH_FREE_THIS);

  if (fd != -1)
    close (fd);
}

bool BCU1DriverLowLevelDriver::init ()
{
  return fd != -1;
}

bool BCU1DriverLowLevelDriver::Connection_Lost ()
{
  return 0;
}

void
BCU1DriverLowLevelDriver::Send_Packet (CArray l)
{
  CArray pdu;
  t->TracePacket (1, this, "Send", l);
  inqueue.put (l);
  pth_sem_set_value (&send_empty, 0);
  pth_sem_inc (&in_signal, TRUE);
}

void
BCU1DriverLowLevelDriver::SendReset ()
{
}

bool BCU1DriverLowLevelDriver::Send_Queue_Empty ()
{
  return inqueue.isempty ();
}

pth_sem_t *
BCU1DriverLowLevelDriver::Send_Queue_Empty_Cond ()
{
  return &send_empty;
}

CArray *
BCU1DriverLowLevelDriver::Get_Packet (pth_event_t stop)
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

void
BCU1DriverLowLevelDriver::Run (pth_sem_t * stop1)
{
  int i;
  uchar buf[255];
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  pth_event_t input = pth_event (PTH_EVENT_SEM, &in_signal);
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      pth_event_concat (stop, input, NULL);
      i = pth_read_ev (fd, buf, sizeof (buf), stop);
      if (i > 0)
	{
	  t->TracePacket (0, this, "Recv", i, buf);
	  outqueue.put (new CArray (buf, i));
	  pth_sem_inc (&out_signal, 1);
	}
      pth_event_isolate (stop);
      if (!inqueue.isempty ())
	{
	  const CArray & c = inqueue.top ();
	  t->TracePacket (0, this, "Send", c);
	  i = pth_write_ev (fd, c.array (), c (), stop);
	  if (i == c ())
	    {
	      if (send_done)
		pth_wait (send_done);

	      pth_sem_dec (&in_signal);
	      inqueue.get ();
	      if (inqueue.isempty ())
		pth_sem_set_value (&send_empty, 1);
	    }
	}
    }
  pth_event_free (stop, PTH_FREE_THIS);
  pth_event_free (input, PTH_FREE_THIS);
}

LowLevelDriverInterface::EMIVer BCU1DriverLowLevelDriver::getEMIVer ()
{
  return vEMI1;
}
