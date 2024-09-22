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

#ifndef TCPTUN_SERVER_H
#define TCPTUN_SERVER_H

#include "eibnetip.h"
#include "server.h"
#include "client.h"

class TunChannel;
using TunChannelPtr = std::shared_ptr<TunChannel>;

class TcpTunConn;
using TcpTunConnPtr = std::shared_ptr<TcpTunConn>;

class TcpTunServer;
using TcpTunServerPtr = std::shared_ptr<TcpTunServer>;

/** Driver for tunnels */
class TcpTunConn : public ClientConnectionBase, public std::enable_shared_from_this<TcpTunConn>
{
public:
  TcpTunConn(TcpTunServer *parent, uint32_t connectionID, int fd);
  virtual ~TcpTunConn();
  bool setup() override;
  void start() override;
  void stop(bool err) override;

  TracePtr t;
  TcpTunServer *parent;

  ev::timer timeout;
  void timeout_cb(ev::timer &w, int revents);
  void reset_timer();

  size_t read_cb(uint8_t *buf, size_t len);
  void error_cb();

  void handlePacket(const EIBNetIPPacket &p1);

  // Return the next free channel ID, or -1 if no channels are available
  int getFreeChannelID();
  bool openChannel(const TunChannelPtr& channel);
  void closeChannel(const TunChannelPtr& channel);
  // Return the channel with ID channelID or nullptr if there is no such channel
  TunChannelPtr findChannel(uint8_t channelID);

  void send(const EIBNetIPPacket& p);

protected:
  uint32_t connectionID;

  SendBuf sendbuf;
  RecvBuf recvbuf;
  int fd;

  uint8_t lastChannelID = 0;
  std::map<uint8_t, TunChannelPtr> channels;
};

SERVER_(TcpTunServer,NetServerBase,tcptunsrv)
{
  friend class TcpTunConn;

public:
  TcpTunServer(BaseRouter& r, IniSectionPtr& s);
  virtual ~TcpTunServer();

  bool setup() override;
  void start() override;
  void stop(bool err) override;

  ClientConnBasePtr createConnection(int cfd) override;

protected:
  void setupConnection(int cfd) override;

private:
  uint32_t lastConnectionID = 0;

  /** config */
  uint16_t port;
  std::string path;
  ev::tstamp keepalive;
  IniSectionPtr tunnel_cfg;
};

class TcpTunSystemdServer : public TcpTunServer
{
public:
  TcpTunSystemdServer(BaseRouter& r, IniSectionPtr& s, int systemd_fd);
  ~TcpTunSystemdServer() override;

  void start() override;
  void stop(bool err) override;
};

#endif

/** @} */
