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

#include "tunchannel.h"
#include "tcptunserver.h"

TunChannel::TunChannel(const TcpTunConnPtr& connection, uint8_t channelID)
  : t(TracePtr(new Trace(*connection->t)))
  , connection(connection)
  , channelID(channelID)
{
  char buf[10];
  snprintf(buf, sizeof(buf), "-%02x", channelID);
  t->setAuxName(connection->t->auxname + buf);
}

TunChannel::~TunChannel()
{
  TRACEPRINTF (t, 8, "Close TunChannel");
}

void TunChannel::setService(const TunServicePtr& service)
{
  if (this->service)
    {
      TRACEPRINTF (t, 8, "Service is already set");
      return;
    }

  TRACEPRINTF (t, 8, "Setting service");

  this->service = service;
}

bool TunChannel::setupChannel()
{
  if (!this->service)
    {
      TRACEPRINTF (t, 8, "Service not set");
      return false;
    }

  return service->setupService();
}

void TunChannel::start()
{
  service->start();
}

void TunChannel::stop(bool err)
{
  service->stop(err);
}

void TunChannel::sendTunnelRequest(const CArray& cemi)
{
  auto connection = this->connection.lock();
  if (!connection)
    {
      TRACEPRINTF (t, 8, "Attempt to send tunnel request without a connection");
      return;
    }

  EIBnet_TunnelRequest r;
  r.channel = channelID;
  r.seqno = sno;
  r.CEMI = cemi;
  connection->send(r.ToPacket(IPV4_TCP));
  sno = (sno + 1) & 0xff;
}

void TunChannel::sendConfigRequest(const CArray& cemi)
{
  auto connection = this->connection.lock();
  if (!connection)
    {
      TRACEPRINTF (t, 8, "Attempt to send config request without a connection");
      return;
    }

  EIBnet_ConfigRequest r;
  r.channel = channelID;
  r.seqno = sno;
  r.CEMI = cemi;
  connection->send(r.ToPacket(IPV4_TCP));
  sno = (sno + 1) & 0xff;
}

void TunChannel::receiveTunnelRequest(EIBnet_TunnelRequest &r1)
{
  // On UDP, the sequence number would be checked here, but for TCP the sequence
  // number is not checked. See ISO 22510:2019 section 5.2.5.3.4
  // if (rno != r1.seqno)
  //   {
  //     TRACEPRINTF (t, 8, "Wrong tunnel sequence number %d<->%d",
  //                  r1.seqno, rno);
  //   }
  rno++;

  if (!service)
    {
      TRACEPRINTF (t, 8, "No tunnel service set");
      return;
    }

  uint8_t status = service->handleTunnelRequest(r1);

  // Note: There are no TUNNELLING_ACKs with TCP, so no way to return the status
  if (status)
    TRACEPRINTF (t, 8, "TCP TUNNELLING_REQUEST failed (%d)", status);
}

void TunChannel::receiveConfigRequest(EIBnet_ConfigRequest &r1)
{
  // On UDP, the sequence number would be checked here, but for TCP the sequence
  // number is not checked. See ISO 22510:2019 section 5.2.5.3.4
  // if (rno != r1.seqno)
  //   {
  //     TRACEPRINTF (t, 8, "Wrong config sequence number %d<->%d",
  //                  r1.seqno, rno);
  //   }
  rno++;

  if (!service)
    {
      TRACEPRINTF (t, 8, "No tunnel service set");
      return;
    }

  uint8_t status = service->handleConfigRequest(r1);

  // Note: There are no DEVICE_CONFIGURATION_ACKs with TCP, so no way to return
  // the status
  if (status)
    TRACEPRINTF (t, 8, "TCP DEVICE_CONFIGURATION_REQUEST failed (%d)", status);
}

TunService::TunService(const TunChannelPtr& channel)
  : t(channel->t)
  , channel(channel)
{
}

TunService::~TunService()
{
}

// This method is overwritten on channel classes which expect to receive
// TUNNELLING_REQUEST packets
ErrorCode TunService::handleTunnelRequest(EIBnet_TunnelRequest &r1)
{
  TRACEPRINTF (t, 8, "Got unexpected TUNNELLING_REQUEST");
  return E_TUNNELING_LAYER;
}

// This method is overwritten on channel classes which expect to receive
// DEVICE_CONFIGURATION_REQUEST packets
ErrorCode TunService::handleConfigRequest(EIBnet_ConfigRequest &r1)
{
  TRACEPRINTF (t, 8, "Got unexpected DEVICE_CONFIGURATION_REQUEST");
  return E_KNX_CONNECTION;
}

TunServiceLinkLayer::TunServiceLinkLayer(const TunChannelPtr& channel, Router& router, LinkConnectClientPtr c)
  : SubDriver(c)
  , TunService(channel)
  , router(router)
{
  SubDriver::t->setAuxName(channel->t->auxname);
}

