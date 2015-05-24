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

class BaseServer;

/** stores a registered busmonitor callback */
typedef struct
{
  L_Busmonitor_CallBack *cb;
} Busmonitor_Info;

/** stores a registered broadcast callback */
typedef struct
{
  L_Data_CallBack *cb;
} Broadcast_Info;

/** stores a registered group callback */
typedef struct
{
  L_Data_CallBack *cb;
  /** group address, for which the frames should be delivered */
  eibaddr_t dest;
} Group_Info;

typedef enum
{
  /** perform no locking */
  Individual_Lock_None,
  /** pervent other connections of Individual_Lock_Connection */
  Individual_Lock_Connection
} Individual_Lock;

/** stores a registered individual callback */
typedef struct
{
  L_Data_CallBack *cb;
  /** source address, from which the frames should be delivered */
  eibaddr_t src;
  /** destiation address, for which the frames should be delivered */
  eibaddr_t dest;
  /** lock of the connection */
  Individual_Lock lock;
} Individual_Info;

typedef struct
{
  CArray data;
  timestamp_t end;

} IgnoreInfo;

class Layer3;

/** Layer 3 frame dispatches */
class Layer3:private Thread
{
  /** Layer 2 interfaces */
  Array < Layer2Interface * > layer2;
  /** buffer queue for receiving from L2 */
  Queue < LPDU * >buf;
  /** semaphre for buffer queue */
  pth_sem_t bufsem;
  /** buffer for packets to ignore when repeat flag is set */
  Array < IgnoreInfo > ignore;

  /** busmonitor callbacks */
  Array < Busmonitor_Info > busmonitor;
  /** vbusmonitor callbacks */
  Array < Busmonitor_Info > vbusmonitor;
  /** broadcast callbacks */
  Array < Broadcast_Info > broadcast;
  /** group callbacks */
  Array < Group_Info > group;
  /** individual callbacks */
  Array < Individual_Info > individual;

  /** process incoming data from L2 */
  void Run (pth_sem_t * stop);
  const char *Name() { return "Layer3"; }

  /** flag whether we're in .Run() */
  bool running;
  /** The trace objects used here */
  Array < Trace * > tracers;
  /** The servers using this L3 */
  Array < BaseServer * > servers;

public:
  /** debug output */
  Trace *t;
  /** our default address */
  eibaddr_t defaultAddr;
  /** temporary: fake L2 for interfaces which aren't yet */
  Layer2Interface *FakeL2;

  Layer3 (eibaddr_t addr, Trace * tr);
  virtual ~Layer3 ();

  /** register a layer2 interface, return true if successful*/
  bool registerLayer2 (Layer2Interface * l2);
  /** deregister a layer2 interface, return true if successful*/
  bool deregisterLayer2 (Layer2Interface * l2);

  /** register a busmonitor callback, return true, if successful*/
  bool registerBusmonitor (L_Busmonitor_CallBack * c);
  /** register a vbusmonitor callback, return true, if successful*/
  bool registerVBusmonitor (L_Busmonitor_CallBack * c);
  /** register a broadcast callback, return true, if successful*/
  bool registerBroadcastCallBack (L_Data_CallBack * c);
  /** register a group callback, return true, if successful
   * @param c callback
   * @param addr group address (0 means all)
   */
  bool registerGroupCallBack (L_Data_CallBack * c, eibaddr_t addr);
  /** register a individual callback, return true, if successful
   * @param c callback
   * @param src source individual address (0 means all)
   * @param dest destination individual address (0 means default address)
   * @param lock Locktype of the connection
   */
  bool registerIndividualCallBack (L_Data_CallBack * c, Individual_Lock lock,
				   eibaddr_t src, eibaddr_t dest = 0);

  /** deregister a busmonitor callback, return true, if successful*/
  bool deregisterBusmonitor (L_Busmonitor_CallBack * c);
  /** deregister a vbusmonitor callback, return true, if successful*/
  bool deregisterVBusmonitor (L_Busmonitor_CallBack * c);
  /** register a broadcast callback, return true, if successful*/
  bool deregisterBroadcastCallBack (L_Data_CallBack * c);
  /** deregister a group callback, return true, if successful
   * @param c callback
   * @param addr group address (0 means all)
   */
  bool deregisterGroupCallBack (L_Data_CallBack * c, eibaddr_t addr);
  /** register a individual callback, return true, if successful
   * @param c callback
   * @param src source individual address (0 means all)
   * @param dest destination individual address (0 means default address)
   */
  bool deregisterIndividualCallBack (L_Data_CallBack * c, eibaddr_t src,
				     eibaddr_t dest = 0);
  /** accept a L_Data frame from L2 */
  void recv_L_Data (LPDU * l);
  /** sends a L_Data frame asynchronouse */
  void send_L_Data (L_Data_PDU * l);

  /** check if any interface accepts this address.
      'l2' says which interface NOT to check. */
  bool hasAddress (eibaddr_t addr, Layer2Interface *l2 = 0);
  /** check if any interface accepts this group address.
      'l2' says which interface NOT to check. */
  bool hasGroupAddress (eibaddr_t addr, Layer2Interface *l2 = 0);
  /** save a pointer to this tracer, for deallocation with the L3 */
  void registerTracer (Trace *t) { tracers.add (t); }
  /** remember this server, for deallocation with the L3 */
  void registerServer (BaseServer *s) { servers.add (s); }
};


#endif
