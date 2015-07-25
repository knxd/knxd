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

DummyL2Driver::DummyL2Driver (Layer3 *l3, L2options *opt) : Layer2 (l3, opt)
{
  TRACEPRINTF (t, 2, this, "Open");

  layer2_is_bus();
  Start ();

  TRACEPRINTF (t, 2, this, "Openend");
}

DummyL2Driver::~DummyL2Driver ()
{
  TRACEPRINTF (t, 2, this, "Close");
  Stop ();
}

void
DummyL2Driver::Send_L_Data (LPDU * l)
{
  TRACEPRINTF (t, 2, this, "Send %s", l->Decode ()());
  if ((mode & BUSMODE_MONITOR) && l->getType () == L_Data)
    {
      L_Busmonitor_PDU *l2 = new L_Busmonitor_PDU (shared_from_this());
      l2->pdu.set (l->ToPacket ());
      l3->recv_L_Data (l2);
    }
  delete l;
}

//Open

void
DummyL2Driver::Run (pth_sem_t * stop1)
{
  TRACEPRINTF (t, 2, this, "DummyStart");
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  pth_wait(stop);
  pth_event_free (stop, PTH_FREE_THIS);
  TRACEPRINTF (t, 2, this, "DummyEnd");
}
