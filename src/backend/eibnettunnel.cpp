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

#include "eibnettunnel.h"
#include "emi.h"

bool
EIBNetIPTunnel::addAddress (eibaddr_t addr)
{
  return 0;
}

bool
EIBNetIPTunnel::removeAddress (eibaddr_t addr)
{
  return 0;
}

bool
EIBNetIPTunnel::addGroupAddress (eibaddr_t addr)
{
  return 1;
}

bool
EIBNetIPTunnel::removeGroupAddress (eibaddr_t addr)
{
  return 1;
}

eibaddr_t
EIBNetIPTunnel::getDefaultAddr ()
{
  return 0;
}

EIBNetIPTunnel::EIBNetIPTunnel (const char *dest, int port, int sport,
				const char *srcip, int Dataport, int flags,
				Trace * tr)
{
  t = tr;
  TRACEPRINTF (t, 2, this, "Open");
  pth_sem_init (&insignal);
  pth_sem_init (&outsignal);
  getwait = pth_event (PTH_EVENT_SEM, &outsignal);
  noqueue = flags & FLAG_B_TUNNEL_NOQUEUE;
  sock = 0;
  if (!GetHostIP (&caddr, dest))
    return;
  caddr.sin_port = htons (port);
  if (!GetSourceAddress (&caddr, &raddr))
    return;
  raddr.sin_port = htons (sport);
  NAT = false;
  dataport = Dataport;
  sock = new EIBNetIPSocket (raddr, 0, t);
  if (!sock->init ())
    {
      delete sock;
      sock = 0;
      return;
    }
  if (srcip)
    {
      if (!GetHostIP (&saddr, srcip))
	{
	  delete sock;
	  sock = 0;
	  return;
	}
      saddr.sin_port = htons (sport);
      NAT = true;
    }
  else
    saddr = raddr;
  sock->sendaddr = caddr;
  sock->recvaddr = caddr;
  sock->recvall = 0;
  mode = 0;
  vmode = 0;
  support_busmonitor = 1;
  connect_busmonitor = 0;
  Start ();
  TRACEPRINTF (t, 2, this, "Opened");
}

EIBNetIPTunnel::~EIBNetIPTunnel ()
{
  TRACEPRINTF (t, 2, this, "Close");
  Stop ();
  while (!outqueue.isempty ())
    delete outqueue.get ();
  pth_event_free (getwait, PTH_FREE_THIS);
  if (sock)
    delete sock;
}

bool EIBNetIPTunnel::init ()
{
  return sock != 0;
}

void
EIBNetIPTunnel::Send_L_Data (LPDU * l)
{
  TRACEPRINTF (t, 2, this, "Send %s", l->Decode ()());
  if (l->getType () != L_Data)
    {
      delete l;
      return;
    }
  L_Data_PDU *l1 = (L_Data_PDU *) l;
  inqueue.put (L_Data_ToCEMI (0x11, *l1));
  pth_sem_inc (&insignal, 1);
  if (vmode)
    {
      L_Busmonitor_PDU *l2 = new L_Busmonitor_PDU;
      l2->pdu.set (l->ToPacket ());
      outqueue.put (l2);
      pth_sem_inc (&outsignal, 1);
    }
  outqueue.put (l);
  pth_sem_inc (&outsignal, 1);
}

LPDU *
EIBNetIPTunnel::Get_L_Data (pth_event_t stop)
{
  if (stop != NULL)
    pth_event_concat (getwait, stop, NULL);

  pth_wait (getwait);

  if (stop)
    pth_event_isolate (getwait);

  if (pth_event_status (getwait) == PTH_STATUS_OCCURRED)
    {
      pth_sem_dec (&outsignal);
      LPDU *c = outqueue.get ();
      if (c)
	TRACEPRINTF (t, 2, this, "Recv %s", c->Decode ()());
      return c;
    }
  else
    return 0;
}