TunServiceLinkLayer::~TunServiceLinkLayer()
{
}

bool TunServiceLinkLayer::setupService()
{
  addAddress(knxaddr);

  return true;
}

bool TunServiceLinkLayer::setup()
{
  if (!SubDriver::setup())
    return false;

  return true;
}

void TunServiceLinkLayer::start()
{
  SubDriver::start();
}

void TunServiceLinkLayer::stop(bool err)
{
  freeAddress();

  auto c = std::dynamic_pointer_cast<LinkConnect>(conn.lock());
  if (c != nullptr)
    router.unregisterLink(c);

  SubDriver::stop(err);
}

void TunServiceLinkLayer::send_L_Data (LDataPtr l)
{
  auto channel_ptr = this->channel.lock();
  if (channel_ptr)
    channel_ptr->sendTunnelRequest(L_Data_ToCEMI(0x29, l));

  send_Next();
}

ErrorCode TunServiceLinkLayer::handleTunnelRequest(EIBnet_TunnelRequest &r1)
{
  TRACEPRINTF (t, 8, "TUNNEL_REQ");

  LDataPtr c = CEMI_to_L_Data(r1.CEMI, t);
  if (!c)
    return E_DATA_CONNECTION;

  if (c->source_address == 0)
    c->source_address = knxaddr;

  if (r1.CEMI[0] == 0x11)
    {
      // Send L_Data.con message
      auto channel_ptr = this->channel.lock();
      if (channel_ptr)
        channel_ptr->sendTunnelRequest(L_Data_ToCEMI(0x2E, c));
    }

  if (r1.CEMI[0] == 0x11 || r1.CEMI[0] == 0x29)
    {
      recv_L_Data(std::move(c));
      return E_NO_ERROR;
    }
  else
    {
      TRACEPRINTF (t, 8, "Wrong leader x%02x", r1.CEMI[0]);
      return E_TUNNELING_LAYER;
    }
}

bool TunServiceLinkLayer::allocAddress()
{
  // Channel can have only one address
  if (knxaddr)
    return false;

  knxaddr = router.get_client_addr(t);
  if (!knxaddr)
    // Out of addresses
    return false;

  t->setAuxName(FormatEIBAddr(knxaddr));

  return true;
}

void TunServiceLinkLayer::freeAddress()
{
  if (!knxaddr)
    return;

  router.release_client_addr(knxaddr);
  knxaddr = 0;
  t->setAuxName("no-addr");
}

TunServiceBusMonitor::TunServiceBusMonitor(const TunChannelPtr& channel, Router& router)
  : TunService(channel)
  , L_Busmonitor_CallBack(channel->t->name)
  , router(router)
{
}

TunServiceBusMonitor::~TunServiceBusMonitor()
{
}

bool TunServiceBusMonitor::setupService()
{
  if (!router.registerVBusmonitor(this))
    return false;

  return true;
}

void TunServiceBusMonitor::start()
{
}

void TunServiceBusMonitor::stop(bool err)
{
  router.deregisterVBusmonitor(this);
}

void TunServiceBusMonitor::send_L_Busmonitor(LBusmonPtr l)
{
  auto channel_ptr = this->channel.lock();
  if (channel_ptr)
    channel_ptr->sendTunnelRequest(Busmonitor_to_CEMI(0x2B, l, no));

  no++;
}

TunServiceConfig::TunServiceConfig(const TunChannelPtr& channel)
  : TunService(channel)
{
}

TunServiceConfig::~TunServiceConfig()
{
}

bool TunServiceConfig::setupService()
{
  return true;
}

void TunServiceConfig::start()
{
}

void TunServiceConfig::stop(bool err)
{
}

ErrorCode TunServiceConfig::handleConfigRequest(EIBnet_ConfigRequest &r1)
{
  if (r1.CEMI.size() == 0)
    return E_DATA_CONNECTION;

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
          res.resize(1);
          res[0] = 0;
          if (obj == 0 && objno == 0)
            {
              if (prop == 0)
                {
                  res.resize(2);
                  res[0] = 0;
                  res[1] = 0;
                  start = 0;
                }
              else
                count = 0;
            }
          else
            count = 0;
          CEMI.resize(6 + res.size());
          CEMI[0] = 0xFB;
          CEMI[1] = (obj >> 8) & 0xff;
          CEMI[2] = obj & 0xff;
          CEMI[3] = objno;
          CEMI[4] = prop;
          CEMI[5] = ((count & 0x0f) << 4) | (start >> 8);
          CEMI[6] = start & 0xff;
          CEMI.setpart(res, 7);

          auto channel_ptr = this->channel.lock();
          if (channel_ptr)
            channel_ptr->sendConfigRequest(CEMI);

          return E_NO_ERROR;
        }
      else
        return E_DATA_CONNECTION;
    }
  else
    return E_DATA_CONNECTION;
}
