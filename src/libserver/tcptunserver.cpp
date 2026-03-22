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

#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

TcpTunConn::TcpTunConn(TcpTunServerBase *parent, uint32_t connectionID, int fd)
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
      snprintf(str, sizeof(str), "stream-%u", connectionID);
      t->setAuxName(str);
    }
  else
    {
      char addrStr[INET_ADDRSTRLEN];
      if (inet_ntop(localSocketAddress.sin_family, &localSocketAddress.sin_addr, addrStr, sizeof(addrStr)))
        {
          char addrPortStr[INET_ADDRSTRLEN + 6];
          snprintf(addrPortStr, sizeof(addrPortStr), "%s:%hu", addrStr, localSocketAddress.sin_port);
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

  // If this connection has an active secure session, wrap in SECURE_WRAPPER
  if (secure_session_id != 0)
    {
      auto wrapped = parent->ip_secure.wrapSecure(secure_session_id,
                                                    data.data(), data.size());
      if (wrapped.empty())
        {
          TRACEPRINTF(t, 2, "IP Secure: wrap failed for session %d", secure_session_id);
          return;
        }
      t->TracePacket(0, "TCP send (secure)", wrapped.size(), wrapped.data());
      if (fd >= 0)
        sendbuf.write(wrapped.data(), wrapped.size());
      return;
    }

  t->TracePacket(0, "TCP send", data.size(), data.data());

  if (fd >= 0)
    sendbuf.write(data.data(), data.size());
}

void
TcpTunConn::handlePacket(const EIBNetIPPacket &p1)
{
  // KNX IP Secure: handle SESSION_REQUEST (unencrypted)
  if (p1.service == SESSION_REQUEST_SVC && parent->ip_secure.isEnabled())
    {
      CArray raw = p1.ToPacket();
      auto resp = parent->ip_secure.handleSessionRequest(raw.data(), raw.size());
      if (resp.empty())
        {
          TRACEPRINTF(t, 2, "IP Secure: SESSION_REQUEST rejected");
          return;
        }
      // Extract session ID from response (bytes 6-7)
      secure_session_id = ((uint16_t)resp[6] << 8) | resp[7];
      TRACEPRINTF(t, 2, "IP Secure: new session %d", secure_session_id);
      t->TracePacket(0, "TCP send SESSION_RESPONSE", resp.size(), resp.data());
      if (fd >= 0)
        sendbuf.write(resp.data(), resp.size());
      reset_timer();
      return;
    }

  // KNX IP Secure: unwrap SECURE_WRAPPER
  if (p1.service == SECURE_WRAPPER_SVC && secure_session_id != 0)
    {
      CArray raw = p1.ToPacket();
      uint16_t sid = 0;
      auto inner = parent->ip_secure.unwrapSecure(raw.data(), raw.size(), sid);
      if (inner.empty())
        {
          TRACEPRINTF(t, 2, "IP Secure: SECURE_WRAPPER decrypt/verify failed");
          return;
        }
      if (sid != secure_session_id)
        {
          TRACEPRINTF(t, 2, "IP Secure: session ID mismatch %d != %d", sid, secure_session_id);
          return;
        }

      t->TracePacket(0, "IP Secure unwrapped", inner.size(), inner.data());
      reset_timer();

      // Parse the inner KNXnet/IP frame
      CArray inner_arr(inner.data(), inner.size());
      std::unique_ptr<EIBNetIPPacket> inner_pkt(
        EIBNetIPPacket::fromPacket(inner_arr, routeBackAddr(), IPV4_TCP));
      if (!inner_pkt)
        {
          TRACEPRINTF(t, 2, "IP Secure: cannot parse inner packet");
          return;
        }

      // Handle SESSION_AUTHENTICATE
      if (inner_pkt->service == SESSION_AUTHENTICATE_SVC)
        {
          auto* session = parent->ip_secure.findSession(secure_session_id);
          bool ok = parent->ip_secure.handleSessionAuthenticate(
            secure_session_id, inner.data(), inner.size());
          if (ok)
            {
              TRACEPRINTF(t, 2, "IP Secure: session %d authenticated (user %d)",
                          secure_session_id, session ? session->user_id : 0);
              auto status = parent->ip_secure.buildSessionStatus(
                secure_session_id, STATUS_AUTH_SUCCESS);
              if (!status.empty())
                {
                  t->TracePacket(0, "TCP send SESSION_STATUS (success)", status.size(), status.data());
                  if (fd >= 0)
                    sendbuf.write(status.data(), status.size());
                }
            }
          else
            {
              TRACEPRINTF(t, 2, "IP Secure: session %d auth FAILED", secure_session_id);
              auto status = parent->ip_secure.buildSessionStatus(
                secure_session_id, STATUS_AUTH_FAILED);
              if (!status.empty() && fd >= 0)
                sendbuf.write(status.data(), status.size());
              parent->ip_secure.removeSession(secure_session_id);
              secure_session_id = 0;
            }
          return;
        }

      // Handle SESSION_STATUS (keepalive/close)
      if (inner_pkt->service == SESSION_STATUS_SVC_ID)
        {
          if (inner.size() >= 7)
            {
              uint8_t status = inner[6];
              if (status == STATUS_CLOSE)
                {
                  TRACEPRINTF(t, 2, "IP Secure: client closed session %d", secure_session_id);
                  auto resp = parent->ip_secure.buildSessionStatus(
                    secure_session_id, STATUS_CLOSE);
                  if (!resp.empty() && fd >= 0)
                    sendbuf.write(resp.data(), resp.size());
                  parent->ip_secure.removeSession(secure_session_id);
                  secure_session_id = 0;
                  stop(false);
                }
              else if (status == STATUS_KEEPALIVE)
                {
                  auto* session = parent->ip_secure.findSession(secure_session_id);
                  if (session && session->state != SecureSession::AUTHENTICATED)
                    {
                      // Keepalive on unauthenticated session
                      auto resp = parent->ip_secure.buildSessionStatus(
                        secure_session_id, STATUS_UNAUTHENTICATED);
                      if (!resp.empty() && fd >= 0)
                        sendbuf.write(resp.data(), resp.size());
                      parent->ip_secure.removeSession(secure_session_id);
                      secure_session_id = 0;
                    }
                  // Authenticated keepalive is a no-op (timer already reset)
                }
            }
          return;
        }

      // Check authentication before forwarding any other service
      auto* session = parent->ip_secure.findSession(secure_session_id);
      if (!session || session->state != SecureSession::AUTHENTICATED)
        {
          TRACEPRINTF(t, 2, "IP Secure: rejecting service in unauthenticated session");
          auto resp = parent->ip_secure.buildSessionStatus(
            secure_session_id, STATUS_UNAUTHENTICATED);
          if (!resp.empty() && fd >= 0)
            sendbuf.write(resp.data(), resp.size());
          parent->ip_secure.removeSession(secure_session_id);
          secure_session_id = 0;
          return;
        }

      // Forward the unwrapped inner packet to normal handler
      handlePacket(*inner_pkt);
      return;
    }

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
              LinkConnectClientPtr link = LinkConnectClientPtr(new LinkConnectClient(std::dynamic_pointer_cast<TcpTunServerBase>(parent->shared_from_this()), parent->tunnel_cfg, t));

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
          chan->setService(std::make_shared<TunServiceConfig>(chan, parent->maxAPDULength));

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

  if (p1.service == DESCRIPTION_REQUEST)
    {
      EIBnet_DescriptionRequest r1;
      EIBnet_DescriptionResponse r2;
      DIB_service_Entry d;
      if (parseEIBnet_DescriptionRequest(p1, r1))
        {
          t->TracePacket(2, "unparseable DESCRIPTION_REQUEST", p1.data);
          return;
        }

      TRACEPRINTF (t, 8, "DESCRIBE");

      Router& router = static_cast<Router &>(parent->router);
      r2.KNXmedium = M_TP1;
      r2.devicestatus = 0;
      r2.individual_addr = router.addr;
      r2.installid = 0;
      inet_pton(AF_INET, "224.0.23.12", &r2.multicastaddr);
      strncpy((char *) r2.name, router.servername.c_str(), sizeof(r2.name) - 1);
      // 03_08_02 Core v01.06.02, §7.5.4.3 Table 3
      // version 2 = KNXnet/IP v2 with TCP support
      d.version = 2;
      d.family = SF_CORE;
      r2.services.push_back(d);
      d.family = SF_DEVICE_MANAGEMENT;
      r2.services.push_back(d);
      d.family = SF_TUNNELLING;
      r2.services.push_back(d);
      send(r2.ToPacket(IPV4_TCP));
      return;
    }

  if (p1.service == TUNNEL_FEATURE_GET)
    {
      // 03_08_04 Tunnelling v01.07.01, §5.4.8: TUNNELLING_FEATURE_GET frame
      // Body: connection header (4) + featureID (1) + reserved (1)
      if (p1.data.size() < 6 || p1.data[0] != 4)
        {
          t->TracePacket(2, "unparseable TUNNEL_FEATURE_GET", p1.data);
          return;
        }

      reset_timer();

      uint8_t chanID = p1.data[1];
      uint8_t seqno = p1.data[2];
      // p1.data[3] is reserved
      uint8_t featureID = p1.data[4];

      auto channel = findChannel(chanID);
      if (!channel)
        {
          TRACEPRINTF (t, 8, "TUNNEL_FEATURE_GET on unknown channel %d", chanID);
          return;
        }

      TRACEPRINTF (t, 8, "TUNNEL_FEATURE_GET ch=%d feat=%d", chanID, featureID);

      // Build TUNNEL_FEATURE_RESPONSE (§5.4.9)
      // resp.data layout: connHdr[0..3] + featureID[4] + returnCode[5] + value[6..]
      // CArray::resize() zero-initializes new elements
      EIBNetIPPacket resp;
      resp.service = TUNNEL_FEATURE_RESPONSE;
      switch (featureID)
        {
        case IF_SUPPORTED_EMI_TYPE: // 2 bytes, bitfield (bit0=EMI1, bit1=EMI2, bit2=cEMI)
          resp.data.resize(8);
          resp.data[4] = featureID;
          resp.data[5] = FR_NO_ERROR;
          resp.data[7] = 0x04; // cEMI only
          break;
        case IF_DEVICE_DESCRIPTOR_TYPE0: // mask version 0701h
          resp.data.resize(8);
          resp.data[4] = featureID;
          resp.data[5] = FR_NO_ERROR;
          resp.data[6] = 0x07;
          resp.data[7] = 0x01;
          break;
        case IF_BUS_CONNECTION_STATUS:
          {
            resp.data.resize(7);
            resp.data[4] = featureID;
            resp.data[5] = FR_NO_ERROR;
            auto& router = static_cast<Router &>(parent->router);
            resp.data[6] = router.isIdle() ? 0x00 : 0x01;
          }
          break;
        case IF_KNX_MANUFACTURER_CODE:
          resp.data.resize(8);
          resp.data[4] = featureID;
          resp.data[5] = FR_NO_ERROR;
          resp.data[6] = (parent->manufacturerCode >> 8) & 0xFF;
          resp.data[7] = parent->manufacturerCode & 0xFF;
          break;
        case IF_ACTIVE_EMI_TYPE:
          resp.data.resize(7);
          resp.data[4] = featureID;
          resp.data[5] = FR_NO_ERROR;
          resp.data[6] = 0x04; // cEMI
          break;
        case IF_INDIVIDUAL_ADDRESS:
          {
            resp.data.resize(8);
            resp.data[4] = featureID;
            resp.data[5] = FR_NO_ERROR;
            auto *llService = dynamic_cast<TunServiceLinkLayer *>(channel->service.get());
            eibaddr_t addr = llService ? llService->knxaddr : 0;
            resp.data[6] = (addr >> 8) & 0xFF;
            resp.data[7] = addr & 0xFF;
          }
          break;
        case IF_MAX_APDU_LENGTH:
          resp.data.resize(8);
          resp.data[4] = featureID;
          resp.data[5] = FR_NO_ERROR;
          resp.data[6] = (parent->maxAPDULength >> 8) & 0xFF;
          resp.data[7] = parent->maxAPDULength & 0xFF;
          break;
        case IF_FEATURE_INFO_ENABLE:
          resp.data.resize(7);
          resp.data[4] = featureID;
          resp.data[5] = FR_NO_ERROR;
          resp.data[6] = channel->featureInfoEnabled ? 0x01 : 0x00;
          break;
        default:
          resp.data.resize(6);
          resp.data[4] = featureID;
          resp.data[5] = FR_ADDRESS_VOID;
          break;
        }
      resp.data[0] = 4; // connection header length
      resp.data[1] = chanID;
      resp.data[2] = seqno;
      resp.data[3] = 0; // reserved
      send(resp);
      return;
    }

  if (p1.service == TUNNEL_FEATURE_SET)
    {
      // 03_08_04 Tunnelling v01.07.01, §5.4.10: TUNNELLING_FEATURE_SET frame
      // Body: connection header (4) + featureID (1) + reserved (1) + value (n)
      if (p1.data.size() < 6 || p1.data[0] != 4)
        {
          t->TracePacket(2, "unparseable TUNNEL_FEATURE_SET", p1.data);
          return;
        }

      reset_timer();

      uint8_t chanID = p1.data[1];
      uint8_t seqno = p1.data[2];
      // p1.data[3] is reserved
      uint8_t featureID = p1.data[4];

      auto channel = findChannel(chanID);
      if (!channel)
        {
          TRACEPRINTF (t, 8, "TUNNEL_FEATURE_SET on unknown channel %d", chanID);
          return;
        }

      TRACEPRINTF (t, 8, "TUNNEL_FEATURE_SET ch=%d feat=%d", chanID, featureID);

      // Build TUNNEL_FEATURE_RESPONSE (§5.4.9)
      // resp.data layout: connHdr[0..3] + featureID[4] + returnCode[5] + value[6..]
      EIBNetIPPacket resp;
      resp.service = TUNNEL_FEATURE_RESPONSE;

      switch (featureID)
        {
        case IF_SUPPORTED_EMI_TYPE:
        case IF_DEVICE_DESCRIPTOR_TYPE0:
        case IF_BUS_CONNECTION_STATUS:
        case IF_KNX_MANUFACTURER_CODE:
        case IF_ACTIVE_EMI_TYPE:
        case IF_INDIVIDUAL_ADDRESS: // read-only (no KNX Secure)
        case IF_MAX_APDU_LENGTH:
          {
            // Echo value from request, capped to 2 bytes (no feature uses more)
            size_t valueLen = p1.data.size() - 6;
            if (valueLen > 2)
              valueLen = 2;
            resp.data.resize(6 + valueLen);
            resp.data[4] = featureID;
            resp.data[5] = FR_ACCESS_READ_ONLY;
            for (size_t i = 0; i < valueLen; i++)
              resp.data[6 + i] = p1.data[6 + i];
          }
          break;
        case IF_FEATURE_INFO_ENABLE: // writable
          {
            if (p1.data.size() < 7)
              {
                resp.data.resize(6);
                resp.data[4] = featureID;
                resp.data[5] = FR_DATA_TYPE_CONFLICT;
                break;
              }
            uint8_t val = p1.data[6];
            if (val > 0x01)
              {
                resp.data.resize(7);
                resp.data[4] = featureID;
                resp.data[5] = FR_DATA_VOID;
                resp.data[6] = val;
                break;
              }
            channel->featureInfoEnabled = (val == 0x01);
            resp.data.resize(7);
            resp.data[4] = featureID;
            resp.data[5] = FR_NO_ERROR;
            resp.data[6] = val;
          }
          break;
        default: // Unknown feature — FR_ADDRESS_VOID, no value (§3.5)
          resp.data.resize(6);
          resp.data[4] = featureID;
          resp.data[5] = FR_ADDRESS_VOID;
          break;
        }

      resp.data[0] = 4; // connection header length
      resp.data[1] = chanID;
      resp.data[2] = seqno;
      resp.data[3] = 0; // reserved
      send(resp);
      return;
    }

  if (p1.service == SEARCH_REQUEST_EXTENDED)
    {
      // 03_08_02 Core v01.06.02, §7.6.3/§7.6.4: SEARCH_REQUEST/RESPONSE_EXTENDED
      // Respond with the same device info as DESCRIPTION_RESPONSE.
      // SRP filtering not implemented — return all DIBs (superset is valid per spec).
      TRACEPRINTF (t, 8, "SEARCH_REQUEST_EXTENDED");

      EIBnet_SearchResponse r2;
      DIB_service_Entry d;
      Router& router = static_cast<Router &>(parent->router);
      r2.KNXmedium = M_TP1;
      r2.devicestatus = 0;
      r2.individual_addr = router.addr;
      r2.installid = 0;
      inet_pton(AF_INET, "224.0.23.12", &r2.multicastaddr);
      strncpy((char *) r2.name, router.servername.c_str(), sizeof(r2.name) - 1);
      // HPAI: route-back for TCP (spec: "only report UDP address", use 0.0.0.0:0)
      memset(&r2.caddr, 0, sizeof(r2.caddr));
      r2.caddr.sin_family = AF_INET;
      // 03_08_02 Core v01.06.02, §7.5.4.3 Table 3
      d.version = 2;
      d.family = SF_CORE;
      r2.services.push_back(d);
      d.family = SF_DEVICE_MANAGEMENT;
      r2.services.push_back(d);
      d.family = SF_TUNNELLING;
      r2.services.push_back(d);
      EIBNetIPPacket pkt = r2.ToPacket(IPV4_TCP);
      pkt.service = SEARCH_RESPONSE_EXTENDED;
      send(pkt);
      return;
    }

  TRACEPRINTF (t, 8, "Unexpected service type: %04x", p1.service);
}

TcpTunServerBase::TcpTunServerBase(BaseRouter& r, IniSectionPtr& s)
  : NetServerBase(r,s)
  , tunnel_cfg(s->sub("tunnel",false))
{
  t->setAuxName("tcptunsrv");
}

TcpTunServerBase::~TcpTunServerBase()
{
  if (fd >= 0)
    close(fd);
}

void
TcpTunServerBase::setupConnection(int cfd)
{
  int val = 1;
  setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof (val));
}

