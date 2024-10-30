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
#include "config.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <unistd.h>
#ifndef SIOCGIFHWADDR
#include <sys/sysctl.h>
#include <net/if_dl.h>
#endif

#include "emi.h"

EIBnetServer::EIBnetServer (BaseRouter& r, IniSectionPtr& s)
  : Server(r,s)
  , mcast(NULL)
  , sock(NULL)
  , tunnel(false)
  , route(false)
  , discover(false)
  , Port(-1)
  , sock_mac(-1)
  , router_cfg(s->sub("router",false))
  , tunnel_cfg(s->sub("tunnel",false))
{
  t->setAuxName("server");
  drop_trigger.set<EIBnetServer,&EIBnetServer::drop_trigger_cb>(this);
  drop_trigger.start();
}

EIBnetDriver::EIBnetDriver (LinkConnectClientPtr c,
                            std::string& multicastaddr, int port, std::string& intf)
  : SubDriver(c)
{
  struct sockaddr_in baddr;
  struct ip_mreqn mcfg;
  sock = 0;
  t->setAuxName("driver");

  TRACEPRINTF (t, 8, "OpenD");

  if (GetHostIP (t, &maddr, multicastaddr) == 0)
    {
      ERRORPRINTF (t, E_ERROR | 11, "Addr '%s' not resolvable", multicastaddr);
      goto err_out;
    }

  if (port)
    {
      maddr.sin_port = htons (port);
      memset (&baddr, 0, sizeof (baddr));
#ifdef HAVE_SOCKADDR_IN_LEN
      baddr.sin_len = sizeof (baddr);
#endif
      baddr.sin_family = AF_INET;
      baddr.sin_addr.s_addr = htonl (INADDR_ANY);
      baddr.sin_port = htons (port);

      sock = new EIBNetIPSocket (baddr, 1, t);
      if (!sock->SetInterface(intf))
        goto err_out;
      if (!sock->init ())
        goto err_out;
      sock->on_recv.set<EIBnetDriver,&EIBnetDriver::recv_cb>(this);
      sock->on_error.set<EIBnetDriver,&EIBnetDriver::error_cb>(this);
    }
  else
    {
      EIBnetServer &parent = *std::static_pointer_cast<EIBnetServer>(server);
      maddr.sin_port = parent.Port;
      sock = parent.sock;
    }

  mcfg.imr_multiaddr = maddr.sin_addr;
  mcfg.imr_address.s_addr = htonl (INADDR_ANY);
  mcfg.imr_ifindex = if_nametoindex(intf.c_str());
  if (!sock->SetMulticast (mcfg))
    goto err_out;

  /** This causes us to ignore multicast packets sent by ourselves */
  if (!GetSourceAddress (t, &maddr, &sock->localaddr))
    goto err_out;
  sock->localaddr.sin_port = std::static_pointer_cast<EIBnetServer>(server)->Port;
  sock->recvall = 2;

  TRACEPRINTF (t, 8, "OpenedD");
  return;

err_out:
  if (sock && port)
    delete (sock);
  sock = 0;
  return;
}

bool
EIBnetDriver::setup()
{
  if (!assureFilter("pace"))
    return false;
  if (!SubDriver::setup())
    return false;
  if (! sock)
    return false;

  return true;
}

EIBnetServer::~EIBnetServer ()
{
  stop_(false);
  TRACEPRINTF (t, 8, "Close E");
}

EIBnetDriver::~EIBnetDriver ()
{
  TRACEPRINTF (t, 8, "CloseD");
  EIBnetServer &parent = *std::static_pointer_cast<EIBnetServer>(server);
  EIBNetIPSocket *ps = parent.sock;
  if (sock && ps && ps != sock)
    delete sock;
}

bool
EIBnetServer::setup()
//(const char *multicastaddr, const int port, const char *intf,
//                     const bool tunnel, const bool route,
//                     const bool discover, const bool single_port)
{
  if(!Server::setup())
    return false;
  route = router_cfg->name.size() > 0;
  tunnel = tunnel_cfg->name.size() > 0;
  discover = cfg->value("discover",false);
  single_port = !cfg->value("multi-port",false);
  multicastaddr = cfg->value("multicast-address","224.0.23.12");
  port = cfg->value("port",3671);
  interface = cfg->value("interface","");
  servername = cfg->value("name", dynamic_cast<Router *>(&router)->servername);
  keepalive = cfg->value("heartbeat-timeout", CONNECTION_ALIVE_TIME);


  if (tunnel)
    {
      /* Check that we have client addresses. */
      if (!static_cast<Router&>(router).hasClientAddrs())
        return false;
      /* set up a temporary fake tunnel stack to test the arguments early. */
      if (!static_cast<Router &>(router).checkStack(tunnel_cfg))
        return false;
    }

  if (route)
    {
      if (!static_cast<Router &>(router).checkStack(router_cfg))
        return false;
    }

  return true;
}

