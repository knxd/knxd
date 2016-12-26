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

EIBnetServer::EIBnetServer (TracePtr tr, String serverName)
	: Layer2mixin::Layer2mixin (tr)
  , name(serverName)
  , mcast(NULL)
  , sock(NULL)
  , tunnel(false)
  , route(false)
  , discover(false)
  , Port(-1)
  , sock_mac(-1)
{
  drop_trigger.set<EIBnetServer,&EIBnetServer::drop_trigger_cb>(this);
  drop_trigger.start();
}

EIBnetDiscover::EIBnetDiscover (EIBnetServer *parent, const char *multicastaddr, int port)
{
  struct sockaddr_in baddr;
  struct ip_mreq mcfg;
  sock = 0;

  this->parent = parent;
  TRACEPRINTF (parent->t, 8, this, "OpenD");
  memset (&baddr, 0, sizeof (baddr));
#ifdef HAVE_SOCKADDR_IN_LEN
  baddr.sin_len = sizeof (baddr);
#endif
  baddr.sin_family = AF_INET;
  baddr.sin_addr.s_addr = htonl (INADDR_ANY);
  baddr.sin_port = htons (port);

  if (GetHostIP (parent->t, &maddr, multicastaddr) == 0)
    {
      ERRORPRINTF (parent->t, E_ERROR | 11, this, "Addr '%s' not resolvable", multicastaddr);
      goto err_out;
    }
  maddr.sin_port = htons (port);

  sock = new EIBNetIPSocket (baddr, 1, parent->t);
  if (!sock->init ())
    goto err_out;
  sock->on_recv.set<EIBnetDiscover,&EIBnetDiscover::on_recv_cb>(this);
  //sock->on_error.set<EIBnetServer,&EIBnetServer::on_error_cb>(parent);

  mcfg.imr_multiaddr = maddr.sin_addr;
  mcfg.imr_interface.s_addr = htonl (INADDR_ANY);
  if (!sock->SetMulticast (mcfg))
    goto err_out;

  /** This causes us to ignore multicast packets sent by ourselves */
  if (!GetSourceAddress (&maddr, &sock->localaddr))
    goto err_out;
  sock->localaddr.sin_port = parent->Port;
  sock->recvall = 2;

  TRACEPRINTF (parent->t, 8, this, "OpenedD");
  return;

err_out:
  if (sock)
    {
      delete (sock);
      sock = 0;
    }
  return;
}

bool
EIBnetDiscover::init ()
{
  if (! sock)
    return false;
  
  return true;
}

EIBnetServer::~EIBnetServer ()
{
  stop();
  TRACEPRINTF (t, 8, this, "Close");
}

EIBnetDiscover::~EIBnetDiscover ()
{
  TRACEPRINTF (parent->t, 8, this, "CloseD");
  if (sock)
    delete sock;
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
                    const bool discover)
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
  baddr.sin_port = 0;

  sock = new EIBNetIPSocket (baddr, 1, t);
  if (!sock)
  {
    ERRORPRINTF (t, E_ERROR | 41, this, "EIBNetIPSocket creation failed");
    goto err_out1;
  }

  if (!sock->init ())
    goto err_out2;

  sock->on_recv.set<EIBnetServer,&EIBnetServer::on_recv_cb>(this);

  sock->recvall = 1;
  Port = sock->port ();

  mcast = new EIBnetDiscover (this, multicastaddr, port);
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
  if (this->route || this->tunnel)
    addGroupAddress(0);

  TRACEPRINTF (t, 8, this, "Opened");

  if (!Layer2mixin::init(l3))
    goto err_out3;

  return true;

err_out4:
  Layer2mixin::RunStop();
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
EIBnetServer::Send_L_Data (L_Data_PDU * l)
{
  if (route)
    {
      TRACEPRINTF (t, 8, this, "Send_Route %s", l->Decode ().c_str());
      EIBNetIPPacket p;
      p.service = ROUTING_INDICATION;
      p.data = L_Data_ToCEMI (0x29, *l);
      Send (p);
    }
  delete l;
}

