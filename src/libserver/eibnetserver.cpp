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

#include "eibnetserver.h"
#include "emi.h"
#include "config.h"
#include <stdlib.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>

EIBnetServer::EIBnetServer (const char *multicastaddr, int port, bool Tunnel,
                bool Route, bool Discover, Layer3 * layer3,
                Trace * tr, String serverName)
	: Layer2mixin::Layer2mixin (layer3, tr)
{
  struct sockaddr_in baddr;
  struct ip_mreq mcfg;
  name = serverName;

  TRACEPRINTF (t, 8, this, "Open");
  memset (&baddr, 0, sizeof (baddr));
#ifdef HAVE_SOCKADDR_IN_LEN
  baddr.sin_len = sizeof (baddr);
#endif
  baddr.sin_family = AF_INET;
  baddr.sin_port = htons (port);
  baddr.sin_addr.s_addr = htonl (INADDR_ANY);

  if (GetHostIP (&maddr, multicastaddr) == 0)
    {
      ERRORPRINTF (t, E_ERROR | 11, this, "Addr '%s' not resolvable", multicastaddr);
      sock = 0;
      return;
    }
  maddr.sin_port = htons (port);

  sock = new EIBNetIPSocket (baddr, 1, t);
  if (!sock->init ())
    {
      delete sock;
      sock = 0;
      return;
    }
  mcfg.imr_multiaddr = maddr.sin_addr;
  mcfg.imr_interface.s_addr = htonl (INADDR_ANY);
  if (!sock->SetMulticast (mcfg))
    {
      delete sock;
      sock = 0;
      return;
    }
  sock->recvall = 2;
  if (!GetSourceAddress (&maddr, &sock->localaddr))
    {
      delete sock;
      sock = 0;
      return;
    }
  sock->localaddr.sin_port = htons (port);
  tunnel = Tunnel;
  route = Route;
  discover = Discover;
  Port = htons (port);
  if (route || tunnel)
    layer2_is_bus ();
  busmoncount = 0;
  Start ();
  TRACEPRINTF (t, 8, this, "Opened");
}


EIBnetServer::~EIBnetServer ()
{
  unsigned int i;
  TRACEPRINTF (t, 8, this, "Close");
  if (busmoncount)
    l3->deregisterVBusmonitor (this);
  Stop ();
  for (i = 0; i < natstate (); i++)
    pth_event_free (natstate[i].timeout, PTH_FREE_THIS);
  if (sock)
    delete sock;
}

bool
EIBnetServer::init ()
{
  if (!sock)
    return false;
  return Layer2mixin::init();
}

void
EIBnetServer::Send_L_Busmonitor (L_Busmonitor_PDU * l)
{
  for (unsigned int i = 0; i < state (); i++)
    {
      if (state[i]->type == CT_BUSMONITOR)
	{
	  state[i]->out.put (Busmonitor_to_CEMI (0x2B, *l, state[i]->no++));
	  pth_sem_inc (state[i]->outsignal, 0);
	}
    }
}


void
EIBnetServer::Send_L_Data (L_Data_PDU * l)
{
  if (l->object == this)
    {
      delete l;
      return;
    }
  if (route)
    {
      TRACEPRINTF (t, 8, this, "Send_Route %s", l->Decode ()());
      sock->sendaddr = maddr;
      EIBNetIPPacket p;
      p.service = ROUTING_INDICATION;
      if (l->dest == 0 && l->AddrType == IndividualAddress)
	{
	  unsigned int i, cnt = 0;
	  for (i = 0; i < natstate (); i++)
	    if (natstate[i].dest == l->source)
	      {
		l->dest = natstate[i].src;
		p.data = L_Data_ToCEMI (0x29, *l);
		sock->Send (p);
		l->dest = 0;
		cnt++;
	      }
	  if (!cnt)
	    {
	      p.data = L_Data_ToCEMI (0x29, *l);
	      sock->Send (p);
	    }
	}
      else
	{
	  p.data = L_Data_ToCEMI (0x29, *l);
	  sock->Send (p);
	}
    }
  delete l;
}

void ConnState::Send_L_Data (L_Data_PDU * l)
{
  if (type == CT_STANDARD)
    {
      out.put (L_Data_ToCEMI (0x29, *l));
      pth_sem_inc (outsignal, 0);
    }
  delete l;
}