ClientConnBasePtr
TcpTunServerBase::createConnection(int cfd)
{
  auto connection = std::shared_ptr<TcpTunConn>(new TcpTunConn(this, ++lastConnectionID, cfd));

  return connection;
}

void
TcpTunServerBase::stop(bool err)
{
  if (fd >= 0)
    {
      close(fd);
      fd = -1;
    }
  NetServerBase::stop(err);
}

TcpTunServer::TcpTunServer(BaseRouter& r, IniSectionPtr& s)
  : TcpTunServerBase(r,s)
{
  t->setAuxName("tcptunsrv");
}

bool
TcpTunServer::setup()
{
  if (!Server::setup())
    return false;
  port = cfg->value("port", 3671);
  keepalive = cfg->value("heartbeat-timeout", CONNECTION_ALIVE_TIME);
  {
    // 03_05_01 Resources v01.10.01, §4.3.7.1: range 15..254
    // 0 = not configured, auto-detect from hardware driver in start()
    int v = cfg->value("max-apdu-length", -1);
    if (v > 254)
      {
        ERRORPRINTF (t, E_ERROR | 151, "max-apdu-length %d exceeds 254, clamping", v);
        v = 254;
      }
    else if (v >= 0 && v < 15)
      {
        ERRORPRINTF (t, E_ERROR | 152, "max-apdu-length %d below minimum 15, clamping", v);
        v = 15;
      }
    maxAPDULength = (v >= 0) ? v : 0;
  }
  manufacturerCode = cfg->value("manufacturer-code", 0);
  ignore_when_systemd = cfg->value("systemd-ignore", port == 3671);

  // KNX IP Secure configuration
  {
    std::string keyring = cfg->value("keyring", "");
    std::string keyring_pwd = cfg->value("keyring-password", "");
    std::string device_auth = cfg->value("device-auth", "");
    std::string user_pwd = cfg->value("user-password", "");

    if (!keyring.empty())
      {
        if (ip_secure.loadKeyring(keyring, keyring_pwd))
          TRACEPRINTF(t, 2, "IP Secure: loaded keys from keyring %s", keyring.c_str());
        else
          TRACEPRINTF(t, 2, "IP Secure: failed to load keyring %s", keyring.c_str());
      }
    if (!device_auth.empty())
      ip_secure.setDeviceAuthPassword(device_auth);
    if (!user_pwd.empty())
      {
        // Set as user 1 (management) and user 2 (tunnelling)
        ip_secure.setUserPassword(1, user_pwd);
        ip_secure.setUserPassword(2, user_pwd);
      }
    if (ip_secure.isEnabled())
      TRACEPRINTF(t, 2, "IP Secure: enabled for TCP tunnel server");
  }

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

  if (maxAPDULength == 0)
    {
      // 03_05_01 Resources v01.10.01, §4.3.7.1: range 15..254
      unsigned int fl = static_cast<Router &>(router).maxFrameLength();
      maxAPDULength = (fl > 8) ? fl - 8 : 15;
      if (maxAPDULength > 254)
        maxAPDULength = 254;
    }

  if (ignore_when_systemd && static_cast<Router &>(router).using_systemd)
    {
      ignore = true;
      stopped(true);
      return;
    }

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

UnixTunServer::UnixTunServer(BaseRouter& r, IniSectionPtr& s)
  : TcpTunServerBase(r,s)
{
  t->setAuxName("unixtunsrv");
}

bool
UnixTunServer::setup()
{
  if (!Server::setup())
    return false;
  path = cfg->value("path", "");
  keepalive = cfg->value("heartbeat-timeout", CONNECTION_ALIVE_TIME);
  ignore_when_systemd = cfg->value("systemd-ignore", false);

  /* Check that we have client addresses. */
  if (!static_cast<Router&>(router).hasClientAddrs())
    return false;
  /* set up a temporary fake tunnel stack to test the arguments early. */
  if (!static_cast<Router &>(router).checkStack(tunnel_cfg))
    return false;

  return true;
}

void
UnixTunServer::start()
{
  int reuse = 1;

  if (maxAPDULength == 0)
    {
      // 03_05_01 Resources v01.10.01, §4.3.7.1: range 15..254
      unsigned int fl = static_cast<Router &>(router).maxFrameLength();
      maxAPDULength = (fl > 8) ? fl - 8 : 15;
      if (maxAPDULength > 254)
        maxAPDULength = 254;
    }

  if (ignore_when_systemd && static_cast<Router &>(router).using_systemd)
    {
      ignore = true;
      stopped(true);
      return;
    }

  struct sockaddr_un addr;

  TRACEPRINTF (t, 8, "OpenUnixSocket '%s'", path);
  memset(&addr, 0, sizeof (addr));
  addr.sun_family = AF_LOCAL;
  strncpy(addr.sun_path, path.c_str(), sizeof (addr.sun_path) - 1);

  fd = socket(AF_LOCAL, SOCK_STREAM, 0);
  if (fd == -1)
    {
      ERRORPRINTF (t, E_ERROR | 151, "OpenUnixSocket %s: socket: %s", path, strerror(errno));
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
              if (::unlink(path.c_str()) == -1 && errno != ENOENT)
                {
                  ERRORPRINTF (t, E_ERROR | 155, "Existing socket %s: unlink: %s", path, strerror(errno));
                  goto ex2;
                }
              if (bind(fd, (struct sockaddr *) &addr, sizeof (addr)) == -1)
                {
                  ERRORPRINTF (t, E_ERROR | 153, "Existing socket %s: bind: %s", path, strerror(errno));
                  goto ex2;
                }
            }
          else
            {
              ERRORPRINTF (t, E_ERROR | 153, "Existing socket %s: bind: %s", path, strerror(errno));
              goto ex2;
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

TcpTunSystemdServer::TcpTunSystemdServer(BaseRouter& r, IniSectionPtr& s, int systemd_fd)
  : TcpTunServerBase(r,s)
{
  t->setAuxName("systemd_tcptunsrv");
  fd = systemd_fd;
}

void
TcpTunSystemdServer::start()
{
  if (maxAPDULength == 0)
    {
      // 03_05_01 Resources v01.10.01, §4.3.7.1: range 15..254
      unsigned int fl = static_cast<Router &>(router).maxFrameLength();
      maxAPDULength = (fl > 8) ? fl - 8 : 15;
      if (maxAPDULength > 254)
        maxAPDULength = 254;
    }

  TRACEPRINTF (t, 8, "OpenSystemdSocket %d", fd);
  if (fd < 0)
    {
      stopped(true);
      return;
    }

  if (listen(fd, 10) == -1)
    {
      ERRORPRINTF (t, E_ERROR | 148, "OpenSystemdSocket: listen: %s", strerror(errno));
      TcpTunServerBase::stop(true);
      return;
    }

  TRACEPRINTF (t, 8, "SystemdSocket %d opened", fd);
  NetServerBase::start();
}

bool
TcpTunSystemdServer::setup()
{
  if (!Server::setup())
    return false;
  keepalive = cfg->value("heartbeat-timeout", CONNECTION_ALIVE_TIME);

  /* Check that we have client addresses. */
  if (!static_cast<Router&>(router).hasClientAddrs())
    return false;
  /* set up a temporary fake tunnel stack to test the arguments early. */
  if (!static_cast<Router &>(router).checkStack(tunnel_cfg))
    return false;

  return true;
}

void
TcpTunSystemdServer::stop(bool err)
{
  TcpTunServerBase::stop(err);
}

TcpTunSystemdServer::~TcpTunSystemdServer()
{
  if (fd >= 0)
    {
      close(fd);
      fd = -1;
    }
}