void
EIBnetServer::start()
{
  struct sockaddr_in baddr;
  LinkConnectClientPtr mcast_conn;

  TRACEPRINTF (t, 8, "Open");

  sock_mac = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock_mac < 0)
    {
      ERRORPRINTF (t, E_ERROR | 27, "Lookup socket creation failed");
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
      ERRORPRINTF (t, E_ERROR | 41, "EIBNetIPSocket creation failed");
      goto err_out1;
    }
  sock->SetInterface(interface);

  if (!sock->init ())
    goto err_out2;

  sock->on_recv.set<EIBnetServer,&EIBnetServer::recv_cb>(this);
  sock->on_error.set<EIBnetServer,&EIBnetServer::error_cb>(this);

  sock->recvall = 1;
  Port = sock->port ();

  mcast_conn = LinkConnectClientPtr(new LinkConnectClient(std::dynamic_pointer_cast<EIBnetServer>(shared_from_this()), router_cfg, t));
  mcast = EIBnetDriverPtr(new EIBnetDriver (mcast_conn, multicastaddr, single_port ? 0 : port, interface));
  if (!mcast)
    {
      ERRORPRINTF (t, E_ERROR | 42, "EIBnetDriver creation failed");
      goto err_out2;
    }
  mcast_conn->set_driver(mcast);
  if (!mcast_conn->setup ())
    goto err_out3;
  if (route && !static_cast<Router &>(router).registerLink(mcast_conn))
    goto err_out3;

  TRACEPRINTF (t, 8, "Opened");

  Server::start();
  return;

err_out3:
  mcast.reset();
err_out2:
  delete sock;
  sock = NULL;
err_out1:
  close (sock_mac);
  sock_mac = -1;
err_out0:
  Server::stop(true);
}

void EIBnetDriver::Send (EIBNetIPPacket p, struct sockaddr_in addr)
{
  if (sock)
    sock->Send (p, addr);
}

void
EIBnetDriver::send_L_Data (LDataPtr l)
{
  EIBnetServer &parent = *std::static_pointer_cast<EIBnetServer>(server);
  if (parent.route)
    {
      EIBNetIPPacket p;
      p.service = ROUTING_INDICATION;
      p.data = L_Data_ToCEMI (0x29, l);
      parent.Send (p);
    }
  send_Next();
}

bool ConnState::setup()
{
  // Force queuing so that a bad or unreachable client can't disable the whole system
  if (!assureFilter("queue", true))
    return false;
  if (! SubDriver::setup())
    return false;
  if (type == CT_BUSMONITOR && ! dynamic_cast<Router *>(&server->router)->registerVBusmonitor(this))
    return false;

  addAddress(addr);
  TRACEPRINTF (t, 8, "Start Conn %d", channel);
  return true;
}

void ConnState::send_L_Busmonitor (LBusmonPtr l)
{
  if (type == CT_BUSMONITOR)
    {
      out.put (Busmonitor_to_CEMI (0x2B, l, no++));
      if (! retries)
        send_trigger.send();
    }
}

void ConnState::send_L_Data (LDataPtr l)
{
  if (type == CT_STANDARD)
    {
      assert (!do_send_next);
      do_send_next = true;
      out.put (L_Data_ToCEMI (0x29, l));
      if (! retries)
        send_trigger.send();
    }
}

