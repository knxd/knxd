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
#include <memory>

static void reset_time(pth_event_t ev, unsigned int sec, unsigned int usec)
{
  pth_event_t ev_prev = pth_event_isolate(ev); // NULL if no other member
  pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, ev, pth_time (sec, usec));
  pth_event_concat(ev, ev_prev, NULL);
}

EIBnetServer::EIBnetServer (Trace * tr, String serverName)
	: Layer2mixin::Layer2mixin (tr)
  , name(serverName)
  , mcast(NULL)
  , sock(NULL)
  , tunnel(false)
  , route(false)
  , discover(false)
  , Port(-1)
  , sock_mac(-1)
  , busmoncount(0)
{
}

EIBnetDiscover::EIBnetDiscover (EIBnetServer *parent, const char *multicastaddr, int port)
{
  struct sockaddr_in baddr;
  struct ip_mreq mcfg;
  sock = 0;

  this->parent = parent;
  TRACEPRINTF (parent->t, 8, this, "OpenD");

  if (GetHostIP (parent->t, &maddr, multicastaddr) == 0)
    {
      ERRORPRINTF (parent->t, E_ERROR | 11, this, "Addr '%s' not resolvable", multicastaddr);
      goto err_out;
    }
  maddr.sin_port = htons (port);

  if (port)
    {
      memset (&baddr, 0, sizeof (baddr));
#ifdef HAVE_SOCKADDR_IN_LEN
      baddr.sin_len = sizeof (baddr);
#endif
      baddr.sin_family = AF_INET;
      baddr.sin_addr.s_addr = htonl (INADDR_ANY);
      baddr.sin_port = htons (port);

      sock = new EIBNetIPSocket (baddr, 1, parent->t);
      if (!sock->init ())
        goto err_out;
    }
  else
    sock = parent->sock;

  mcfg.imr_multiaddr = maddr.sin_addr;
  mcfg.imr_interface.s_addr = htonl (INADDR_ANY);
  if (!sock->SetMulticast (mcfg))
    goto err_out;

  /** This causes us to ignore multicast packets sent by ourselves */
  if (!GetSourceAddress (&maddr, &sock->localaddr))
    goto err_out;
  sock->localaddr.sin_port = parent->Port;
  sock->recvall = 2;

  if (port)
    Start ();
  TRACEPRINTF (parent->t, 8, this, "OpenedD");
  return;

err_out:
  if (sock && port)
    delete (sock);
  sock = 0;
  return;
}

bool
EIBnetDiscover::init ()
{
  return sock != 0;
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
  if (mcast)
    delete mcast;
  if (sock_mac >= 0)
    close (sock_mac);
}

EIBnetDiscover::~EIBnetDiscover ()
{
  TRACEPRINTF (parent->t, 8, this, "CloseD");
  if (sock && parent->sock != sock)
    {
      Stop ();
      delete sock;
    }
}

// we can't just not define this function
bool
EIBnetServer::init (Layer3 *l3)
{
  ERRORPRINTF (t, E_ERROR | 43, this, "Code error: missing parameters");
  return false;
}

bool
EIBnetServer::init (Layer3 *l3,
                    const char *multicastaddr, const int port,
                    const bool tunnel, const bool route,
                    const bool discover, const bool single_port)
{
  struct sockaddr_in baddr;

  TRACEPRINTF (t, 8, this, "Open");
  sock_mac = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock_mac < 0)
  {
    ERRORPRINTF (t, E_ERROR | 11, this, "Lookup socket creation failed");
    goto err_out0;
  }
  memset (&baddr, 0, sizeof (baddr));
#ifdef HAVE_SOCKADDR_IN_LEN
  baddr.sin_len = sizeof (baddr);
#endif
  baddr.sin_family = AF_INET;
  baddr.sin_addr.s_addr = htonl (INADDR_ANY);
  baddr.sin_port = single_port ? htons(port) : 0;

  sock = new EIBNetIPSocket (baddr, 1, t);
  if (!sock)
  {
    ERRORPRINTF (t, E_ERROR | 41, this, "EIBNetIPSocket creation failed");
    goto err_out1;
  }
  if (!sock->init ())
    goto err_out2;
  sock->recvall = 1;
  Port = sock->port ();

  mcast = new EIBnetDiscover (this, multicastaddr, single_port ? 0 : port);
  if (!mcast)
  {
    ERRORPRINTF (t, E_ERROR | 42, this, "EIBnetDiscover creation failed");
    goto err_out2;
  }
  if (!mcast->init ())
    goto err_out3;

  this->tunnel = tunnel;
  this->route = route;
  this->discover = discover;
  this->single_port = single_port;
  if (this->route || this->tunnel)
    addGroupAddress(0);

  busmoncount = 0;
  Start ();
  TRACEPRINTF (t, 8, this, "Opened");

  if (Layer2mixin::init(l3))
    return true;

