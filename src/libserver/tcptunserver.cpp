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

#include "tcptunserver.h"
#include "config.h"
#include "tunchannel.h"

#include <netinet/tcp.h>
#include <sys/un.h>
#include <arpa/inet.h>

TcpTunConn::TcpTunConn(TcpTunServer *parent, uint32_t connectionID, int fd)
  : t(TracePtr(new Trace(*parent->t)))
  , sendbuf(fd)
  , recvbuf(fd)
  , connectionID(connectionID)
  , fd(fd)
{
  this->parent = parent;

  recvbuf.on_read.set<TcpTunConn, &TcpTunConn::read_cb>(this);
  recvbuf.on_error.set<TcpTunConn, &TcpTunConn::error_cb>(this);
  sendbuf.on_error.set<TcpTunConn, &TcpTunConn::error_cb>(this);

  timeout.set<TcpTunConn,&TcpTunConn::timeout_cb>(this);
  timeout.start(parent->keepalive, 0);

  // Get address of local host
  sockaddr_in localSocketAddress;
  socklen_t len = sizeof(localSocketAddress);
  if (getsockname(fd, (struct sockaddr *)&localSocketAddress, &len) != 0 ||
      len != sizeof(localSocketAddress) ||
      localSocketAddress.sin_family != AF_INET)
    {
      char str[64];
      snprintf(str, sizeof(str), "stream-%d", connectionID);
      t->setAuxName(str);
    }
  else
    {
      char addrStr[64];
      if (inet_ntop(localSocketAddress.sin_family, &localSocketAddress.sin_addr, addrStr, sizeof(addrStr)))
        {
          char addrPortStr[64];
          snprintf(addrPortStr, sizeof(addrPortStr), "%s:%d", addrStr, localSocketAddress.sin_port);
          t->setAuxName(addrPortStr);
        }
      else
        {
          t->setAuxName("addr-error");
        }
    }
}

TcpTunConn::~TcpTunConn()
{
  TRACEPRINTF (t, 8, "Closing TcpTunConn");
}

void TcpTunConn::reset_timer()
{
  timeout.set(parent->keepalive, 0);
}

void
TcpTunConn::error_cb()
{
  TRACEPRINTF (t, 8, "TcpTunConn communication error");
  stop(true);
}

size_t
TcpTunConn::read_cb(uint8_t *buf, size_t len)
{
  size_t done = 0;
  for (;;) {
    if (len < HEADER_SIZE_10)
      return done;
    if (buf[0] != HEADER_SIZE_10 || buf[1] != KNXNETIP_VERSION_10)
      {
        stop(true);
        return done;
      }
    int tlen = (buf[4] << 8) | buf[5];
    if (tlen > len)
      return done;
  
    t->TracePacket(0, "TCP recv", tlen, buf);

    CArray data(buf, tlen);
    std::unique_ptr<EIBNetIPPacket> packet(EIBNetIPPacket::fromPacket(data, routeBackAddr(), IPV4_TCP));
    if (!packet)
      {
        stop(true);
        return done;
      }

    handlePacket(*packet);
    done += tlen;
    buf += tlen;
    len -= tlen;
  }
}

void TcpTunConn::timeout_cb(ev::timer &, int)
{
  TRACEPRINTF (t, 8, "Timeout for TCP connection");

  stop(true);
}

int TcpTunConn::getFreeChannelID()
{
  uint8_t res = this->lastChannelID;

  while (true)
    {
      res = (res + 1) & 0xff;

      // Channel res is free
      if (this->channels.find(res) == this->channels.end())
        {
          this->lastChannelID = res;
          return res;
        }

      // No channel is free
      if (res == this->lastChannelID)
        {
          return -1;
        }
    }
}

