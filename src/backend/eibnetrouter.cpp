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

#include "eibnetrouter.h"
#include "emi.h"
#include "config.h"
#include "layer3.h"

EIBNetIPRouter::EIBNetIPRouter (const char *multicastaddr, int port,
				eibaddr_t a UNUSED, Layer3 * l3) : Layer2Interface (l3)
{
  struct sockaddr_in baddr;
  struct ip_mreq mcfg;
  TRACEPRINTF (t, 2, this, "Open");
  mode = 0;
  vmode = 0;
  memset (&baddr, 0, sizeof (baddr));
#ifdef HAVE_SOCKADDR_IN_LEN
  baddr.sin_len = sizeof (baddr);
#endif
  baddr.sin_family = AF_INET;
  baddr.sin_port = htons (port);
  baddr.sin_addr.s_addr = htonl (INADDR_ANY);
  sock = new EIBNetIPSocket (baddr, 1, t);
  if (!sock->init ())
    {
      delete sock;
      sock = 0;
      return;
    }
  sock->recvall = 2;
  if (GetHostIP (&sock->sendaddr, multicastaddr) == 0)
    {
      delete sock;
      sock = 0;
      return;
    }
  sock->sendaddr.sin_port = htons (port);
  if (!GetSourceAddress (&sock->sendaddr, &sock->localaddr))
    return;
  sock->localaddr.sin_port = sock->sendaddr.sin_port;

  mcfg.imr_multiaddr = sock->sendaddr.sin_addr;
  mcfg.imr_interface.s_addr = htonl (INADDR_ANY);
  if (!sock->SetMulticast (mcfg))
    {
      delete sock;
      sock = 0;
      return;
    }
  Start ();
  TRACEPRINTF (t, 2, this, "Opened");
}

EIBNetIPRouter::~EIBNetIPRouter ()
{
  TRACEPRINTF (t, 2, this, "Destroy");
  Stop ();
  if (sock)
    delete sock;
}

bool
EIBNetIPRouter::init ()
{
  if (sock == 0)
    return false;
  return Layer2Interface::init ();
}

void
EIBNetIPRouter::Send_L_Data (LPDU * l)
{
  TRACEPRINTF (t, 2, this, "Send %s", l->Decode ()());
  if (l->getType () != L_Data)
    {
      delete l;
      return;
    }
  L_Data_PDU *l1 = (L_Data_PDU *) l;
  EIBNetIPPacket p;
  p.data = L_Data_ToCEMI (0x29, *l1);
  p.service = ROUTING_INDICATION;
  sock->Send (p);
  if (vmode)
    {
      L_Busmonitor_PDU *l2 = new L_Busmonitor_PDU (this);
      l2->pdu.set (l->ToPacket ());
      l3->recv_L_Data (l2);
    }
  l3->recv_L_Data (l);
}

void
EIBNetIPRouter::Run (pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      EIBNetIPPacket *p = sock->Get (stop);
      if (p)
	{
	  if (p->service != ROUTING_INDICATION)
	    {
	      delete p;
	      continue;
	    }
	  if (p->data () < 2 || p->data[0] != 0x29)
	    {
              if (p->data () < 2)
                {
	          TRACEPRINTF (t, 2, this, "No payload (%d)", p->data ());
                }
              else
                {
	          TRACEPRINTF (t, 2, this, "Payload not L_Data.ind (%02x)", p->data[0]);
                }
	      delete p;
	      continue;
	    }
	  const CArray data = p->data;
	  delete p;
	  L_Data_PDU *c = CEMI_to_L_Data (data, this);
	  if (c)
	    {
	      TRACEPRINTF (t, 2, this, "Recv %s", c->Decode ()());
	      if (mode == 0)
		{
		  if (vmode)
		    {
		      L_Busmonitor_PDU *l2 = new L_Busmonitor_PDU (this);
		      l2->pdu.set (c->ToPacket ());
		      l3->recv_L_Data (l2);
		    }
		  l3->recv_L_Data (c);
		  continue;
		}
	      L_Busmonitor_PDU *p1 = new L_Busmonitor_PDU (this);
	      p1->pdu = c->ToPacket ();
	      delete c;
	      l3->recv_L_Data (p1);
	      continue;
	    }
	}
    }
  pth_event_free (stop, PTH_FREE_THIS);
}

bool
EIBNetIPRouter::openVBusmonitor ()
{
  vmode = 1;
  return 1;
}

bool
EIBNetIPRouter::closeVBusmonitor ()
{
  vmode = 0;
  return 1;
}

bool
EIBNetIPRouter::enterBusmonitor ()
{
  mode = 1;
  return 1;
}

bool
EIBNetIPRouter::leaveBusmonitor ()
{
  mode = 0;
  return 1;
}

bool
EIBNetIPRouter::Open ()
{
  mode = 0;
  return 1;
}

bool
EIBNetIPRouter::Close ()
{
  return 1;
}

bool
EIBNetIPRouter::Connection_Lost ()
{
  return 0;
}

bool
EIBNetIPRouter::Send_Queue_Empty ()
{
  return 1;
}