err_out3:
  delete mcast;
  mcast = NULL;
err_out2:
  delete sock;
  sock = NULL;
err_out1:
  close (sock_mac);
  sock_mac = -1;
err_out0:
  return false;
}

void EIBnetDiscover::Send (EIBNetIPPacket p, struct sockaddr_in addr)
{
  if (sock)
    sock->Send (p, addr);
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
		Send (p);
		l->dest = 0;
		cnt++;
	      }
	  if (!cnt)
	    {
	      p.data = L_Data_ToCEMI (0x29, *l);
	      Send (p);
	    }
	}
      else
	{
	  p.data = L_Data_ToCEMI (0x29, *l);
	  Send (p);
	}
    }
  delete l;
}

bool ConnState::init()
{
  if (! Layer2mixin::init(parent->l3))
    return false;
  if (! addGroupAddress(0))
    return false;
  return true;
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
      ConnStatePtr s = ConnStatePtr(new ConnState (this, addr));
      s->channel = id;
      s->daddr = r1.daddr;
      s->caddr = r1.caddr;
      s->state = 0;
      s->sno = 0;
      s->rno = 0;
      s->no = 1;
      s->type = type;
      s->nat = r1.nat;
      if(!s->init())
        return -1;

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
	reset_time(natstate[i].timeout, 180, 0);
	return;
      }
  i = natstate ();
  natstate.resize (i + 1);
  natstate[i].src = l.source;
  natstate[i].dest = l.dest;
  natstate[i].timeout = pth_event (PTH_EVENT_RTIME, pth_time (180, 0));
}

ConnState::ConnState (EIBnetServer *p, eibaddr_t addr)
  : Layer2mixin (new Trace(p->t,p->t->name+':'+FormatEIBAddr(addr)))
{
  parent = p;
  timeout = pth_event (PTH_EVENT_RTIME, pth_time (120, 0));
  outsignal = new pth_sem_t;
  pth_sem_init (outsignal);
  outwait = pth_event (PTH_EVENT_SEM, outsignal);
  sendtimeout = pth_event (PTH_EVENT_RTIME, pth_time (1, 0));
  if (!addr)
    remoteAddr = p->l3->get_client_addr ();
  else
    remoteAddr = addr;
  if (remoteAddr)
    addAddress(remoteAddr);
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
        break;

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
          else
            {
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
              reset_time(sendtimeout, 1, 0);
              parent->mcast->Send (p, daddr);
            }
        }
    }

  if (channel > 0)
    {
      EIBnet_DisconnectRequest r;
      r.channel = channel;
      if (GetSourceAddress (&caddr, &r.caddr))
        {
          r.caddr.sin_port = parent->Port;
          r.nat = nat;
          parent->Send (r.ToPacket (), caddr);
        }
    }
  parent->drop_state (std::static_pointer_cast<ConnState>(shared_from_this()));
}

void ConnState::shutdown(void)
{
  Stop();
}

void EIBnetServer::drop_state (ConnStatePtr s)
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

void
EIBnetServer::drop_state (uint8_t index)
{
  ConnStatePtr state2 = state[index];
  state.deletepart (index);
  state2->shutdown();
}

ConnState::~ConnState()
{
  TRACEPRINTF (parent->t, 8, this, "CloseS");
  Stop();
  pth_event_free (timeout, PTH_FREE_THIS);
  pth_event_free (sendtimeout, PTH_FREE_THIS);
  pth_event_free (outwait, PTH_FREE_THIS);
  delete outsignal;
  if (type == CT_BUSMONITOR)
    parent->delBusmonitor ();
}

