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
class Router;
class RouterHigh;
class RouterLow;

typedef std::shared_ptr<RouterLow> RouterLowPtr;
typedef std::shared_ptr<RouterHigh> RouterHighPtr;

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
  friend class RouterLow;
  friend class RouterHigh;
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

  /** second step, after hitting the global queue */
  void start_();
  void stop_();

  /** last step, after hitting the global queue */
  void started();
  void stopped();
  void errored();

  /** callbacks from LinkConnect */
  void linkStateChanged(const LinkConnectPtr& link);

  /** register a new link. Must be fully linked and setup() must be OK. */
  bool registerLink(const LinkConnectPtr& link, bool transient = false);
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
  /** packet buffer is empty */
  void send_Next();

private:
  Factory<Server>& servers;
  Factory<Driver>& drivers;
  Factory<Filter>& filters;

  bool do_server(ServerPtr &link, IniSectionPtr& s, const std::string& servername, bool quiet = false);
  bool do_driver(LinkConnectPtr &link, IniSectionPtr& s, const std::string& servername, bool quiet = false);

  RouterLowPtr r_low;
  RouterHighPtr r_high;

  void send_L_Data(LDataPtr l1); // called by RouterHigh
  void queue_L_Data(LDataPtr l1); // called by RouterLow
  void queue_L_Busmonitor (LBusmonPtr l); // called by RouterLow

  /** loop counter for keeping track of iterators */
  int seq = 1;

  /** Markers to continue sending */
  bool low_send_more = false;
  bool high_send_more = false;
  bool high_sending = false;

public:
  /** Look up a filter by name */
  FilterPtr get_filter(const LinkConnectPtr_ &link, IniSectionPtr& s, const std::string& filtername);

  /** Create a temporary dummy driver stack to test arguments for filters etc.
   * Testing the calling driver's config args is the caller#s job.
   */
  bool checkStack(IniSectionPtr& cfg);

  /** name of our main section */
  std::string main;

private:
  /** create a link */
  LinkConnectPtr setup_link(std::string& name);

  /** interfaces */
  std::unordered_map<int, LinkConnectPtr> links;

  /** queue of interfaces which called linkChanged() */
  Queue<LinkConnectPtr> linkChanges;

  // libev
  ev::async trigger;
  void trigger_cb (ev::async &w, int revents);
  ev::async mtrigger;
  void mtrigger_cb (ev::async &w, int revents);
  ev::async state_trigger;
  void state_trigger_cb (ev::async &w, int revents);
  ev::timer start_timer;
  void start_timer_cb (ev::timer &w, int revents);
  float start_timeout;

  /** buffer queues for receiving from L2 */
  Queue < LDataPtr > buf;
  Queue < LBusmonPtr > mbuf;
  /** buffer for packets to ignore when repeat flag is set */
  Array < IgnoreInfo > ignore;

  /** Start of address block to assign dynamically to clients */
  eibaddr_t client_addrs_start;
  /** Length of address block to assign dynamically to clients */
  int client_addrs_len = 0;
  int client_addrs_pos;
  std::vector<bool> client_addrs;

public:
  bool hasClientAddrs(bool complain = true);

private:
  /** busmonitor callbacks */
  Array < Busmonitor_Info > busmonitor;
  /** vbusmonitor callbacks */
  Array < Busmonitor_Info > vbusmonitor;

  /** flag whether some driver is active */
  bool some_running = false;
  /** suppress "still foo" messages */
  int in_link_loop = 0;
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
  /** eventual exit code. Inremebted on fatal error */
  int exitcode = 0;

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

  /** error checking */
  bool has_send_more(LinkConnectPtr i);
};

/** global filter adapter, sending end */
class RouterHigh : public Driver
{
  Router* router;
public:
  RouterHigh(Router& r, const RouterLowPtr& rl);
  virtual ~RouterHigh() {}

  virtual void recv_L_Data (LDataPtr l);
  virtual void recv_L_Busmonitor (LBusmonPtr l);
  virtual void send_L_Data (LDataPtr l)
    {
      router->send_L_Data(std::move(l));
    }
  virtual bool checkAddress (eibaddr_t addr)
    {
      LinkConnectPtr link = nullptr;
      return router->checkAddress(addr, link);
    }
  virtual bool checkGroupAddress (eibaddr_t addr)
    {
      LinkConnectPtr link = nullptr;
      return router->checkGroupAddress(addr, link);
    }
  virtual bool hasAddress (eibaddr_t addr)
    {
      LinkConnectPtr link = nullptr;
      return router->hasAddress(addr, link);
    }
  virtual void addAddress (eibaddr_t addr)
    {
      if (addr != router->addr)
        ERRORPRINTF (t, E_ERROR, "%s filter: Trying to add address %s", router->main, FormatEIBAddr(addr));
    }

  virtual void start() { router->start_(); }
  virtual void stop() { router->stop_(); }
};

/** global filter adapter, receiving end */
class RouterLow : public LinkConnect_
{
public:
  Router* router;
  RouterLow(Router& r);
  virtual ~RouterLow() {}

  virtual void recv_L_Data (LDataPtr l) { router->queue_L_Data (std::move(l)); }
  virtual void recv_L_Busmonitor (LBusmonPtr l) { router->queue_L_Busmonitor (std::move(l)); }
  virtual void send_Next ();

  virtual void start() { send->start(); }
  virtual void stop() { send->stop(); }

  virtual void started() { router->started(); }
  virtual void stopped() { router->stopped(); }
  virtual void errored() { router->errored(); }
};

#endif