int
EIBnetServer::addClient (ConnType type, const EIBnet_ConnectRequest & r1,
                         eibaddr_t addr)
{
  int id = 1;
rt:
  ITER(i, connections)
  if ((*i)->channel == id)
    {
      id++;
      goto rt;
    }
  if (id <= 0xff)  // TODO configurable maximum
    {
      LinkConnectClientPtr conn = LinkConnectClientPtr(new LinkConnectClient(std::dynamic_pointer_cast<EIBnetServer>(shared_from_this()), tunnel_cfg, t));
      ConnStatePtr s = ConnStatePtr(new ConnState(this, conn, addr));
      conn->set_driver(s);
      s->channel = id;
      s->daddr = r1.daddr;
      s->caddr = r1.caddr;
      s->retries = 0;
      s->sno = 0;
      s->rno = 0;
      s->no = 1;
      s->type = type;
      s->nat = r1.nat;
      if(!conn->setup())
        return -1;
      if(!static_cast<Router &>(router).registerLink(conn, true))
        return -1;
      connections.push_back(s);
      return id;
    }
  return -1;
}

ConnState::ConnState (EIBnetServer *parent, LinkConnectClientPtr c, eibaddr_t addr)
  : L_Busmonitor_CallBack(c->t->name), SubDriver (c)
{
  this->parent = parent;
  t->setAuxName(FormatEIBAddr(addr));
  timeout.set <ConnState,&ConnState::timeout_cb> (this);
  sendtimeout.set <ConnState,&ConnState::sendtimeout_cb> (this);
  send_trigger.set<ConnState,&ConnState::send_trigger_cb>(this);
  send_trigger.start();
  timeout.start(parent->keepalive, 0);
  this->addr = addr;
  TRACEPRINTF (t, 9, "has %s", FormatEIBAddr (addr));
}

void ConnState::sendtimeout_cb(ev::timer &, int)
{
  if (++retries <= 2)
    {
      send_trigger.send();
      return;
    }
  CArray p = out.get ();
  t->TracePacket (2, "dropped no-ACK", p.size(), p.data());
  stop(true);
}

void ConnState::send_trigger_cb(ev::async &, int)
{
  if (out.empty ())
    return;
  EIBNetIPPacket p;
  if (type == CT_CONFIG)
    {
      EIBnet_ConfigRequest r;
      r.channel = channel;
      r.seqno = sno;
      r.CEMI = out.front ();
      p = r.ToPacket ();
    }
  else
    {
      EIBnet_TunnelRequest r;
      r.channel = channel;
      r.seqno = sno;
      r.CEMI = out.front ();
      p = r.ToPacket ();
    }
  retries ++;
  sendtimeout.start(TUNNELING_REQUEST_TIMEOUT,0);
  std::static_pointer_cast<EIBnetServer>(server)->mcast->Send (p, daddr);
}

void ConnState::timeout_cb(ev::timer &, int)
{
  if (channel > 0)
    {
      EIBnet_DisconnectRequest r;
      r.channel = channel;
      if (GetSourceAddress (t, &caddr, &r.caddr))
        {
          r.caddr.sin_port = std::static_pointer_cast<EIBnetServer>(server)->Port;
          r.nat = nat;
          std::static_pointer_cast<EIBnetServer>(server)->Send (r.ToPacket (), caddr);
        }
    }
  stop(true);
}

void ConnState::stop(bool err)
{
  TRACEPRINTF (t, 8, "Stop Conn %d", channel);
  if (type == CT_BUSMONITOR)
    dynamic_cast<Router *>(&server->router)->deregisterVBusmonitor(this);
  timeout.stop();
  sendtimeout.stop();
  send_trigger.stop();
  retries = 0;
  std::static_pointer_cast<EIBnetServer>(server)->drop_connection (std::static_pointer_cast<ConnState>(shared_from_this()));
  if (addr)
    {
      dynamic_cast<Router *>(&server->router)->release_client_addr(addr);
      addr = 0;
    }
  SubDriver::stop(err);
}

void EIBnetServer::drop_connection (ConnStatePtr s)
{
  drop_q.put(std::move(s));
  drop_trigger.send();
}

void EIBnetServer::drop_trigger_cb(ev::async &, int)
{
  while (!drop_q.empty())
    {
      ConnStatePtr s = drop_q.get();
      ITER(i,connections)
      if (*i == s)
        {
          connections.erase (i);
          auto c = std::dynamic_pointer_cast<LinkConnect>(s->conn.lock());
          if (c != nullptr)
            static_cast<Router &>(router).unregisterLink(c);
          break;
        }
    }
}

ConnState::~ConnState()
{
  TRACEPRINTF (t, 8, "CloseS");
}