bool TcpTunConn::openChannel(const TunChannelPtr& channel)
{
  if (channels.find(channel->channelID) != channels.end())
    {
      TRACEPRINTF (t, 8, "Attempting to reuse open channel ID");
      return false;
    }

  if (!channel->setupChannel())
    {
      TRACEPRINTF (t, 8, "Channel setup failed");
      channel->stop(true);
      return false;
    }

  this->channels[channel->channelID] = channel;

  return true;
}

void TcpTunConn::closeChannel(const TunChannelPtr& channel)
{
  TRACEPRINTF (t, 8, "Closing channel %d", channel->channelID);

  channel->stop(false);

  this->channels.erase(channel->channelID);
}

TunChannelPtr TcpTunConn::findChannel(uint8_t channelID)
{
  auto it = this->channels.find(channelID);
  if (it == this->channels.end())
    return TunChannelPtr();
  return it->second;
}

void TcpTunConn::stop(bool err)
{
  TRACEPRINTF (t, 8, "Stop Conn");

  // Close all channels
  while (true)
    {
      auto it = this->channels.begin();
      if (it == this->channels.end())
        break;
      closeChannel(it->second);
    }

  sendbuf.stop();
  recvbuf.stop();

  close(fd);
  fd = -1;

  timeout.stop();

  parent->deregister(shared_from_this());
}

void
TcpTunConn::start()
{
  if (running)
    return;
  if (fd == -1)
    return;

  sendbuf.start();
  recvbuf.start();

  running = true;
}

bool TcpTunConn::setup()
{
  return true;
}

void
TcpTunConn::send(const EIBNetIPPacket& p)
{
  CArray data = p.ToPacket();

  t->TracePacket(0, "TCP send", data.size(), data.data());

  if (fd >= 0)
    sendbuf.write(data.data(), data.size());
}

