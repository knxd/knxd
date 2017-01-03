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
#include "nat.h"
#include "layer3.h"

NatL2Filter::NatL2Filter (L2options *opt, Layer2Ptr l2) : Layer23 (l2)
{
  TRACEPRINTF (t, 2, this, "OpenFilter");
}

NatL2Filter::~NatL2Filter ()
{
  TRACEPRINTF (t, 2, this, "CloseFilter");
}

Layer2Ptr
NatL2Filter::clone (Layer2Ptr l2)
{
  Layer2Ptr c = Layer2Ptr(new NatL2Filter(NULL, l2));
  // now copy our settings to c. In this case there's nothing to copy,
  // because each interface has its own NAT table.
  return c;
}

void
NatL2Filter::Send_L_Data (L_Data_PDU * l)
{
  /* Sending a packet to this interface: record address pair, clear source */
  if (l->getType () == L_Data)
    {
      L_Data_PDU *l1 = dynamic_cast<L_Data_PDU *>(l);
      if (l1->AddrType == IndividualAddress)
        addReverseAddress (l1->source, l1->dest);
      l1->source = 0;
    }
  l2->Send_L_Data (l);
}


void
NatL2Filter::recv_L_Data (LPDU * l)
{
  /* Receiving a packet from this interface: reverse-lookup real destination from source */
  if (l->getType () == L_Data)
    {
      L_Data_PDU *l1 = dynamic_cast<L_Data_PDU *>(l);
      if (l1->AddrType == IndividualAddress)
        l1->dest = getDestinationAddress (l1->source);
    }
  l3->recv_L_Data (l);
}

void NatL2Filter::addReverseAddress (eibaddr_t src, eibaddr_t dest)
{
  ITER(i,revaddr)
    if (i->dest == dest)
      {
        i->src = src;
        return;
      }
  phys_comm srcdest = (phys_comm) { .src=src, .dest=dest };
  revaddr.push_back(srcdest);
}

eibaddr_t NatL2Filter::getDestinationAddress (eibaddr_t src)
{
  ITER(i,revaddr)
    if (i->dest == src)
      return i->src;

  return 0;
}