void ConnState::reset_timer()
{
  timeout.set(parent->keepalive, 0);
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
#ifdef SIOCGIFHWADDR
              if (ioctl(sock_mac, SIOCGIFHWADDR, &ifr))
                continue;
              if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER)
                continue;
              memcpy(mac_address, ifr.ifr_hwaddr.sa_data, sizeof(mac_address));
#else
              /* for FreeBSD, doesn't have ioctl SIOCGIFHWADDR */
              int mib[6];
              size_t len;
              char *buf;
              unsigned char *ptr;
              struct if_msghdr        *ifm;
              struct sockaddr_dl        *sdl;

              mib[0] = CTL_NET;
              mib[1] = AF_ROUTE;
              mib[2] = 0;
              mib[3] = AF_LINK;
              mib[4] = NET_RT_IFLIST;

              if ((mib[5] = if_nametoindex(ifr.ifr_name)) == 0)
                {
                  TRACEPRINTF(t, 2, "get_mac_address if_nametoindex error");
                  goto out;
                }
              if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0)
                {
                  TRACEPRINTF(t, 2, "get_mac_address sysctl 1 error");
                  goto out;
                }

              buf = new char[len];

              if (sysctl(mib, 6, buf, &len, NULL, 0) < 0)
                {
                  TRACEPRINTF(t, 2, "get_mac_address sysctl 2 error");
                  goto out;
                }

              ifm = (struct if_msghdr *)buf;
              sdl = (struct sockaddr_dl *)(ifm + 1);
              ptr = (unsigned char *)LLADDR(sdl);
              memcpy(mac_address, ptr, sizeof(mac_address));
