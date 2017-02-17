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

#ifndef ROUTER_H
#define ROUTER_H

#include <unordered_map>

#include "link.h"
#include "lpdu.h"
#include <ev++.h>

class BaseServer;
class GroupCache;
class LinkConnect;

class LPtr {
public:
  LDataPtr first;
  LinkConnectPtr second;
  LPtr(LDataPtr f, LinkConnectPtr s) {first = std::move(f); second = s; }
};

/** stores a registered busmonitor callback */
typedef struct
{
  L_Busmonitor_CallBack *cb;
} Busmonitor_Info;

typedef struct
{
  CArray data;
  timestamp_t end;
} IgnoreInfo;

class Router : public BaseRouter {
public:
  Router(IniData& d, std::string sn);
  ~Router();

  /** debug output */
  TracePtr t;
  /** my name */
  std::string servername;

  /** the server's own address */
  eibaddr_t addr = 0;

  /** group cache */
  std::shared_ptr<GroupCache> getCache() { return cache; }
  void setCache(std::shared_ptr<GroupCache> cache) { this->cache = cache; }

  /** read and apply settings */
  bool setup();
  /** start up */
  void start();
  /** shut down*/
  void stop();

  /** callbacks from link */
  void link_started(LinkConnectPtr link);
  void link_stopped(LinkConnectPtr link);

  /** register a new link. Must be fully linked and setup() must be OK. */
  bool registerLink(LinkConnectPtr link);
  /** unregister a new link */
  bool unregisterLink(LinkConnectPtr link);

  /** register a busmonitor callback, return true, if successful*/
  bool registerBusmonitor (L_Busmonitor_CallBack * c);
  /** register a vbusmonitor callback, return true, if successful*/
  bool registerVBusmonitor (L_Busmonitor_CallBack * c);

  /** deregister a busmonitor callback, return true, if successful*/
  bool deregisterBusmonitor (L_Busmonitor_CallBack * c);
  /** deregister a vbusmonitor callback, return true, if successful*/
  bool deregisterVBusmonitor (L_Busmonitor_CallBack * c);

  /** Get a free dynamic address */
  eibaddr_t get_client_addr (TracePtr t);
  /** … and release it */
  void release_client_addr (eibaddr_t addr);

  /** check if any interface knows this address. */
  bool hasAddress (eibaddr_t addr, LinkConnectPtr& link);
  /** check if any interface accepts this address.
      'l2' says which interface NOT to check. */
  bool checkAddress (eibaddr_t addr, LinkConnectPtr l2 = nullptr);
  /** check if any interface accepts this group address.
      'l2' says which interface NOT to check. */
  bool checkGroupAddress (eibaddr_t addr, LinkConnectPtr l2 = nullptr);

  /** accept a L_Data frame */
  void recv_L_Data (LDataPtr l, LinkConnect& link);
  /** accept a L_Busmonitor frame */
  void recv_L_Busmonitor (LBusmonPtr l);

private:
  Factory<Server>& servers;
  Factory<Driver>& drivers;
  Factory<Filter>& filters;

  bool do_server(ServerPtr &link, IniSection& s, const std::string& servername, bool quiet = false);
  bool do_driver(LinkConnectPtr &link, IniSection& s, const std::string& servername, bool quiet = false);
public:
  FilterPtr get_filter(LinkConnectPtr link, IniSection& s, const std::string& filtername);

private:
  /** name of our main section */
  std::string main;

  /** create a link */
  LinkBasePtr setup_link(std::string& name);

  /** interfaces */
  std::unordered_map<std::string, LinkConnectPtr> links;

  // libev
  ev::async trigger;
  void trigger_cb (ev::async &w, int revents);
  ev::async mtrigger;
  void mtrigger_cb (ev::async &w, int revents);

  /** buffer queues for receiving from L2 */
  Queue < LPtr > buf;
  Queue < LBusmonPtr > mbuf;
  /** buffer for packets to ignore when repeat flag is set */
  Array < IgnoreInfo > ignore;

  /** Start of address block to assign dynamically to clients */
  eibaddr_t client_addrs_start;
  /** Length of address block to assign dynamically to clients */
  int client_addrs_len = 0;
  int client_addrs_pos;
  std::vector<bool> client_addrs;

  /** busmonitor callbacks */
  Array < Busmonitor_Info > busmonitor;
  /** vbusmonitor callbacks */
  Array < Busmonitor_Info > vbusmonitor;

  /** flag whether we've been started */
  bool running = false;
  /** flag whether we're transitioning states */
  bool switching = false;

  /** treat route count 7 as per EIB spec? */
  bool force_broadcast;

  ev::async cleanup;
  void cleanup_cb (ev::async &w, int revents);
  /** to-be-closed client connections*/
  Queue < LinkBasePtr > cleanup_q;

  /** group cache */
  std::shared_ptr<GroupCache> cache;

  /** parser support */
  bool readaddr (const std::string& addr, eibaddr_t& parsed);
  bool readaddrblock (const std::string& addr, eibaddr_t& parsed, int &len);

};

#endif