void
EIBnetServer::addBusmonitor ()
{
  if (busmoncount == 0)
    {
      if (!l3->registerVBusmonitor (this))
	TRACEPRINTF (t, 8, this, "Registervbusmonitor failed");
      busmoncount++;
    }
}

void
EIBnetServer::delBusmonitor ()
{
  busmoncount--;
  if (busmoncount == 0)
    l3->deregisterVBusmonitor (this);
}

int
EIBnetServer::addClient (ConnType type, const EIBnet_ConnectRequest & r1,
                         eibaddr_t addr)
{
  unsigned int i;
  int id = 1;
rt:
  for (i = 0; i < state (); i++)
    if (state[i]->channel == id)
      {
	id++;
	goto rt;
      }
  if (id <= 0xff)
    {
      ConnState *s = new ConnState (this, addr);
      s->channel = id;
      s->daddr = r1.daddr;
      s->caddr = r1.caddr;
      s->state = 0;
      s->sno = 0;
      s->rno = 0;
      s->no = 1;
      s->type = type;
      s->nat = r1.nat;

      int pos = state ();
      state.resize (state () + 1);
      state[pos] = s;

      s->Start();
    }
  return id;
}

void
EIBnetServer::addNAT (const L_Data_PDU & l)
{
  unsigned int i;
  if (l.AddrType != IndividualAddress)
    return;
  for (i = 0; i < natstate (); i++)
    if (natstate[i].src == l.source && natstate[i].dest == l.dest)
      {
	pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE,
		   natstate[i].timeout, pth_time (180, 0));
	return;
      }
  i = natstate ();
  natstate.resize (i + 1);
  natstate[i].src = l.source;
  natstate[i].dest = l.dest;
  natstate[i].timeout = pth_event (PTH_EVENT_RTIME, pth_time (180, 0));
}

ConnState::ConnState (EIBnetServer *p, eibaddr_t addr) : Layer2mixin (p->l3, p->t)
{
  parent = p;
  timeout = pth_event (PTH_EVENT_RTIME, pth_time (120, 0));
  outsignal = new pth_sem_t;
  pth_sem_init (outsignal);
  outwait = pth_event (PTH_EVENT_SEM, outsignal);
  sendtimeout = pth_event (PTH_EVENT_RTIME, pth_time (1, 0));
  if (addr)
    addAddress(addr);
}

void
ConnState::Run (pth_sem_t * stop1)
{
  EIBNetIPPacket *p1;
  EIBNetIPPacket p;
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);

  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      pth_event_concat (stop, timeout, NULL);
      if (state)
	pth_event_concat (stop, sendtimeout, NULL);
      else
	pth_event_concat (stop, outwait, NULL);
      pth_wait(stop);
      pth_event_isolate (timeout);
      pth_event_isolate (sendtimeout);
      pth_event_isolate (outwait);

      if (pth_event_status (timeout) == PTH_STATUS_OCCURRED)
	{
	  shutdown();
	  return;
	}
      if (state ? pth_event_status (sendtimeout) == PTH_STATUS_OCCURRED
                : !out.isempty ())
	{
	  TRACEPRINTF (t, 8, this, "TunnelSend %d", channel);
	  state++;
	  if (state > 10)
	    {
	      out.get ();
	      pth_sem_dec (outsignal);
	      state = 0;
	    }

	  EIBNetIPPacket p;
	  if (type == CT_CONFIG)
	    {
	      EIBnet_ConfigRequest r;
	      r.channel = channel;
	      r.seqno = sno;
	      r.CEMI = out.top ();
	      p = r.ToPacket ();
	    }
	  else
	    {
	      EIBnet_TunnelRequest r;
	      r.channel = channel;
	      r.seqno = sno;
	      r.CEMI = out.top ();
	      p = r.ToPacket ();
	    }
	  pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE,
		     sendtimeout, pth_time (1, 0));
	  parent->Send (p, daddr);
	}
    }

  EIBnet_DisconnectRequest r;
  r.channel = channel;
  if (GetSourceAddress (&caddr, &r.caddr))
    {
      r.caddr.sin_port = parent->Port;
      r.nat = nat;
      parent->Send (r.ToPacket (), caddr);
    }
  shutdown();
}

void ConnState::shutdown(void)
{
  parent->drop_state (this);
}

void EIBnetServer::drop_state (ConnState *s)
{
  for (int i=0; i < state (); i++)
    {
      if (state[i] == s)
        {
	  drop_state(i);
	  break;
	}
    }
}

