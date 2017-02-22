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
#include "lowlevel.h"
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
  virtual ~Router();

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
  void link_started(const LinkConnectPtr& link);
  void link_stopped(const LinkConnectPtr& link);

  /** register a new link. Must be fully linked and setup() must be OK. */
  bool registerLink(const LinkConnectPtr& link);
  /** unregister a new link */
  bool unregisterLink(const LinkConnectPtr& link);

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
  bool hasAddress (eibaddr_t addr, LinkConnectPtr& link, bool quiet = false);
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
  Factory<LowLevelDriver>& lowlevels;

  bool do_server(ServerPtr &link, IniSection& s, const std::string& servername, bool quiet = false);
  bool do_driver(LinkConnectPtr &link, IniSection& s, const std::string& servername, bool quiet = false);
public:
  /** Look up a filter by name */
  FilterPtr get_filter(const LinkConnectPtr &link, IniSection& s, const std::string& filtername);

  /** Look up a filter by name */
  LowLevelDriver * get_lowlevel(const DriverPtr& parent, IniSection& s, const std::string& lowlevelname);

  /** Create a temporary dummy driver stack to test arguments for filters etc.
   * Testing the calling driver's config args is the caller#s job.
   */
  bool checkStack(IniSection& cfg);

private:
  /** name of our main section */
  std::string main;

  /** create a link */
  LinkConnectPtr setup_link(std::string& name);

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

  /** flag whether some driver is active */
  bool some_running = false;
  /** flag whether new drivers should be active */
  bool want_up = false;
  /** flag whether all drivers are active */
  bool all_running = false;
  /** flag to signal systemd */
  bool running_signal = false;

  /** treat route count 7 as per EIB spec? */
  bool force_broadcast = false;

  /** iterators are evil */
  bool links_changed = false;

public:
  /** allow unparsed tags in the config file? */
  bool unknown_ok = false;
  /** flag whether systemd has passed us any file descriptors */
  bool using_systemd = false;

  bool isIdle() { return !some_running; }
  bool isRunning() { return all_running; }

private:
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