void
TcpTunConn::handlePacket(const EIBNetIPPacket &p1)
{
  if (p1.service == CONNECTIONSTATE_REQUEST)
    {
      EIBnet_ConnectionStateRequest r1;
      EIBnet_ConnectionStateResponse r2;

      if (parseEIBnet_ConnectionStateRequest(p1, r1))
        {
          t->TracePacket(2, "unparseable CONNECTIONSTATE_REQUEST", p1.data);
          return;
        }

      reset_timer();

      r2.channel = r1.channel;
      auto channel = findChannel(r1.channel);
      if (!channel)
        {
          r2.status = E_CONNECTION_ID;
          send(r2.ToPacket(IPV4_TCP));
          return;
        }

      TRACEPRINTF (t, 8, "CONNECTIONSTATE_REQUEST");

      send(r2.ToPacket(IPV4_TCP));
      return;
    }

  if (p1.service == DISCONNECT_REQUEST)
    {
      EIBnet_DisconnectRequest r1;
      EIBnet_DisconnectResponse r2;
      if (parseEIBnet_DisconnectRequest(p1, r1))
        {
          t->TracePacket (2, "unparseable DISCONNECT_REQUEST", p1.data);
          return;
        }

      reset_timer();

      r2.channel = r1.channel;
      auto channel = findChannel(r1.channel);
      if (!channel)
        {
          r2.status = E_CONNECTION_ID;
          send(r2.ToPacket(IPV4_TCP));
          return;
        }

      TRACEPRINTF (t, 8, "DISCONNECT_REQUEST");

      closeChannel(channel);

      r2.status = 0;
      send(r2.ToPacket(IPV4_TCP));
      return;
    }

  if (p1.service == CONNECTION_REQUEST)
    {
      EIBnet_ConnectRequest r1;
      EIBnet_ConnectResponse r2;
      // For TCP, the "Route Back" HPAI must be used.
      // See ISO 22510:2019 section 5.2.8.6.2
      r2.daddr = routeBackAddr();
      if (parseEIBnet_ConnectRequest(p1, r1))
        {
          t->TracePacket(2, "unparseable CONNECTION_REQUEST", p1.data);
          return;
        }

      reset_timer();

      if (r1.CRI.size() == 3 && r1.CRI[0] == TUNNEL_CONNECTION)
        {
          int newChannelID = getFreeChannelID();
          if (newChannelID < 0)
          {
            TRACEPRINTF (t, 8, "Out of channel IDs");
            r2.status = E_NO_MORE_CONNECTIONS;
            send(r2.ToPacket(IPV4_TCP));
            return;
          }

          if (r1.CRI[1] == TUNNEL_LINKLAYER)
            {
              LinkConnectClientPtr link = LinkConnectClientPtr(new LinkConnectClient(std::dynamic_pointer_cast<TcpTunServer>(parent->shared_from_this()), parent->tunnel_cfg, t));

              auto chan = std::make_shared<TunChannel>(shared_from_this(), newChannelID);
              auto service = std::make_shared<TunServiceLinkLayer>(chan, static_cast<Router &>(parent->router), link);
              chan->setService(service);

              link->set_driver(service);

              if (!link->setup())
                {
                  TRACEPRINTF (t, 8, "Tunnel CONNECTION_REQ link setup failed");
                  r2.status = E_NO_MORE_CONNECTIONS;
                  send(r2.ToPacket(IPV4_TCP));
                  chan->stop(true);
                  return;
                }

              if (!static_cast<Router &>(parent->router).registerLink(link, true))
                {
                  TRACEPRINTF (t, 8, "Tunnel CONNECTION_REQ registering link failed");
                  r2.status = E_NO_MORE_CONNECTIONS;
                  send(r2.ToPacket(IPV4_TCP));
                  chan->stop(true);
                  return;
                }

              // Allocate an address
              if (!service->allocAddress())
                {
                  TRACEPRINTF (t, 8, "Tunnel CONNECTION_REQ no free addresses");
                  r2.status = E_NO_MORE_CONNECTIONS;
                  send(r2.ToPacket(IPV4_TCP));
                  chan->stop(true);
                  return;
                }

              if (openChannel(chan))
                {
                  r2.CRD.resize(3);
                  r2.CRD[0] = TUNNEL_CONNECTION;
                  TRACEPRINTF (t, 8, "Tunnel CONNECTION_REQ with %s", FormatEIBAddr(service->knxaddr));
                  r2.CRD[1] = (service->knxaddr >> 8) & 0xFF;
                  r2.CRD[2] = (service->knxaddr >> 0) & 0xFF;
                  r2.status = E_NO_ERROR;
                  r2.channel = chan->channelID;
                }
              else
                r2.status = E_TUNNELING_LAYER;
            }
          else if (r1.CRI[1] == TUNNEL_BUSMONITOR)
            {
              r2.CRD.resize(3);
              r2.CRD[0] = TUNNEL_CONNECTION;
              r2.CRD[1] = 0;
              r2.CRD[2] = 0;
              auto chan = std::make_shared<TunChannel>(shared_from_this(), newChannelID);
              chan->setService(std::make_shared<TunServiceBusMonitor>(chan, static_cast<Router &>(parent->router)));
              if (openChannel(chan))
                {
                  r2.status = E_NO_ERROR;
                  r2.channel = chan->channelID;
                }
              else
                r2.status = E_TUNNELING_LAYER;
            }
          else
            {
              r2.status = E_TUNNELING_LAYER;
              TRACEPRINTF (t, 8, "bad CONNECTION_REQ: [1] x%02x", r1.CRI[1]);
              send(r2.ToPacket(IPV4_TCP));
              return;
            }
        }
      else if (r1.CRI.size() == 1 && r1.CRI[0] == DEVICE_MGMT_CONNECTION)
        {
          r2.CRD.resize(1);
          r2.CRD[0] = DEVICE_MGMT_CONNECTION;
          TRACEPRINTF (t, 8, "Tunnel CONNECTION_REQ, no addr (mgmt)");

          int newChannelID = getFreeChannelID();
          if (newChannelID < 0)
          {
            TRACEPRINTF (t, 8, "Out of channel IDs");
            r2.status = E_NO_MORE_CONNECTIONS;
            send(r2.ToPacket(IPV4_TCP));
            return;
          }

          auto chan = std::make_shared<TunChannel>(shared_from_this(), newChannelID);
          chan->setService(std::make_shared<TunServiceConfig>(chan));

          if (openChannel(chan))
            {
              r2.status = E_NO_ERROR;
              r2.channel = chan->channelID;
            }
          else
            r2.status = E_TUNNELING_LAYER;
        }
      else
        {
          TRACEPRINTF (t, 8, "bad CONNECTION_REQ: size %d, [0] x%02x", r1.CRI.size(), r1.CRI[0]);
          r2.status = E_CONNECTION_TYPE;
        }

      send(r2.ToPacket(IPV4_TCP));
      return;
    }

  if (p1.service == TUNNEL_REQUEST)
    {
      EIBnet_TunnelRequest r1;
      EIBnet_TunnelACK r2;
      if (parseEIBnet_TunnelRequest(p1, r1))
        {
          t->TracePacket(2, "unparseable TUNNEL_REQUEST", p1.data);
          return;
        }

      reset_timer();

      auto channel = findChannel(r1.channel);
      if (!channel)
        {
          TRACEPRINTF (t, 8, "TUNNEL_REQUEST on unknown channel %d", r1.channel);
          r2.status = E_CONNECTION_ID;
          send(r2.ToPacket(IPV4_TCP));
          return;
        }

      channel->receiveTunnelRequest(r1);

      return;
    }

  if (p1.service == DEVICE_CONFIGURATION_REQUEST)
    {
      EIBnet_ConfigRequest r1;
      EIBnet_ConfigACK r2;
      if (parseEIBnet_ConfigRequest(p1, r1))
        {
          t->TracePacket(2, "unparseable DEVICE_CONFIGURATION_REQUEST", p1.data);
          return;
        }

      reset_timer();

      auto channel = findChannel(r1.channel);
      if (!channel)
        {
          TRACEPRINTF (t, 8, "DEVICE_CONFIGURATION_REQUEST on unknown channel %d", r1.channel);
          r2.status = E_CONNECTION_ID;
          send(r2.ToPacket(IPV4_TCP));
          return;
        }

      TRACEPRINTF (t, 8, "CONFIG_REQ on channel %d",r1.channel);

      channel->receiveConfigRequest(r1);

      return;
    }

  TRACEPRINTF (t, 8, "Unexpected service type: %04x", p1.service);
}