void
EIBnetServer::handle_packet (EIBNetIPPacket *p1, EIBNetIPSocket *isock)
{
  /* Get MAC Address */
  /* TODO: cache all of this, and ask at most once per seoncd */

  struct ifreq ifr;
  struct ifconf ifc;
  char buf[1024];
  unsigned char mac_address[IFHWADDRLEN]= {0,0,0,0,0,0};

  if (sock_mac != -1 && discover &&
      (p1->service == DESCRIPTION_REQUEST || p1->service == SEARCH_REQUEST))
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
      r2.multicastaddr = mcast->maddr.sin_addr;
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
      isock->Send (r2.ToPacket (), r1.caddr);
      goto out;
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
      r2.multicastaddr = mcast->maddr.sin_addr;
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
      isock->Send (r2.ToPacket (), r1.caddr);
      goto out;
    }
  if (p1->service == ROUTING_INDICATION && route)
    {
      if (p1->data () < 2 || p1->data[0] != 0x29)
	goto out;
      const CArray data = p1->data;
      L_Data_PDU *c = CEMI_to_L_Data (data, shared_from_this());
      if (c)
	{
	  TRACEPRINTF (t, 8, this, "Recv_Route %s", c->Decode ()());
          addNAT (*c);
          c->object = this;
          l3->recv_L_Data (c);
	}
      goto out;
    }
  if (p1->service == CONNECTIONSTATE_REQUEST)
    {
      uchar res = 21;
      EIBnet_ConnectionStateRequest r1;
      EIBnet_ConnectionStateResponse r2;
      if (parseEIBnet_ConnectionStateRequest (*p1, r1))
	goto out;
      for (unsigned int i = 0; i < state (); i++)
	if (state[i]->channel == r1.channel)
	  {
            res = 0;
            reset_time(state[i]->timeout, 120,0);
	  }
      r2.channel = r1.channel;
      r2.status = res;
      isock->Send (r2.ToPacket (), r1.caddr);
      goto out;
    }
  if (p1->service == DISCONNECT_REQUEST)
    {
      uchar res = 0x21;
      EIBnet_DisconnectRequest r1;
      EIBnet_DisconnectResponse r2;
      if (parseEIBnet_DisconnectRequest (*p1, r1))
	goto out;
      for (unsigned int i = 0; i < state (); i++)
	if (state[i]->channel == r1.channel)
	  {
            res = 0;
            state[i]->channel = 0;
            drop_state(i);
            break;
	  }
      r2.channel = r1.channel;
      r2.status = res;
      isock->Send (r2.ToPacket (), r1.caddr);
      goto out;
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
      isock->Send (r2.ToPacket (), r1.caddr);
      goto out;
    }
  if (p1->service == TUNNEL_REQUEST && tunnel)
    {
      EIBnet_TunnelRequest r1;
      EIBnet_TunnelACK r2;
      if (parseEIBnet_TunnelRequest (*p1, r1))
	goto out;
      TRACEPRINTF (t, 8, this, "TUNNEL_REQ");
      for (unsigned int i = 0; i < state (); i++)
	if (state[i]->channel == r1.channel)
	  {
	    state[i]->tunnel_request(r1, isock);
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
      for (unsigned int i = 0; i < state (); i++)
	if (state[i]->channel == r1.channel)
	  {
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
      for (unsigned int i = 0; i < state (); i++)
	if (state[i]->channel == r1.channel)
	  {
	    state[i]->config_request (r1, isock);
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
      for (unsigned int i = 0; i < state (); i++)
	if (state[i]->channel == r1.channel)
	  {
	    state[i]->config_response (r1);
	    break;
	  }
      goto out;
    }
  TRACEPRINTF (t, 8, this, "Unexpected service type: %04x", p1->service);
out:
  delete p1;
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
	handle_packet (p1, this->sock);
    }

  /* copy aray since shutdown will mutate this */
  Array<ConnStatePtr> state2 = state;
  state.resize(0);
  for (i = 0; i < state2 (); i++)
    state2[i]->shutdown();
  pth_event_free (stop, PTH_FREE_THIS);
}

void
EIBnetDiscover::Run (pth_sem_t * stop1)
{
  EIBNetIPPacket *p1;
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);

  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      p1 = sock->Get (stop);
      if (p1)
	parent->handle_packet (p1, this->sock);
    }
  pth_event_free (stop, PTH_FREE_THIS);
}

void ConnState::tunnel_request(EIBnet_TunnelRequest &r1, EIBNetIPSocket *isock)
{
  EIBnet_TunnelACK r2;
  r2.channel = r1.channel;
  r2.seqno = r1.seqno;

  if (rno == ((r1.seqno + 1) & 0xff))
    {
      TRACEPRINTF (t, 8, this, "Lost ACK for %d", rno);
      isock->Send (r2.ToPacket (), daddr);
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
      L_Data_PDU *c = CEMI_to_L_Data (r1.CEMI, shared_from_this());
      if (c)
	{
	  r2.status = 0;
          if (c->source == 0)
            c->source = remoteAddr;
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
	r2.status = 0x29;
    }
  else
    {
      TRACEPRINTF (t, 8, this, "Type not CT_STANDARD (%d)", type);
      r2.status = 0x29;
    }
  rno++;
  isock->Send (r2.ToPacket (), daddr);
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

void ConnState::config_request(EIBnet_ConfigRequest &r1, EIBNetIPSocket *isock)
{
  EIBnet_ConfigACK r2;
  if (rno == ((r1.seqno + 1) & 0xff))
    {
      r2.channel = r1.channel;
      r2.seqno = r1.seqno;
      isock->Send (r2.ToPacket (), daddr);
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
  isock->Send (r2.ToPacket (), daddr);
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
