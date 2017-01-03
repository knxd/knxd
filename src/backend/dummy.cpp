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

DummyL2Driver::DummyL2Driver (L2options *opt) : Layer2 (opt)
{
  TRACEPRINTF (t, 2, this, "Open");
}

bool
DummyL2Driver::init (Layer3 *l3)
{
    addGroupAddress(0);
    return Layer2::init(l3);
}


DummyL2Driver::~DummyL2Driver ()
{
  TRACEPRINTF (t, 2, this, "Close");
}

void
DummyL2Driver::Send_L_Data (L_Data_PDU * l)
{
  TRACEPRINTF (t, 2, this, "Send %s", l->Decode ().c_str());
  if ((mode & BUSMODE_MONITOR) && l->getType () == L_Data)
  if (mode & BUSMODE_MONITOR)
    {
      L_Busmonitor_PDU *l2 = new L_Busmonitor_PDU (shared_from_this());
      l2->pdu.set (l->ToPacket ());
      l3->recv_L_Busmonitor (l2);
    }
  delete l;
}

DummyL2Filter::DummyL2Filter (L2options *opt, Layer2Ptr l2) : Layer23 (l2)
{
  TRACEPRINTF (t, 2, this, "OpenFilter");
}

void
DummyL2Filter::Send_L_Data (L_Data_PDU * l)
{
  TRACEPRINTF (t, 2, this, "Passing %s", l->Decode ().c_str());
  Layer23::Send_L_Data (l);
}

DummyL2Filter::~DummyL2Filter ()
{
  TRACEPRINTF (t, 2, this, "CloseFilter");
}

Layer2Ptr
DummyL2Filter::clone (Layer2Ptr l2)
{
    Layer2Ptr c = Layer2Ptr(new DummyL2Filter(NULL, l2));
    // now copy our settings to c. In this case there's nothing to copy.
    return c;
}