TcpTunServer::TcpTunServer(BaseRouter& r, IniSectionPtr& s)
  : NetServerBase(r,s)
  , tunnel_cfg(s->sub("tunnel",false))
{
  t->setAuxName("tcptunsrv");
}

TcpTunServer::~TcpTunServer()
{
  if (fd >= 0)
    close(fd);
}

bool
TcpTunServer::setup()
{
  if (!Server::setup())
    return false;
  port = cfg->value("port", 3671);
  path = cfg->value("path", "");
  keepalive = cfg->value("heartbeat-timeout", CONNECTION_ALIVE_TIME);
  ignore_when_systemd = cfg->value("systemd-ignore", (port == 3671 && path == ""));

  /* Check that we have client addresses. */
  if (!static_cast<Router&>(router).hasClientAddrs())
    return false;
  /* set up a temporary fake tunnel stack to test the arguments early. */
  if (!static_cast<Router &>(router).checkStack(tunnel_cfg))
    return false;

  return true;
}

void
TcpTunServer::start()
{
  int reuse = 1;

  if (ignore_when_systemd && static_cast<Router &>(router).using_systemd)
    {
      ignore = true;
      stopped(true);
      return;
    }

  if (path == "")
    {
      // TCP
      struct sockaddr_in addr;

      TRACEPRINTF (t, 8, "OpenInetSocket %d", port);
      memset(&addr, 0, sizeof (addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      addr.sin_addr.s_addr = htonl(INADDR_ANY);

      fd = socket(AF_INET, SOCK_STREAM, 0);
      if (fd == -1)
        {
          ERRORPRINTF (t, E_ERROR | 149, "OpenInetSocket %d: socket: %s", port, strerror(errno));
          goto ex1;
        }

      setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse));

      if (bind(fd, (struct sockaddr *) &addr, sizeof (addr)) == -1)
        {
          ERRORPRINTF (t, E_ERROR | 150, "OpenInetSocket %d: bind: %s", port, strerror(errno));
          goto ex2;
        }
    }
  else
    {
      // Unix domain socket
      struct sockaddr_un addr;

      TRACEPRINTF (t, 8, "OpenUnixSocket '%s'", path);
      memset(&addr, 0, sizeof (addr));
      addr.sun_family = AF_LOCAL;
      strncpy(addr.sun_path, path.c_str(), sizeof (addr.sun_path) - 1);

      fd = socket(AF_LOCAL, SOCK_STREAM, 0);
      if (fd == -1)
        {
          ERRORPRINTF (t, E_ERROR | 151, "OpenUnixSocket %d: socket: %s", port, strerror(errno));
          goto ex1;
        }

      if (bind(fd, (struct sockaddr *) &addr, sizeof (addr)) == -1)
        {
          /*
           * dead file?
           */
          if (errno == EADDRINUSE)
            {
              if (connect(fd, (struct sockaddr *) &addr, sizeof (addr)) == 0)
                {
ex:
                  ERRORPRINTF (t, E_ERROR | 152, "OpenLocalSocket %s: bind: %s", path, strerror(errno));
                  goto ex2;
                }
              else if (errno == ECONNREFUSED)
                {
                  ::unlink(path.c_str());
                  if (bind(fd, (struct sockaddr *) &addr, sizeof (addr)) == -1)
                    goto ex;
                }
              else
                {
                  ERRORPRINTF (t, E_ERROR | 153, "Existing socket %s: connect: %s", path, strerror(errno));
                  goto ex2;
                }
            }
        }
    }

  if (listen(fd, 10) == -1)
    {
      ERRORPRINTF (t, E_ERROR | 154, "OpenSocket: listen: %s", strerror(errno));
      goto ex2;
    }

  TRACEPRINTF (t, 8, "Socket opened");
  NetServerBase::start();
  return;

ex2:
  close(fd);
  fd = -1;
ex1:
  stop(true);
  return;
}

