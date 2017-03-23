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
#include "router.h"

bool
NatL2Filter::setup()
{
  if (!Filter::setup())
    return false;

  std::string opt = cfg->value("address","");
  if (opt.length() == 0)
    {
      auto c = std::dynamic_pointer_cast<LinkConnect>(conn.lock());
      if (c == nullptr)
        {
          // either the parent has vanished, or the object is not a
          // LinkConnect – which happens when you try to apply the filter
          // globally. The former is exceedingly unlikely, but …
          if (conn.lock() != nullptr)
            ERRORPRINTF(t, E_ERROR, "%s: cannot be used globally");
          return false;
        }
      addr = dynamic_cast<Router *>(&c->router)->addr;
    }
  else
    {
      int a,b,c;
      if (sscanf (opt.c_str(), "%d.%d.%d", &a, &b, &c) != 3 ||
            a<0 || b<0 || c<0 || a>0x0f || b>0x0f || c>0xff)
        {
          ERRORPRINTF(t, E_ERROR, "Address must be #.#.#, not %s",a);
          return false;
        }
      addr = (a << 12) | (b << 8) | c;
    }

  return true;
}

NatL2Filter::~NatL2Filter ()
{
}

void
NatL2Filter::send_L_Data (LDataPtr  l)
{
  /* Sending a packet to this interface: record address pair, clear source */
  if (l->AddrType == IndividualAddress)
    addReverseAddress (l->source, l->dest);
  l->source = addr;
}


void
NatL2Filter::recv_L_Data (LDataPtr  l)
{
  /* Receiving a packet from this interface: reverse-lookup real destination from source */
  if (l->source == addr)
    {
      TRACEPRINTF (t, 5, "drop packet from %s", FormatEIBAddr (l->source));
      return;
    }
  if (l->AddrType == IndividualAddress)
    l->dest = getDestinationAddress (l->source);
  Filter::recv_L_Data (std::move(l));
}

void NatL2Filter::addReverseAddress (eibaddr_t src, eibaddr_t dest)
{
  ITER(i,revaddr)
    if (i->dest == dest)
      {
        if (i->src != src)
	  {
	    TRACEPRINTF (t, 5, "from %s to %s", FormatEIBAddr (src), FormatEIBAddr (dest));
            i->src = src;
	  }
        return;
      }

  TRACEPRINTF (t, 5, "from %s to %s", FormatEIBAddr (src), FormatEIBAddr (dest));
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