bool ConnState::init()
{
  if (! Layer2mixin::init(parent->l3))
    return false;
  if (! addGroupAddress(0))
    return false;
  if (type == CT_BUSMONITOR && ! l3->registerVBusmonitor(this))
    return false;
  return true;
}

void ConnState::Send_L_Busmonitor (L_Busmonitor_PDU * l)
{
  if (type == CT_BUSMONITOR)
    {
      out.put (Busmonitor_to_CEMI (0x2B, *l, no++));
      if (! state)
        send_trigger.send();
    }
}

void ConnState::Send_L_Data (L_Data_PDU * l)
{
  if (type == CT_STANDARD)
    {
      out.put (L_Data_ToCEMI (0x29, *l));
      if (! state)
        send_trigger.send();
    }
  delete l;
}

int
EIBnetServer::addClient (ConnType type, const EIBnet_ConnectRequest & r1,
                         eibaddr_t addr)
{
  unsigned int i;
  int id = 1;
rt:
  ITER(i, state)
    if ((*i)->channel == id)
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

      state.push_back(s);
    }
  return id;
}

ConnState::ConnState (EIBnetServer *p, eibaddr_t addr)
  : Layer2mixin (TracePtr(new Trace(*(p->t),p->t->name+':'+FormatEIBAddr(addr))))
{
  parent = p;
  timeout.set <ConnState,&ConnState::timeout_cb> (this);
  sendtimeout.set <ConnState,&ConnState::sendtimeout_cb> (this);
  send_trigger.set<ConnState,&ConnState::send_trigger_cb>(this);
  send_trigger.start();

  if (!addr)
    remoteAddr = p->l3->get_client_addr ();
  else
    remoteAddr = addr;
  if (remoteAddr)
    addAddress(remoteAddr);
}

void ConnState::sendtimeout_cb(ev::timer &w, int revents)
{
  state++;
  if (state <= 10)
    {
      send_trigger.send();
      return;
    }
  CArray p = out.get ();
  t->TracePacket (2, this, "dropped", p.size(), p.data());

  state = 0;
}

void ConnState::send_trigger_cb(ev::async &w, int revents)
{
  if (out.isempty ())
    return;
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
  sendtimeout.start(1,0);
  parent->mcast->Send (p, daddr);
}

void ConnState::timeout_cb(ev::timer &w, int revents)
{
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
  stop();
}

void ConnState::stop(void)
{
  if (type == CT_BUSMONITOR)
    l3->deregisterVBusmonitor(this);
  timeout.stop();
  sendtimeout.stop();
  send_trigger.stop();
  parent->drop_state (std::static_pointer_cast<ConnState>(shared_from_this()));
}

void EIBnetServer::drop_state (ConnStatePtr s)
{
  drop_q.put(s);
  drop_trigger.send();
}

void EIBnetServer::drop_trigger_cb(ev::async &w, int revents)
{
  while (!drop_q.isempty())
    {
      ConnStatePtr s = drop_q.get();
      ITER(i,state)
        if (*i == s)
          {
            state.erase (i);
            break;
          }
    }
}

ConnState::~ConnState()
{
  TRACEPRINTF (parent->t, 8, this, "CloseS");
  stop();
}

