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

/**
 * @file
 * @ingroup KNX_03_08_01
 * KNXnet/IP
 * @{
 */

#ifndef TUNCHANNEL_H
#define TUNCHANNEL_H

#include "eibnetip.h"
#include "link.h"
#include "router.h"

class TunChannel;
using TunChannelPtr = std::shared_ptr<TunChannel>;

class TunService;
using TunServicePtr = std::shared_ptr<TunService>;

class TcpTunConn;
using TcpTunConnPtr = std::shared_ptr<TcpTunConn>;

class TunChannel
{
public:
  TunChannel(const TcpTunConnPtr& connection, uint8_t channelID);
  virtual ~TunChannel();

  void setService(const TunServicePtr& service);

  bool setupChannel();
  void start();
  void stop(bool err);

  void sendTunnelRequest(const CArray& cemi);
  void sendConfigRequest(const CArray& cemi);

  // handle various packets from the connection
  void receiveTunnelRequest(EIBnet_TunnelRequest &r1);
  void receiveConfigRequest(EIBnet_ConfigRequest &r1);

  TracePtr t;
  std::weak_ptr<TcpTunConn> connection;
  TunServicePtr service;
  uint8_t channelID;

  // Sending sequence counter
  uint8_t sno = 0;
  // Receiving sequence counter
  uint8_t rno = 0;
};

class TunService
{
public:
  TunService(const std::shared_ptr<TunChannel>& channel);
  virtual ~TunService();

  virtual bool setupService() = 0;
  virtual void start() = 0;
  virtual void stop(bool err) = 0;

  virtual ErrorCode handleTunnelRequest(EIBnet_TunnelRequest& r1);
  virtual ErrorCode handleConfigRequest(EIBnet_ConfigRequest &r1);

  TracePtr t;
  std::weak_ptr<TunChannel> channel;
};

class TunServiceLinkLayer : public SubDriver, public TunService
{
public:
  using TunService::t;

  TunServiceLinkLayer(const std::shared_ptr<TunChannel>& channel, Router& router, LinkConnectClientPtr c);
  virtual ~TunServiceLinkLayer();

  ErrorCode handleTunnelRequest(EIBnet_TunnelRequest& r1) override;

  bool setupService() override;
  void start() override;
  void stop(bool err) override;

  bool setup() override;

  void send_L_Data(LDataPtr l) override;

  bool allocAddress();
  void freeAddress();

  bool hasAddress(eibaddr_t a) const override
  {
    return knxaddr && knxaddr == a;
  }

  Router& router;
  eibaddr_t knxaddr = 0;
};

class TunServiceBusMonitor : public TunService, public L_Busmonitor_CallBack
{
public:
  TunServiceBusMonitor(const std::shared_ptr<TunChannel>& channel, Router& router);
  virtual ~TunServiceBusMonitor();

  bool setupService() override;
  void start() override;
  void stop(bool err) override;

  void send_L_Busmonitor(LBusmonPtr l) override;

  Router& router;
  int no = 1;
};

class TunServiceConfig : public TunService
{
public:
  TunServiceConfig(const std::shared_ptr<TunChannel>& channel);
  virtual ~TunServiceConfig();

  bool setupService() override;
  void start() override;
  void stop(bool err) override;

  ErrorCode handleConfigRequest(EIBnet_ConfigRequest &r1) override;
};

#endif

/** @} */
