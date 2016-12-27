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

#ifndef LAYER3_H
#define LAYER3_H

#include "layer2.h"
#include "lpdu.h"
#include <ev++.h>

class BaseServer;
class GroupCache;

/** stores a registered busmonitor callback */
typedef struct
{
  L_Busmonitor_CallBack *cb;
} Busmonitor_Info;

typedef enum
{
  /** perform no locking */
  Individual_Lock_None,
  /** pervent other connections of Individual_Lock_Connection */
  Individual_Lock_Connection
} Individual_Lock;

typedef struct
{
  CArray data;
  timestamp_t end;
} IgnoreInfo;

class Layer3 {
public:
  Layer3();
  virtual ~Layer3();

  /** debug output */
  virtual TracePtr tr() = 0;
  /** our default address */
  virtual eibaddr_t getDefaultAddr() = 0;
  /** group cache */
  virtual std::shared_ptr<GroupCache> getCache() = 0;

  /** register a layer2 interface, return true if successful*/
  virtual bool registerLayer2 (Layer2Ptr l2) = 0;
  /** deregister a layer2 interface, return true if successful*/
  virtual bool deregisterLayer2 (Layer2Ptr l2) = 0;

  /** register a busmonitor callback, return true, if successful*/
  virtual bool registerBusmonitor (L_Busmonitor_CallBack * c) = 0;
  /** register a vbusmonitor callback, return true, if successful*/
  virtual bool registerVBusmonitor (L_Busmonitor_CallBack * c) = 0;
  /** register a broadcast callback, return true, if successful*/

  /** deregister a busmonitor callback, return true, if successful*/
  virtual bool deregisterBusmonitor (L_Busmonitor_CallBack * c) = 0;
  /** deregister a vbusmonitor callback, return true, if successful*/
  virtual bool deregisterVBusmonitor (L_Busmonitor_CallBack * c) = 0;
  /** register a broadcast callback, return true, if successful*/

  /** accept a L_Data frame from L2 */
  virtual void recv_L_Data (LPDU * l) = 0;

  /** check if any interface accepts this address.
      'l2' says which interface NOT to check. */
  virtual bool hasAddress (eibaddr_t addr, Layer2Ptr l2 = nullptr) = 0;
  /** check if any interface accepts this group address.
      'l2' says which interface NOT to check. */
  virtual bool hasGroupAddress (eibaddr_t addr, Layer2Ptr l2 = nullptr) = 0;
  /** remember this server, for deallocation with the L3 */
  virtual void registerServer (BaseServer *s) = 0;
  /** remember this server, for deallocation with the L3 */
  virtual void deregisterServer (BaseServer *s) = 0;

  /** Get a free dynamic address */
  virtual eibaddr_t get_client_addr () = 0;
};

class Layer3real : public Layer3
{
public:
  Layer3real (eibaddr_t addr, TracePtr tr, bool force_broadcast = false);
  virtual ~Layer3real ();
private:
  TracePtr _tr = 0;
  eibaddr_t defaultAddr = 0;

  // libev
  ev::async trigger;
  void trigger_cb (ev::async &w, int revents);

  /** Layer 2 interfaces */
  Array < Layer2Ptr > layer2;
  /** buffer queue for receiving from L2 */
  Queue < LPDU * >buf;
  /** buffer for packets to ignore when repeat flag is set */
  Array < IgnoreInfo > ignore;

  /** Start of address block to assign dynamically to clients */
  eibaddr_t client_addrs_start;
  /** Length of address block to assign dynamically to clients */
  int client_addrs_len = 0;
  int client_addrs_pos;

  /** busmonitor callbacks */
  Array < Busmonitor_Info > busmonitor;
  /** vbusmonitor callbacks */
  Array < Busmonitor_Info > vbusmonitor;

  /** process incoming data from L2 */
  const char *Name() { return "Layer3"; }

  /** flag whether we're in .Run() */
  bool running;
  /** The servers using this L3 */
  Array < BaseServer * > servers;

  /** treat route count 7 as per EIB spec? */
  bool force_broadcast;

  ev::async cleanup;
  void cleanup_cb (ev::async &w, int revents);
  /** to-be-closed client connections*/
  Queue <Layer2Ptr> cleanup_q;

  /** group cache */
  std::shared_ptr<GroupCache> cache;

public:
  /** implement all of Layer3 */
  TracePtr tr() { return _tr; }
  eibaddr_t getDefaultAddr() { return defaultAddr; }
  std::shared_ptr<GroupCache> getCache() { return cache; }

  bool registerLayer2 (Layer2Ptr l2);
  bool deregisterLayer2 (Layer2Ptr l2);

  bool registerBusmonitor (L_Busmonitor_CallBack * c);
  bool registerVBusmonitor (L_Busmonitor_CallBack * c);

  bool deregisterBusmonitor (L_Busmonitor_CallBack * c);
  bool deregisterVBusmonitor (L_Busmonitor_CallBack * c);

  void recv_L_Data (LPDU * l);
  bool hasAddress (eibaddr_t addr, Layer2Ptr l2 = nullptr);
  bool hasGroupAddress (eibaddr_t addr, Layer2Ptr l2 = nullptr);
  void registerServer (BaseServer *s) { servers.push_back (s); }
  void deregisterServer (BaseServer *s);

  void set_client_block (eibaddr_t r_start, int r_len);
  eibaddr_t get_client_addr ();
};

#endif