void
TcpTunServer::setupConnection(int cfd)
{
  int val = 1;
  setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof (val));
}

ClientConnBasePtr
TcpTunServer::createConnection(int cfd)
{
  auto connection = std::shared_ptr<TcpTunConn>(new TcpTunConn(this, ++lastConnectionID, cfd));

  return connection;
}

void
TcpTunServer::stop(bool err)
{
  if (fd >= 0)
    {
      close(fd);
      fd = -1;
    }
  NetServerBase::stop(err);
}

TcpTunSystemdServer::TcpTunSystemdServer(BaseRouter& r, IniSectionPtr& s, int systemd_fd)
  : TcpTunServer(r,s)
{
  t->setAuxName("systemd_tcptunsrv");
  fd = systemd_fd;
}

void
TcpTunSystemdServer::start()
{
  TRACEPRINTF (t, 8, "OpenSystemdSocket %d", fd);
  if (fd < 0)
    {
      stopped(true);
      return;
    }

  if (listen(fd, 10) == -1)
    {
      ERRORPRINTF (t, E_ERROR | 148, "OpenSystemdSocket: listen: %s", strerror(errno));
      TcpTunServer::stop(true);
      return;
    }

  TRACEPRINTF (t, 8, "SystemdSocket %d opened", fd);
  NetServerBase::start();
}

void
TcpTunSystemdServer::stop(bool err)
{
  TcpTunServer::stop(err);
}

TcpTunSystemdServer::~TcpTunSystemdServer()
{
  if (fd >= 0)
    {
      close(fd);
      fd = -1;
    }
}