#endif
              break;
            }
        }
    }
  /* End MAC Address */

  if (p1->service == SEARCH_REQUEST)
    {
      EIBnet_SearchRequest r1;
      EIBnet_SearchResponse r2;
      DIB_service_Entry d;
      if (parseEIBnet_SearchRequest (*p1, r1))
        {
          t->TracePacket (2, "unparseable SEARCH_REQUEST", p1->data);
          goto out;
        }
      TRACEPRINTF (t, 8, "SEARCH_REQ");
      if (!discover)
        goto out;

      r2.KNXmedium = 2;
      r2.devicestatus = 0;
      r2.individual_addr = dynamic_cast<Router *>(&router)->addr;
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
      strncpy ((char *) r2.name, servername.c_str(), sizeof(r2.name) - 1);
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
      if (!GetSourceAddress (t, &r1.caddr, &r2.caddr))
        goto out;
      r2.caddr.sin_port = Port;
      isock->Send (r2.ToPacket (), r1.caddr);
      goto out;
    }

  if (p1->service == DESCRIPTION_REQUEST)
    {
      EIBnet_DescriptionRequest r1;
      EIBnet_DescriptionResponse r2;
      DIB_service_Entry d;
      if (parseEIBnet_DescriptionRequest (*p1, r1))
        {
          t->TracePacket (2, "unparseable DESCRIPTION_REQUEST", p1->data);
          goto out;
        }
      if (!discover)
        goto out;
      TRACEPRINTF (t, 8, "DESCRIBE");
      r2.KNXmedium = 2;
      r2.devicestatus = 0;
      r2.individual_addr = dynamic_cast<Router *>(&router)->addr;
      r2.installid = 0;
      r2.multicastaddr = mcast->maddr.sin_addr;
      memcpy(r2.MAC, mac_address, sizeof(r2.MAC));
      //FIXME: Hostname, indiv. address
      strncpy ((char *) r2.name, servername.c_str(), sizeof(r2.name) - 1);
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
  if (p1->service == ROUTING_INDICATION)
    {
      if (p1->data.size() < 2 || p1->data[0] != 0x29)
        {
          t->TracePacket (2, "unparseable ROUTING_INDICATION", p1->data);
          goto out;
        }
      LDataPtr c = CEMI_to_L_Data (p1->data, t);
      if (!c)
        t->TracePacket (2, "unCEMIable ROUTING_INDICATION", p1->data);
      else if (route)
        mcast->recv_L_Data (std::move(c));
      goto out;
    }
  if (p1->service == CONNECTIONSTATE_REQUEST)
    {
      EIBnet_ConnectionStateRequest r1;
      EIBnet_ConnectionStateResponse r2;
      if (parseEIBnet_ConnectionStateRequest (*p1, r1))
        {
          t->TracePacket (2, "unparseable CONNECTIONSTATE_REQUEST", p1->data);
          goto out;
        }
      r2.channel = r1.channel;
      r2.status = E_CONNECTION_ID;
      ITER(i, connections)
      if ((*i)->channel == r1.channel)
        {
          TRACEPRINTF ((*i)->t, 8, "CONNECTIONSTATE_REQUEST on %d", r1.channel);
          r2.status = 0;
          (*i)->reset_timer();
          break;
        }
      if (r2.status)
        TRACEPRINTF (t, 2, "Unknown connection %d", r2.channel);

      isock->Send (r2.ToPacket (), r1.caddr);
      goto out;
    }
  if (p1->service == DISCONNECT_REQUEST)
    {
      EIBnet_DisconnectRequest r1;
      EIBnet_DisconnectResponse r2;
      if (parseEIBnet_DisconnectRequest (*p1, r1))
        {
          t->TracePacket (2, "unparseable DISCONNECT_REQUEST", p1->data);
          goto out;
        }
      r2.status = E_CONNECTION_ID;
      r2.channel = r1.channel;
      ITER(i,connections)
      if ((*i)->channel == r1.channel)
        {
          r2.status = 0;
          TRACEPRINTF ((*i)->t, 8, "DISCONNECT_REQUEST");
          (*i)->stop(false);
          break;
        }
      if (r2.status)
        TRACEPRINTF (t, 8, "DISCONNECT_REQUEST on %d", r1.channel);
      isock->Send (r2.ToPacket (), r1.caddr);
      goto out;
    }
  if (p1->service == CONNECTION_REQUEST)
    {
      EIBnet_ConnectRequest r1;
      EIBnet_ConnectResponse r2;
      if (parseEIBnet_ConnectRequest (*p1, r1))
        {
          t->TracePacket (2, "unparseable CONNECTION_REQUEST", p1->data);
          goto out;
        }
      r2.status = E_CONNECTION_TYPE;
      if (r1.CRI.size() == 3 && r1.CRI[0] == 4)
        {
          eibaddr_t a = tunnel ? static_cast<Router &>(router).get_client_addr (t) : 0;
          r2.CRD.resize (3);
          r2.CRD[0] = 0x04;
          if (tunnel)
            TRACEPRINTF (t, 8, "Tunnel CONNECTION_REQ with %s", FormatEIBAddr(a));
          r2.CRD[1] = (a >> 8) & 0xFF;
          r2.CRD[2] = (a >> 0) & 0xFF;
          if (!tunnel)
            TRACEPRINTF (t, 8, "Tunnel CONNECTION_REQ, ignored, not tunneling");
          else if (!a)
            {
              TRACEPRINTF (t, 8, "Tunnel CONNECTION_REQ no free addresses");
              r2.status = E_NO_MORE_CONNECTIONS;
            }
          else if (r1.CRI[1] == 0x02 || r1.CRI[1] == 0x80)
            {
              int id = addClient ((r1.CRI[1] == 0x80) ? CT_BUSMONITOR : CT_STANDARD, r1, a);
              if (id >= 0)
                {
                  r2.channel = id;
                  r2.status = E_NO_ERROR;
                }
            }
          else
            {
              r2.status = E_TUNNELING_LAYER;
              TRACEPRINTF (t, 8, "bad CONNECTION_REQ: [1] x%02x", r1.CRI[1]);
            }
        }
      else if (r1.CRI.size() == 1 && r1.CRI[0] == 3)
        {
          r2.CRD.resize (1);
          r2.CRD[0] = 0x03;
          TRACEPRINTF (t, 8, "Tunnel CONNECTION_REQ, no addr (mgmt)");
          int id = addClient (CT_CONFIG, r1, 0);
          if (id >= 0)
            {
              r2.channel = id;
              r2.status = E_NO_ERROR;
            }
        }
      else
        {
          TRACEPRINTF (t, 8, "bad CONNECTION_REQ: size %d, [0] x%02x", r1.CRI.size(), r1.CRI[0]);
          // XXX set status to something more reasonable
        }
      if (!GetSourceAddress (t, &r1.caddr, &r2.daddr))
        goto out;
      if (tunnel && (r2.status != E_NO_ERROR))
        {
          if (r2.status == E_NO_MORE_CONNECTIONS)
            TRACEPRINTF (t, 8, "CONNECTION_REQ: no free channel");
          else
            TRACEPRINTF (t, 8, "CONNECTION_REQ: error x%x", r2.status);
        }
      r2.daddr.sin_port = Port;
      r2.nat = r1.nat;
      isock->Send (r2.ToPacket (), r1.caddr);
      goto out;
    }
  if (p1->service == TUNNEL_REQUEST)
    {
      EIBnet_TunnelRequest r1;
      EIBnet_TunnelACK r2;
      if (parseEIBnet_TunnelRequest (*p1, r1))
        {
          t->TracePacket (2, "unparseable TUNNEL_REQUEST", p1->data);
          goto out;
        }
      if (tunnel)
        ITER(i,connections)
        if ((*i)->channel == r1.channel)
          {
            (*i)->tunnel_request(r1, isock);
            goto out;
          }
      TRACEPRINTF (t, 8, "TUNNEL_REQ on unknown %d", r1.channel);
      goto out;
    }
  if (p1->service == TUNNEL_RESPONSE)
    {
      EIBnet_TunnelACK r1;
      if (parseEIBnet_TunnelACK (*p1, r1))
        {
          t->TracePacket (2, "unparseable TUNNEL_RESPONSE", p1->data);
          goto out;
        }
      if (tunnel)
        ITER(i, connections)
        if ((*i)->channel == r1.channel)
          {
            (*i)->tunnel_response (r1);
            goto out;
          }
      TRACEPRINTF (t, 8, "TUNNEL_ACK on unknown %d",r1.channel);
      goto out;
    }
  if (p1->service == DEVICE_CONFIGURATION_REQUEST)
    {
      EIBnet_ConfigRequest r1;
      EIBnet_ConfigACK r2;
      if (parseEIBnet_ConfigRequest (*p1, r1))
        {
          t->TracePacket (2, "unparseable DEVICE_CONFIGURATION_REQUEST", p1->data);
          goto out;
        }
      TRACEPRINTF (t, 8, "CONFIG_REQ on %d",r1.channel);
      ITER(i, connections)
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
        {
          t->TracePacket (2, "unparseable DEVICE_CONFIGURATION_ACK", p1->data);
          goto out;
        }
      ITER(i, connections)
      if ((*i)->channel == r1.channel)
        {
          (*i)->config_response (r1);
          goto out;
        }
      TRACEPRINTF (t, 8, "CONFIG_ACK on unknown channel %d",r1.channel);
      goto out;
    }
  TRACEPRINTF (t, 8, "Unexpected service type: %04x", p1->service);
out:
  delete p1;
}