void EIBnetServer::drop_state (uint8_t index)
{
  delete state[index];
  state.deletepart (index);
}

ConnState::~ConnState()
{
  pth_event_free (timeout, PTH_FREE_THIS);
  pth_event_free (sendtimeout, PTH_FREE_THIS);
  pth_event_free (outwait, PTH_FREE_THIS);
  delete outsignal;
  if (type == CT_BUSMONITOR)
    parent->delBusmonitor ();
}

void
EIBnetServer::Run (pth_sem_t * stop1)
{
  EIBNetIPPacket *p1;
  EIBNetIPPacket p;
  unsigned int i;
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);

  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      for (i = 0; i < natstate (); i++)
	pth_event_concat (stop, natstate[i].timeout, NULL);
      p1 = sock->Get (stop);
      for (i = 0; i < natstate (); i++)
	pth_event_isolate (natstate[i].timeout);
      for (i = 0; i < natstate (); i++)
	if (pth_event_status (natstate[i].timeout) == PTH_STATUS_OCCURRED)
	  {
	    pth_event_free (natstate[i].timeout, PTH_FREE_THIS);
	    natstate.deletepart (i, 1);
	  }
      if (p1)
	{
          /* Get MAC Address */

          struct ifreq ifr;
          struct ifconf ifc;
          char buf[1024];
          unsigned char mac_address[IFHWADDRLEN]= {0,0,0,0,0,0};

          int sock_mac = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
          if (sock_mac != -1)
	    {
              ifc.ifc_len = sizeof(buf);
              ifc.ifc_buf = buf;
              if (ioctl(sock_mac, SIOCGIFCONF, &ifc) != -1)
		{
                  struct ifreq* it = ifc.ifc_req;
                  const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

                  for (; it != end; ++it)
                    {
                      strcpy(ifr.ifr_name, it->ifr_name);
                      if (ioctl(sock_mac, SIOCGIFFLAGS, &ifr))
			continue;
		      if (ifr.ifr_flags & IFF_LOOPBACK) // don't count loopback
			continue;
		      if (ioctl(sock_mac, SIOCGIFHWADDR, &ifr))
			continue;
		      if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER)
			continue;
		      memcpy(mac_address, ifr.ifr_hwaddr.sa_data, sizeof(mac_address));
		      break;
		    }
                }
              close(sock_mac);
	    }
          /* End MAC Address */

	  if (p1->service == SEARCH_REQUEST && discover)
	    {
	      EIBnet_SearchRequest r1;
	      EIBnet_SearchResponse r2;
	      DIB_service_Entry d;
	      if (parseEIBnet_SearchRequest (*p1, r1))
		goto out;
	      TRACEPRINTF (t, 8, this, "SEARCH");
	      r2.KNXmedium = 2;
	      r2.devicestatus = 0;
	      r2.individual_addr = l3->defaultAddr;
	      r2.installid = 0;
	      r2.multicastaddr = maddr.sin_addr;
	      r2.serial[0]=1;
	      r2.serial[1]=2;
	      r2.serial[2]=3;
	      r2.serial[3]=4;
	      r2.serial[4]=5;
	      r2.serial[5]=6;
	      //FIXME: Hostname, MAC-addr
              memcpy(r2.MAC, mac_address, sizeof(r2.MAC));
	      //FIXME: Hostname, indiv. address
              strncpy ((char *) r2.name, name (), sizeof(r2.name));
	      d.version = 1;
	      d.family = 2; // core
	      r2.services.add (d);
	      //d.family = 3; // device management
	      //r2.services.add (d);
	      d.family = 4;
	      if (tunnel)
		r2.services.add (d);
	      d.family = 5;
	      if (route)
		r2.services.add (d);
	      if (!GetSourceAddress (&r1.caddr, &r2.caddr))
		goto out;
	      r2.caddr.sin_port = Port;
	      sock->Send (r2.ToPacket (), r1.caddr);
	    }
	  if (p1->service == DESCRIPTION_REQUEST && discover)
	    {
	      EIBnet_DescriptionRequest r1;
	      EIBnet_DescriptionResponse r2;
	      DIB_service_Entry d;
	      if (parseEIBnet_DescriptionRequest (*p1, r1))
		goto out;
	      TRACEPRINTF (t, 8, this, "DESCRIBE");
	      r2.KNXmedium = 2;
	      r2.devicestatus = 0;
	      r2.individual_addr = l3->defaultAddr;
	      r2.installid = 0;
	      r2.multicastaddr = maddr.sin_addr;
              memcpy(r2.MAC, mac_address, sizeof(r2.MAC));
	      //FIXME: Hostname, indiv. address
              strncpy ((char *) r2.name, name(), sizeof(r2.name));
	      d.version = 1;
	      d.family = 2;
	      if (discover)
		r2.services.add (d);
	      d.family = 3;
	      r2.services.add (d);
	      d.family = 4;
	      if (tunnel)
		r2.services.add (d);
	      d.family = 5;
	      if (route)
		r2.services.add (d);
	      sock->Send (r2.ToPacket (), r1.caddr);
	    }
	  if (p1->service == ROUTING_INDICATION && route)
	    {
	      if (p1->data () < 2 || p1->data[0] != 0x29)
		goto out;
	      const CArray data = p1->data;
	      L_Data_PDU *c = CEMI_to_L_Data (data, this);
	      if (c)
		{
		  TRACEPRINTF (t, 8, this, "Recv_Route %s", c->Decode ()());
		  if (c->hopcount)
		    {
		      c->hopcount--;
		      addNAT (*c);
		      c->object = this;
		      l3->recv_L_Data (c);
		    }
		  else
		    {
		      TRACEPRINTF (t, 8, this, "RecvDrop");
		      delete c;
		    }
		}
	    }
	  if (p1->service == CONNECTIONSTATE_REQUEST)
	    {
	      uchar res = 21;
	      EIBnet_ConnectionStateRequest r1;
	      EIBnet_ConnectionStateResponse r2;
	      if (parseEIBnet_ConnectionStateRequest (*p1, r1))
		goto out;
	      for (i = 0; i < state (); i++)
		if (state[i]->channel == r1.channel)
		  {
		    if (compareIPAddress (p1->src, state[i]->caddr))
		      {
			res = 0;
			pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE,
				   state[i]->timeout, pth_time (120, 0));
		      }
		    else
		      TRACEPRINTF (t, 8, this, "Invalid control address");
		  }
	      r2.channel = r1.channel;
	      r2.status = res;
	      sock->Send (r2.ToPacket (), r1.caddr);
	    }
	  if (p1->service == DISCONNECT_REQUEST)
	    {
	      uchar res = 0x21;
	      EIBnet_DisconnectRequest r1;
	      EIBnet_DisconnectResponse r2;
	      if (parseEIBnet_DisconnectRequest (*p1, r1))
		goto out;
	      for (i = 0; i < state (); i++)
		if (state[i]->channel == r1.channel)
		  {
		    if (compareIPAddress (p1->src, state[i]->caddr))
		      {
			drop_state(i);
			break;
		      }
		    else
		      TRACEPRINTF (t, 8, this, "Invalid control address");
		  }
	      r2.channel = r1.channel;
	      r2.status = res;
	      sock->Send (r2.ToPacket (), r1.caddr);
	    }
	  if (p1->service == CONNECTION_REQUEST)
	    {
	      EIBnet_ConnectRequest r1;
	      EIBnet_ConnectResponse r2;
	      if (parseEIBnet_ConnectRequest (*p1, r1))
		goto out;
	      r2.status = 0x22;
	      if (r1.CRI () == 3 && r1.CRI[0] == 4 && tunnel)
		{
		  eibaddr_t a = l3->get_client_addr ();
		  r2.CRD.resize (3);
		  r2.CRD[0] = 0x04;
		  TRACEPRINTF (t, 8, this, "Tunnel CONNECTION_REQ with %s", FormatEIBAddr(a)());
		  r2.CRD[1] = (a >> 8) & 0xFF;
		  r2.CRD[2] = (a >> 0) & 0xFF;
		  if (r1.CRI[1] == 0x02 || r1.CRI[1] == 0x80)
		    {
		      int id = addClient ((r1.CRI[1] == 0x80) ? CT_BUSMONITOR : CT_STANDARD, r1, a);
		      if (id <= 0xff)
			{
			  if (r1.CRI[1] == 0x80)
			    addBusmonitor ();
			  r2.channel = id;
			  r2.status = 0;
			}
		    }
		}
	      else if (r1.CRI () == 1 && r1.CRI[0] == 3)
		{
		  r2.CRD.resize (1);
		  r2.CRD[0] = 0x03;
		  int id = addClient (CT_CONFIG, r1, 0);
		  if (id <= 0xff)
		    {
		      r2.channel = id;
		      r2.status = 0;
		    }
		}
	      if (!GetSourceAddress (&r1.caddr, &r2.daddr))
		goto out;
	      r2.daddr.sin_port = Port;
	      r2.nat = r1.nat;
	      sock->Send (r2.ToPacket (), r1.caddr);
	    }
	  if (p1->service == TUNNEL_REQUEST && tunnel)
	    {
	      EIBnet_TunnelRequest r1;
	      EIBnet_TunnelACK r2;
	      if (parseEIBnet_TunnelRequest (*p1, r1))
		goto out;
	      TRACEPRINTF (t, 8, this, "TUNNEL_REQ");
	      for (i = 0; i < state (); i++)
		if (state[i]->channel == r1.channel)
		  {
		    if (!compareIPAddress (p1->src, state[i]->daddr))
		      {
			TRACEPRINTF (t, 8, this, "Invalid data endpoint");
			break;
		      }
		    state[i]->tunnel_request(r1);
		    break;
		  }
	      goto out;
	    }
	  if (p1->service == TUNNEL_RESPONSE && tunnel)
	    {
	      EIBnet_TunnelACK r1;
	      if (parseEIBnet_TunnelACK (*p1, r1))
		goto out;
	      TRACEPRINTF (t, 8, this, "TUNNEL_ACK");
	      for (i = 0; i < state (); i++)
		if (state[i]->channel == r1.channel)
		  {
		    if (!compareIPAddress (p1->src, state[i]->daddr))
		      {
			TRACEPRINTF (t, 8, this, "Invalid data endpoint");
			break;
		      }
		    state[i]->tunnel_response (r1);
		    break;
		  }
	      goto out;
	    }
	  if (p1->service == DEVICE_CONFIGURATION_REQUEST)
	    {
	      EIBnet_ConfigRequest r1;
	      EIBnet_ConfigACK r2;
	      if (parseEIBnet_ConfigRequest (*p1, r1))
		goto out;
	      TRACEPRINTF (t, 8, this, "CONFIG_REQ");
	      for (i = 0; i < state (); i++)
		if (state[i]->channel == r1.channel)
		  {
		    if (!compareIPAddress (p1->src, state[i]->daddr))
		      {
			TRACEPRINTF (t, 8, this, "Invalid data endpoint");
			  break;
		      }
		    state[i]->config_request (r1);
		    break;
		  }
	      goto out;
	    }
	  if (p1->service == DEVICE_CONFIGURATION_ACK)
	    {
	      EIBnet_ConfigACK r1;
	      if (parseEIBnet_ConfigACK (*p1, r1))
		goto out;
	      TRACEPRINTF (t, 8, this, "CONFIG_ACK");
	      for (i = 0; i < state (); i++)
		if (state[i]->channel == r1.channel)
		  {
		    if (!compareIPAddress (p1->src, state[i]->daddr))
		      {
			TRACEPRINTF (t, 8, this, "Invalid data endpoint");
			break;
		      }
		    state[i]->config_response (r1);
		    break;
		  }
	      goto out;
	    }
	  TRACEPRINTF (t, 8, this, "Unexpected service type: %d", p1->service);
	out:
	  delete p1;
	}
    }
  for (i = 0; i < state (); i++)
    delete state[i];
  pth_event_free (stop, PTH_FREE_THIS);
}

