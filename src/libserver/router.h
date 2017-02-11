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

#include "link.h"
#include "lpdu.h"
#include <ev++.h>

class BaseServer;
class GroupCache;
class LinkConnect;

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

class Router {
public:
  Router(IniData d, std::string &sn);
  ~Router();

  /** debug output */
  TracePtr t;

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

  /** register a layer2 interface, return the L3 registered-to if successful.
   * Registration may shift the caller to a different L3 if intermediate
   * layer23 modules are present
   */
private:
  map<std::string, LinkRecv> links;

  /** register a busmonitor callback, return true, if successful*/
  bool registerBusmonitor (L_Busmonitor_CallBack * c);
  /** register a vbusmonitor callback, return true, if successful*/
  bool registerVBusmonitor (L_Busmonitor_CallBack * c);
  /** register a broadcast callback, return true, if successful*/

  /** deregister a busmonitor callback, return true, if successful*/
  bool deregisterBusmonitor (L_Busmonitor_CallBack * c);
  /** deregister a vbusmonitor callback, return true, if successful*/
  bool deregisterVBusmonitor (L_Busmonitor_CallBack * c);
  /** register a broadcast callback, return true, if successful*/

  /** accept a L_Data frame */
  void recv_L_Data (LDataPtr l, LinkConnectPtr link);
  /** accept a L_Busmonitor frame */
  void recv_L_Busmonitor (LBusmonPtr l);

  /** check if any interface knows this address. */
  LinkConnectPtr hasAddress (eibaddr_t addr);
  /** check if any interface accepts this address.
      'l2' says which interface NOT to check. */
  bool checkAddress (eibaddr_t addr, LinkConnectPtr l2 = nullptr);
  /** check if any interface accepts this group address.
      'l2' says which interface NOT to check. */
  bool checkGroupAddress (eibaddr_t addr, LinkConnectPtr l2 = nullptr);

  /** remember this server, for deallocation with the L3 */
  virtual void registerServer (BaseServer *s) = 0;
  /** remember this server, for deallocation with the L3 */
  virtual void deregisterServer (BaseServer *s) = 0;

  /** Get a free dynamic address */
  virtual eibaddr_t get_client_addr (TracePtr t);
  /** … and release it */
  virtual void release_client_addr (eibaddr_t addr);

  virtual Driver3 * findFilter (const char *name) { return nullptr; }

private:
  // libev
  ev::async trigger;
  void trigger_cb (ev::async &w, int revents);
  ev::async mtrigger;
  void mtrigger_cb (ev::async &w, int revents);

  /** interfaces */
  Array < LinkBasePtr > layer2;
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

  /** busmonitor callbacks */
  Array < Busmonitor_Info > busmonitor;
  /** vbusmonitor callbacks */
  Array < Busmonitor_Info > vbusmonitor;

  /** flag whether we've been started */
  bool running;

  /** treat route count 7 as per EIB spec? */
  bool force_broadcast;

  ev::async cleanup;
  void cleanup_cb (ev::async &w, int revents);
  /** to-be-closed client connections*/
  Queue < LinkBasePtr > cleanup_q;

  /** group cache */
  std::shared_ptr<GroupCache> cache;
};

#endif