void
EIBnetServer::recv_cb (EIBNetIPPacket *p)
{
  handle_packet (p, this->sock);
}

void
EIBnetServer::error_cb ()
{
  ERRORPRINTF (t, E_ERROR | 46, "Communication error: %s", strerror(errno));
  stop(true);
}

//void
//EIBnetServer::error_cb ()
//{
//  TRACEPRINTF (t, 8, "got an error");
//  stop();
//}

void
EIBnetServer::stop(bool err)
{
  stop_(err);
  Server::stop(err);
}

void
EIBnetServer::stop_(bool err)
{
  drop_trigger.stop();

  R_ITER(i,connections)
    (*i)->stop(err);

  if (mcast)
    {
      auto c = std::dynamic_pointer_cast<LinkConnect>(mcast->conn.lock());

      if (c)
        {
          c->stop(err);
          if(route)
            static_cast<Router &>(router).unregisterLink(c);
        }
      mcast.reset();
    }
  if (sock)
    {
      delete sock;
      sock = 0;
    }
  if (sock_mac >= 0)
    {
      close (sock_mac);
      sock_mac = -1;
    }
}

void
EIBnetDriver::recv_cb (EIBNetIPPacket *p)
{
  EIBnetServer &parent = *std::static_pointer_cast<EIBnetServer>(server);
  parent.handle_packet (p, this->sock);
}

void
EIBnetDriver::error_cb ()
{
  EIBnetServer &parent = *std::static_pointer_cast<EIBnetServer>(server);
  ERRORPRINTF (t, E_ERROR | 47, "Communication error (driver): %s", strerror(errno));
  parent.stop(true);
}