void ConnState::tunnel_request(EIBnet_TunnelRequest &r1)
{
  EIBnet_TunnelACK r2;
  r2.channel = r1.channel;
  r2.seqno = r1.seqno;

  if (rno == ((r1.seqno + 1) & 0xff))
    {
      TRACEPRINTF (t, 8, this, "Lost ACK for %d", rno);
      parent->Send (r2.ToPacket (), daddr);
      return;
    }
  if (rno != r1.seqno)
    {
      TRACEPRINTF (t, 8, this, "Wrong sequence %d<->%d",
		   r1.seqno, rno);
      return;
    }
  if (type == CT_STANDARD)
    {
      L_Data_PDU *c = CEMI_to_L_Data (r1.CEMI, this);
      if (c)
	{
	  r2.status = 0;
	  if (c->hopcount)
	    {
	      c->hopcount--;
	      if (r1.CEMI[0] == 0x11)
		{
		  out.put (L_Data_ToCEMI (0x2E, *c));
		  pth_sem_inc (outsignal, 0);
		}
	      c->object = this;
	      if (r1.CEMI[0] == 0x11 || r1.CEMI[0] == 0x29)
		l3->recv_L_Data (c);
	      else
		delete c;
	    }
	  else
	    {
	      TRACEPRINTF (t, 8, this, "RecvDrop");
	      delete c;
	    }
	}
      else
	r2.status = 0x29;
    }
  else
    {
      TRACEPRINTF (t, 8, this, "Type not CT_STANDARD (%d)", type);
      r2.status = 0x29;
    }
  rno++;
  parent->Send (r2.ToPacket (), daddr);
}