void ConnState::reset_timer()
{
  timeout.set(120,0);
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
      strncpy ((char *) r2.name, name.c_str(), sizeof(r2.name));
      d.version = 1;
      d.family = 2; // core
      r2.services.push_back (d);
      //d.family = 3; // device management
      //r2.services.add (d);
      d.family = 4;
      if (tunnel)
	r2.services.push_back (d);
      d.family = 5;
      if (route)
	r2.services.push_back (d);
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
      strncpy ((char *) r2.name, name.c_str(), sizeof(r2.name));
      d.version = 1;
      d.family = 2;
      if (discover)
	r2.services.push_back (d);
      d.family = 3;
      r2.services.push_back (d);
      d.family = 4;
      if (tunnel)
	r2.services.push_back (d);
      d.family = 5;
      if (route)
	r2.services.push_back (d);
      isock->Send (r2.ToPacket (), r1.caddr);
      goto out;
    }
  if (p1->service == ROUTING_INDICATION && route)
    {
      if (p1->data.size() < 2 || p1->data[0] != 0x29)
	goto out;
      const CArray data = p1->data;
      L_Data_PDU *c = CEMI_to_L_Data (data, shared_from_this());
      if (c)
	{
	  TRACEPRINTF (t, 8, this, "Recv_Route %s", c->Decode ().c_str());
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
      ITER(i, state)
	if ((*i)->channel == r1.channel)
	  {
            res = 0;
            (*i)->reset_timer();
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
      ITER(i,state)
	if ((*i)->channel == r1.channel)
	  {
            res = 0;
            (*i)->channel = 0;
            drop_state(*i);
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
      if (r1.CRI.size() == 3 && r1.CRI[0] == 4 && tunnel)
	{
	  eibaddr_t a = l3->get_client_addr ();
	  r2.CRD.resize (3);
	  r2.CRD[0] = 0x04;
	  TRACEPRINTF (t, 8, this, "Tunnel CONNECTION_REQ with %s", FormatEIBAddr(a).c_str());
	  r2.CRD[1] = (a >> 8) & 0xFF;
	  r2.CRD[2] = (a >> 0) & 0xFF;
	  if (r1.CRI[1] == 0x02 || r1.CRI[1] == 0x80)
	    {
	      int id = addClient ((r1.CRI[1] == 0x80) ? CT_BUSMONITOR : CT_STANDARD, r1, a);
	      if (id <= 0xff)
		{
		  r2.channel = id;
		  r2.status = 0;
		}
	    }
	}
      else if (r1.CRI.size() == 1 && r1.CRI[0] == 3)
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
      ITER(i,state)
	if ((*i)->channel == r1.channel)
	  {
	    (*i)->tunnel_request(r1, isock);
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
      ITER(i, state)
	if ((*i)->channel == r1.channel)
	  {
	    (*i)->tunnel_response (r1);
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
      ITER(i, state)
	if ((*i)->channel == r1.channel)
	  {
	    (*i)->config_request (r1, isock);
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
      ITER(i, state)
	if ((*i)->channel == r1.channel)
	  {
	    (*i)->config_response (r1);
	    break;
	  }
      goto out;
    }
  TRACEPRINTF (t, 8, this, "Unexpected service type: %04x", p1->service);
out:
  delete p1;
}

void
EIBnetServer::on_recv_cb (EIBNetIPPacket *p)
{
  handle_packet (p, this->sock);
}

//void
//EIBnetServer::error_cb ()
//{
//  TRACEPRINTF (t, 8, this, "got an error");
//  stop();
//}

void
EIBnetServer::stop()
{
  ITER(i, state)
    (*i)->stop();
  drop_trigger.stop();
  if (sock)
  {
    delete sock;
    sock = 0;
  }
  if (mcast)
  {
    delete mcast;
    mcast = 0;
  }
  if (sock_mac >= 0)
  {
    close (sock_mac);
    sock_mac = -1;
  }
}

void
EIBnetDiscover::on_recv_cb (EIBNetIPPacket *p)
{
  parent->handle_packet (p, this->sock);
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
	  if (c->hopcount)
	    {
	      c->hopcount--;
              if (c->source == 0)
                c->source = remoteAddr;
	      if (r1.CEMI[0] == 0x11)
		{
		  out.put (L_Data_ToCEMI (0x2E, *c));
		  if (! state)
                    send_trigger.send();
		}
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
  if (!out.isempty())
    send_trigger.send();
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
  if (type == CT_CONFIG && r1.CEMI.size() > 1)
    {
      if (r1.CEMI[0] == 0xFC)
	{
	  if (r1.CEMI.size() == 7)
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
	      CEMI.resize (6 + res.size());
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
              if (! state)
                {
                  state = 1;
                  send_trigger.send();
                }
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
  if (!out.isempty())
    send_trigger.send();
}