void ConnState::tunnel_request(EIBnet_TunnelRequest &r1, EIBNetIPSocket *isock)
{
  EIBnet_TunnelACK r2;
  r2.channel = r1.channel;
  r2.seqno = r1.seqno;

  if (rno == ((r1.seqno + 1) & 0xff))
    {
      TRACEPRINTF (t, 8, "Lost ACK for %d", rno);
      isock->Send (r2.ToPacket (), daddr);
      return;
    }
  if (rno != r1.seqno)
    {
      TRACEPRINTF (t, 8, "Wrong sequence %d<->%d",
                   r1.seqno, rno);
      return;
    }
  if (type == CT_STANDARD)
    {
      TRACEPRINTF (t, 8, "TUNNEL_REQ");
      LDataPtr c = CEMI_to_L_Data (r1.CEMI, t);
      if (c)
        {
          r2.status = 0;
          if (c->source_address == 0)
            c->source_address = addr;
          if (r1.CEMI[0] == 0x11)
            {
              out.put (L_Data_ToCEMI (0x2E, c));
              if (! retries)
                send_trigger.send();
            }
          if (r1.CEMI[0] == 0x11 || r1.CEMI[0] == 0x29)
            recv_L_Data (std::move(c));
          else
            TRACEPRINTF (t, 8, "Wrong leader x%02x", r1.CEMI[0]);
        }
      else
        r2.status = 0x29;
    }
  else
    {
      TRACEPRINTF (t, 8, "Type not CT_STANDARD (%d)", type);
      r2.status = 0x29;
    }
  rno++;
  isock->Send (r2.ToPacket (), daddr);

  reset_timer(); // presumably the client is alive if it can send
}

void ConnState::tunnel_response (EIBnet_TunnelACK &r1)
{
  TRACEPRINTF (t, 8, "TUNNEL_ACK");
  if (sno != r1.seqno)
    {
      TRACEPRINTF (t, 8, "Wrong sequence %d<->%d",
                   r1.seqno, sno);
      return;
    }
  if (r1.status != 0)
    {
      TRACEPRINTF (t, 8, "Wrong status %d", r1.status);
      return;
    }
  if (! retries)
    {
      TRACEPRINTF (t, 8, "Unexpected ACK 1");
      return;
    }
  if (type != CT_STANDARD && type != CT_BUSMONITOR)
    {
      TRACEPRINTF (t, 8, "Unexpected Connection Type");
      return;
    }
  sno++;

  out.get ();
  sendtimeout.stop();
  reset_timer(); // presumably the client is alive if it can ack
  retries = 0;
  if (!out.empty())
    send_trigger.send();
  else if (do_send_next)
    {
      do_send_next = false;
      send_Next();
    }
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
      TRACEPRINTF (t, 8, "Wrong sequence %d<->%d",
                   r1.seqno, rno);
      return;
    }
  r2.channel = r1.channel;
  r2.seqno = r1.seqno;
  if (type == CT_CONFIG && r1.CEMI.size() > 1)
    {
      if (r1.CEMI[0] == 0xFC) // M_PropRead.req
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
              r2.status = E_NO_ERROR;

              out.push (CEMI);
              if (! retries)
                send_trigger.send();
            }
          else
            r2.status = E_DATA_CONNECTION;
        }
      else
        r2.status = E_DATA_CONNECTION;
    }
  else
    r2.status = E_TUNNELING_LAYER;
  rno++;
  isock->Send (r2.ToPacket (), daddr);
}

void ConnState::config_response (EIBnet_ConfigACK &r1)
{
  TRACEPRINTF (t, 8, "CONFIG_ACK");
  if (sno != r1.seqno)
    {
      TRACEPRINTF (t, 8, "Wrong sequence %d<->%d",
                   r1.seqno, sno);
      return;
    }
  if (r1.status != 0)
    {
      TRACEPRINTF (t, 8, "Wrong status %d", r1.status);
      return;
    }
  if (!retries)
    {
      TRACEPRINTF (t, 8, "Unexpected ACK 2");
      return;
    }
  if (type != CT_CONFIG)
    {
      TRACEPRINTF (t, 8, "Unexpected Connection Type");
      return;
    }
  sno++;
  sendtimeout.stop();

  out.get ();
  retries = 0;
  if (!out.empty())
    send_trigger.send();
  else if (do_send_next)
    {
      do_send_next = false;
      send_Next();
    }
}