bool
EIBNetIPTunnel::Send_Queue_Empty ()
{
  return inqueue.isempty ();
}

bool
EIBNetIPTunnel::openVBusmonitor ()
{
  vmode = 1;
  return 1;
}

bool
EIBNetIPTunnel::closeVBusmonitor ()
{
  vmode = 0;
  return 1;
}

bool
EIBNetIPTunnel::enterBusmonitor ()
{
  mode = 1;
  if (support_busmonitor)
    connect_busmonitor = 1;
  inqueue.put (CArray ());
  pth_sem_inc (&insignal, 1);
  return 1;
}

bool
EIBNetIPTunnel::leaveBusmonitor ()
{
  mode = 0;
  connect_busmonitor = 0;
  inqueue.put (CArray ());
  pth_sem_inc (&insignal, 1);
  return 1;
}

bool
EIBNetIPTunnel::Open ()
{
  return 1;
}

bool
EIBNetIPTunnel::Close ()
{
  return 1;
}

bool
EIBNetIPTunnel::Connection_Lost ()
{
  return 0;
}

void
EIBNetIPTunnel::Run (pth_sem_t * stop1)
{
  int channel = -1;
  int mod = 0;
  int rno = 0;
  int sno = 0;
  int retry = 0;
  int heartbeat = 0;
  int drop = 0;
  eibaddr_t myaddr;
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  pth_event_t input = pth_event (PTH_EVENT_SEM, &insignal);
  pth_event_t timeout = pth_event (PTH_EVENT_RTIME, pth_time (0, 0));
  pth_event_t timeout1 = pth_event (PTH_EVENT_RTIME, pth_time (10, 0));
  L_Data_PDU *c;

  EIBNetIPPacket p;
  EIBNetIPPacket *p1;
  EIBnet_ConnectRequest creq;
  creq.nat = saddr.sin_addr.s_addr == 0;
  EIBnet_ConnectResponse cresp;
  EIBnet_ConnectionStateRequest csreq;
  csreq.nat = saddr.sin_addr.s_addr == 0;
  EIBnet_ConnectionStateResponse csresp;
  EIBnet_TunnelRequest treq;
  EIBnet_TunnelACK tresp;
  EIBnet_DisconnectRequest dreq;
  dreq.nat = saddr.sin_addr.s_addr == 0;
  EIBnet_DisconnectResponse dresp;
  creq.caddr = saddr;
  creq.daddr = saddr;
  creq.CRI.resize (3);
  creq.CRI[0] = 0x04;
  creq.CRI[1] = 0x02;
  creq.CRI[2] = 0x00;
  p = creq.ToPacket ();
  sock->sendaddr = caddr;
  sock->Send (p);

  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      if (mod == 1)
	pth_event_concat (stop, input, NULL);
      if (mod == 2 || mod == 3)
	pth_event_concat (stop, timeout, NULL);

      pth_event_concat (stop, timeout1, NULL);

      p1 = sock->Get (stop);
      pth_event_isolate (stop);
      pth_event_isolate (timeout);
      pth_event_isolate (timeout1);
      if (p1)
	{
	  switch (p1->service)
	    {
	    case CONNECTION_RESPONSE:
	      if (mod)
		goto err;
	      if (parseEIBnet_ConnectResponse (*p1, cresp))
		{
		  TRACEPRINTF (t, 1, this, "Recv wrong connection response");
		  break;
		}
	      if (cresp.status != 0)
		{
		  TRACEPRINTF (t, 1, this, "Connect failed with error %02X",
			       cresp.status);
		  if (cresp.status == 0x23 && support_busmonitor == 1
		      && connect_busmonitor == 1)
		    {
		      TRACEPRINTF (t, 1, this, "Disable busmonitor support");
		      support_busmonitor = 0;
		      connect_busmonitor = 0;
		      creq.CRI[1] = 0x02;
		      pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout1,
				 pth_time (10, 0));
		      p = creq.ToPacket ();
		      TRACEPRINTF (t, 1, this, "Connectretry");
		      sock->sendaddr = caddr;
		      sock->Send (p);
		    }
		  break;
		}
	      if (cresp.CRD () != 3)
		{
		  TRACEPRINTF (t, 1, this, "Recv wrong connection response");
		  break;
		}
	      myaddr = (cresp.CRD[1] << 8) | cresp.CRD[2];
	      daddr = cresp.daddr;
	      if (!cresp.nat)
		{
		  if (NAT)
		    daddr.sin_addr = caddr.sin_addr;
		  if (dataport != -1)
		    daddr.sin_port = htons (dataport);
		}
	      channel = cresp.channel;
	      mod = 1;
	      sno = 0;
	      rno = 0;
	      sock->recvaddr2 = daddr;
	      sock->recvall = 3;
	      pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout1,
			 pth_time (30, 0));
	      heartbeat = 0;
	      break;

	    case TUNNEL_REQUEST:
	      if (mod == 0)
		{
		  TRACEPRINTF (t, 1, this, "Not connected");
		  goto err;
		}
	      if (parseEIBnet_TunnelRequest (*p1, treq))
		{
		  TRACEPRINTF (t, 1, this, "Invalid request");
		  break;
		}
	      if (treq.channel != channel)
		{
		  TRACEPRINTF (t, 1, this, "Not for us");
		  break;
		}
	      if (((treq.seqno + 1) & 0xff) == rno)
		{
		  tresp.status = 0;
		  tresp.channel = channel;
		  tresp.seqno = treq.seqno;
		  p = tresp.ToPacket ();
		  sock->sendaddr = daddr;
		  sock->Send (p);
		  sock->recvall = 0;
		  break;
		}
	      if (treq.seqno != rno)
		{
		  TRACEPRINTF (t, 1, this, "Wrong sequence %d<->%d",
			       treq.seqno, rno);
		  if (treq.seqno < rno)
		    treq.seqno += 0x100;
		  if (treq.seqno >= rno + 5)
		    {
		      dreq.caddr = saddr;
		      dreq.channel = channel;
		      p = dreq.ToPacket ();
		      sock->sendaddr = caddr;
		      sock->Send (p);
		      sock->recvall = 0;
		      mod = 0;
		    }
		  break;
		}
	      rno++;
	      if (rno > 0xff)
		rno = 0;
	      tresp.status = 0;
	      tresp.channel = channel;
	      tresp.seqno = treq.seqno;
	      p = tresp.ToPacket ();
	      sock->sendaddr = daddr;
	      sock->Send (p);
	      //Confirmation
	      if (treq.CEMI[0] == 0x2E)
		{
		  if (mod == 3)
		    mod = 1;
		  break;
		}
	      if (treq.CEMI[0] == 0x2B)
		{
		  L_Busmonitor_PDU *l2 = CEMI_to_Busmonitor (treq.CEMI);
		  outqueue.put (l2);
		  pth_sem_inc (&outsignal, 1);
		  break;
		}
	      if (treq.CEMI[0] != 0x29)
		{
		  TRACEPRINTF (t, 1, this, "Unexpected CEMI Type %02X",
			       treq.CEMI[0]);
		  break;
		}
	      c = CEMI_to_L_Data (treq.CEMI);
	      if (c)
		{

		  TRACEPRINTF (t, 1, this, "Recv %s", c->Decode ()());
		  if (mode == 0)
		    {
		      if (vmode)
			{
			  L_Busmonitor_PDU *l2 = new L_Busmonitor_PDU;
			  l2->pdu.set (c->ToPacket ());
			  outqueue.put (l2);
			  pth_sem_inc (&outsignal, 1);
			}
		      if (c->AddrType == IndividualAddress
			  && c->dest == myaddr)
			c->dest = 0;
		      outqueue.put (c);
		      pth_sem_inc (&outsignal, 1);
		      break;
		    }
		  L_Busmonitor_PDU *p1 = new L_Busmonitor_PDU;
		  p1->pdu = c->ToPacket ();
		  delete c;
		  outqueue.put (p1);
		  pth_sem_inc (&outsignal, 1);
		  break;
		}
	      TRACEPRINTF (t, 1, this, "Unknown CEMI");
	      break;
	    case TUNNEL_RESPONSE:
	      if (mod == 0)
		{
		  TRACEPRINTF (t, 1, this, "Not connected");
		  goto err;
		}
	      if (parseEIBnet_TunnelACK (*p1, tresp))
		{
		  TRACEPRINTF (t, 1, this, "Invalid response");
		  break;
		}
	      if (tresp.channel != channel)
		{
		  TRACEPRINTF (t, 1, this, "Not for us");
		  break;
		}
	      if (tresp.seqno != sno)
		{
		  TRACEPRINTF (t, 1, this, "Wrong sequence %d<->%d",
			       tresp.seqno, sno);
		  break;
		}
	      if (tresp.status)
		{
		  TRACEPRINTF (t, 1, this, "Error in ACK %d", tresp.status);
		  break;
		}
	      if (mod == 2)
		{
		  sno++;
		  if (sno > 0xff)
		    sno = 0;
		  pth_sem_dec (&insignal);
		  inqueue.get ();
		  if (noqueue)
		    {
		      mod = 3;
		      pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout,
				 pth_time (1, 0));
		    }
		  else
		    mod = 1;
		  retry = 0;
		  drop = 0;
		}
	      else
		TRACEPRINTF (t, 1, this, "Unexpected ACK");
	      break;
	    case CONNECTIONSTATE_RESPONSE:
	      if (parseEIBnet_ConnectionStateResponse (*p1, csresp))
		{
		  TRACEPRINTF (t, 1, this, "Invalid response");
		  break;
		}
	      if (csresp.channel != channel)
		{
		  TRACEPRINTF (t, 1, this, "Not for us");
		  break;
		}
	      if (csresp.status == 0)
		{
		  if (heartbeat > 0)
		    heartbeat--;
		  else
		    TRACEPRINTF (t, 1, this,
				 "Duplicate Connection State Response");
		}
	      else if (csresp.status == 0x21)
		{
		  TRACEPRINTF (t, 1, this,
			       "Connection State Response not connected");
		  dreq.caddr = saddr;
		  dreq.channel = channel;
		  p = dreq.ToPacket ();
		  sock->sendaddr = caddr;
		  sock->Send (p);
		  sock->recvall = 0;
		  mod = 0;
		}
	      else
		TRACEPRINTF (t, 1, this,
			     "Connection State Response Error %02x",
			     csresp.status);
	      break;
	    case DISCONNECT_REQUEST:
	      if (mod == 0)
		{
		  TRACEPRINTF (t, 1, this, "Not connected");
		  goto err;
		}
	      if (parseEIBnet_DisconnectRequest (*p1, dreq))
		{
		  TRACEPRINTF (t, 1, this, "Invalid request");
		  break;
		}
	      if (dreq.channel != channel)
		{
		  TRACEPRINTF (t, 1, this, "Not for us");
		  break;
		}
	      dresp.channel = channel;
	      dresp.status = 0;
	      p = dresp.ToPacket ();
	      t->TracePacket (1, this, "SendDis", p.data);
	      sock->sendaddr = caddr;
	      sock->Send (p);
	      sock->recvall = 0;
	      mod = 0;
	      break;
	    case DISCONNECT_RESPONSE:
	      if (mod == 0)
		{
		  TRACEPRINTF (t, 1, this, "Not connected");
		  break;
		}
	      if (parseEIBnet_DisconnectResponse (*p1, dresp))
		{
		  TRACEPRINTF (t, 1, this, "Invalid request");
		  break;
		}
	      if (dresp.channel != channel)
		{
		  TRACEPRINTF (t, 1, this, "Not for us");
		  break;
		}
	      mod = 0;
	      sock->recvall = 0;
	      TRACEPRINTF (t, 1, this, "Disconnected");
	      pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout1,
			 pth_time (0, 100));
	      break;
	    default:
	    err:
	      TRACEPRINTF (t, 1, this, "Recv unexpected service %04X",
			   p1->service);
	    }
	  delete p1;
	}
      if (mod == 2 && pth_event_status (timeout) == PTH_STATUS_OCCURRED)
	{
	  mod = 1;
	  retry++;
	  if (retry > 3)
	    {
	      TRACEPRINTF (t, 1, this, "Drop");
	      pth_sem_dec (&insignal);
	      inqueue.get ();
	      retry = 0;
	      drop++;
	      if (drop >= 3)
		{
		  dreq.caddr = saddr;
		  dreq.channel = channel;
		  p = dreq.ToPacket ();
		  sock->sendaddr = caddr;
		  sock->Send (p);
		  sock->recvall = 0;
		  mod = 0;
		}
	    }
	}
      if (mod == 3 && pth_event_status (timeout) == PTH_STATUS_OCCURRED)
	mod = 1;
      if (mod != 0 && pth_event_status (timeout1) == PTH_STATUS_OCCURRED)
	{
	  pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout1,
		     pth_time (30, 0));
	  if (heartbeat < 5)
	    {
	      csreq.caddr = saddr;
	      csreq.channel = channel;
	      p = csreq.ToPacket ();
	      TRACEPRINTF (t, 1, this, "Heartbeat");
	      sock->sendaddr = caddr;
	      sock->Send (p);
	      heartbeat++;
	    }
	  else
	    {
	      TRACEPRINTF (t, 1, this, "Disconnection because of errors");
	      dreq.caddr = saddr;
	      dreq.channel = channel;
	      p = dreq.ToPacket ();
	      sock->sendaddr = caddr;
	      if (channel != -1)
		sock->Send (p);
	      sock->recvall = 0;
	      mod = 0;
	    }
	}
      if (mod == 0 && pth_event_status (timeout1) == PTH_STATUS_OCCURRED)
	{
	  pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout1,
		     pth_time (10, 0));
	  creq.CRI[1] =
	    ((connect_busmonitor && support_busmonitor) ? 0x80 : 0x02);
	  p = creq.ToPacket ();
	  TRACEPRINTF (t, 1, this, "Connectretry");
	  sock->sendaddr = caddr;
	  sock->Send (p);
	}

      if (!inqueue.isempty () && inqueue.top ()() == 0)
	{
	  pth_sem_dec (&insignal);
	  inqueue.get ();
	  if (support_busmonitor)
	    {
	      dreq.caddr = saddr;
	      dreq.channel = channel;
	      p = dreq.ToPacket ();
	      sock->sendaddr = caddr;
	      sock->Send (p);
	    }
	}

      if (!inqueue.isempty () && mod == 1)
	{
	  treq.channel = channel;
	  treq.seqno = sno;
	  treq.CEMI = inqueue.top ();
	  p = treq.ToPacket ();
	  t->TracePacket (1, this, "SendTunnel", p.data);
	  sock->sendaddr = daddr;
	  sock->Send (p);
	  mod = 2;
	  pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout,
		     pth_time (1, 0));
	}
    }
out:
  dreq.caddr = saddr;
  dreq.channel = channel;
  p = dreq.ToPacket ();
  sock->sendaddr = caddr;
  if (channel != -1)
    sock->Send (p);

  pth_event_free (stop, PTH_FREE_THIS);
  pth_event_free (input, PTH_FREE_THIS);
  pth_event_free (timeout, PTH_FREE_THIS);
  pth_event_free (timeout1, PTH_FREE_THIS);
}