void ConnState::tunnel_response (EIBnet_TunnelACK &r1)
{
  if (sno != r1.seqno)
    {
      TRACEPRINTF (t, 8, this, "Wrong sequence %d<->%d",
		   r1.seqno, sno);
      return;
    }
  if (r1.status != 0)
    {
      TRACEPRINTF (t, 8, this, "Wrong status %d", r1.status);
      return;
    }
  if (!state)
    {
      TRACEPRINTF (t, 8, this, "Unexpected ACK");
      return;
    }
  if (type != CT_STANDARD && type != CT_BUSMONITOR)
    {
      TRACEPRINTF (t, 8, this, "Unexpected Connection Type");
      return;
    }
  sno++;
  state = 0;
  out.get ();
  pth_sem_dec (outsignal);
}

void ConnState::config_request(EIBnet_ConfigRequest &r1)
{
  EIBnet_ConfigACK r2;
  if (rno == ((r1.seqno + 1) & 0xff))
    {
      r2.channel = r1.channel;
      r2.seqno = r1.seqno;
      parent->Send (r2.ToPacket (), daddr);
      return;
    }
  if (rno != r1.seqno)
    {
      TRACEPRINTF (t, 8, this, "Wrong sequence %d<->%d",
		   r1.seqno, rno);
      return;
    }
  r2.channel = r1.channel;
  r2.seqno = r1.seqno;
  if (type == CT_CONFIG && r1.CEMI () > 1)
    {
      if (r1.CEMI[0] == 0xFC)
	{
	  if (r1.CEMI () == 7)
	    {
	      CArray res, CEMI;
	      int obj = (r1.CEMI[1] << 8) | r1.CEMI[2];
	      int objno = r1.CEMI[3];
	      int prop = r1.CEMI[4];
	      int count = (r1.CEMI[5] >> 4) & 0x0f;
	      int start = (r1.CEMI[5] & 0x0f) | r1.CEMI[6];
	      res.resize (1);
	      res[0] = 0;
	      if (obj == 0 && objno == 0)
		{
		  if (prop == 0)
		    {
		      res.resize (2);
		      res[0] = 0;
		      res[1] = 0;
		      start = 0;
		    }
		  else
		    count = 0;
		}
	      else
		count = 0;
	      CEMI.resize (6 + res ());
	      CEMI[0] = 0xFB;
	      CEMI[1] = (obj >> 8) & 0xff;
	      CEMI[2] = obj & 0xff;
	      CEMI[3] = objno;
	      CEMI[4] = prop;
	      CEMI[5] = ((count & 0x0f) << 4) | (start >> 8);
	      CEMI[6] = start & 0xff;
	      CEMI.setpart (res, 7);
	      r2.status = 0x00;
	      out.put (CEMI);
	      pth_sem_inc (outsignal, 0);
	    }
	  else
	    r2.status = 0x26;
	}
      else
	r2.status = 0x26;
    }
  else
    r2.status = 0x29;
  rno++;
  parent->Send (r2.ToPacket (), daddr);
}

void ConnState::config_response (EIBnet_ConfigACK &r1)
{
  TRACEPRINTF (t, 8, this, "CONFIG_ACK");
  if (sno != r1.seqno)
    {
      TRACEPRINTF (t, 8, this, "Wrong sequence %d<->%d",
		   r1.seqno, sno);
      return;
    }
  if (r1.status != 0)
    {
      TRACEPRINTF (t, 8, this, "Wrong status %d", r1.status);
      return;
    }
  if (!state)
    {
      TRACEPRINTF (t, 8, this, "Unexpected ACK");
      return;
    }
  if (type != CT_CONFIG)
    {
      TRACEPRINTF (t, 8, this, "Unexpected Connection Type");
      return;
    }
  sno++;
  state = 0;
  out.get ();
  pth_sem_dec (outsignal);
}
