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
#include <sys/ioctl.h>
#include "dummy.h"
#include "layer3.h"

DummyL2Driver::DummyL2Driver (Layer3 *l3) : Layer2Interface (l3)
{
  TRACEPRINTF (t, 2, this, "Open");

  pth_sem_init (&outsignal);
  getwait = pth_event (PTH_EVENT_SEM, &outsignal);

  mode = 0;
  vmode = 0;

  Start ();
  TRACEPRINTF (t, 2, this, "Openend");
}

DummyL2Driver::~DummyL2Driver ()
{
  TRACEPRINTF (t, 2, this, "Close");
  Stop ();
  pth_event_free (getwait, PTH_FREE_THIS);

  while (!outqueue.isempty ())
    delete outqueue.get ();
}

bool DummyL2Driver::openVBusmonitor ()
{
  vmode = 1;
  return 1;
}

bool DummyL2Driver::closeVBusmonitor ()
{
  vmode = 0;
  return 1;
}

bool
DummyL2Driver::Connection_Lost ()
{
  return false;
}

bool
DummyL2Driver::Send_Queue_Empty ()
{
  return true;
}

void
DummyL2Driver::Send_L_Data (LPDU * l)
{
  TRACEPRINTF (t, 2, this, "Send %s", l->Decode ()());
  if (vmode && l->getType () == L_Data)
    {
      L_Busmonitor_PDU *l2 = new L_Busmonitor_PDU (this);
      l2->pdu.set (l->ToPacket ());
      outqueue.put (l2);
      pth_sem_inc (&outsignal, 1);
    }
  delete l;
}

LPDU *
DummyL2Driver::Get_L_Data (pth_event_t stop)
{
  if (stop != NULL)
    pth_event_concat (getwait, stop, NULL);

  pth_wait (getwait);

  if (stop)
    pth_event_isolate (getwait);

  if (pth_event_status (getwait) == PTH_STATUS_OCCURRED)
    {
      pth_sem_dec (&outsignal);
      LPDU *l = outqueue.get ();
      TRACEPRINTF (t, 2, this, "Recv %s", l->Decode ()());
      return l;
    }
  else
    return 0;
}


//Open

bool
DummyL2Driver::enterBusmonitor ()
{
  mode = 1;
  return 1;
}

bool
DummyL2Driver::leaveBusmonitor ()
{
  mode = 0;
  return 1;
}

bool
DummyL2Driver::Open ()
{
  return 1;
}

bool
DummyL2Driver::Close ()
{
  return 1;
}

void
DummyL2Driver::Run (pth_sem_t * stop1)
{
  TRACEPRINTF (t, 2, this, "DummyStart");
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  pth_wait(stop);
  pth_event_free (stop, PTH_FREE_THIS);
  TRACEPRINTF (t, 2, this, "DummyEnd");
}
