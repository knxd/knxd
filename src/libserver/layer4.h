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

#ifndef LAYER4_H
#define LAYER4_H

#include "layer3.h"

/** information about a broadcast packet */
typedef struct
{
  /** Layer 4 data */
  CArray data;
  /** source address */
  eibaddr_t src;
} BroadcastComm;

/** informations about a group communication packet */
typedef struct
{
  /** Layer 4 data */
  CArray data;
  /** source address */
  eibaddr_t src;
} GroupComm;

/** a raw layer 4 connection packet */
typedef struct
{
  /** Layer 4 data */
  CArray data;
  /** individual address of the remote device */
  eibaddr_t addr;
} TpduComm;

/** informations about a group communication packet */
typedef struct
{
  /** Layer 4 data */
  CArray data;
  /** source address */
  eibaddr_t src;
  /** destination address */
  eibaddr_t dst;
} GroupAPDU;

class Layer4common:public Layer2mixin
{
protected:
  Layer4common(Trace * tr);
  bool init_ok;
public:
  bool init (Layer3 *l3);
  void cleanup () { Layer2mixin::RunStop(); }
};

/** Broadcast Layer 4 connection */
class T_Broadcast:public Layer4common
{
  /** output queue */
  Queue < BroadcastComm > outqueue;
  /** semaphore for output queue */
  pth_sem_t sem;

public:
  T_Broadcast (Trace * t, int write_only);
  virtual ~T_Broadcast ();

  /** enqueues a packet */
  void Send_L_Data (L_Data_PDU * l);
  /** dequeues APDU of a broadcast; aborts with NULL if stop occurs */
  BroadcastComm *Get (pth_event_t stop);
  /** send APDU c */
  void Send (const CArray & c);
};
typedef std::shared_ptr<T_Broadcast> T_BroadcastPtr;

/** Group Communication socket */
class GroupSocket:public Layer4common
{
  /** output queue */
  Queue < GroupAPDU > outqueue;
  /** semaphore for output queue */
  pth_sem_t sem;

public:
  GroupSocket (Trace * t, int write_only);
  virtual ~GroupSocket ();

  /** enqueues a packet from L3 */
  void Send_L_Data (L_Data_PDU * l);
  /** dequeues APDU of a broadcast; aborts with NULL if stop occurs */
  GroupAPDU *Get (pth_event_t stop);
  /** send APDU to L3 */
  void Send (const GroupAPDU & c);
};
typedef std::shared_ptr<GroupSocket> GroupSocketPtr;

/** Group Layer 4 connection */
class T_Group:public Layer4common
{
  /** output queue */
  Queue < GroupComm > outqueue;
  /** semaphore for output queue */
  pth_sem_t sem;
  /** group address */
  eibaddr_t groupaddr;

public:
  T_Group (Trace * t, eibaddr_t dest, int write_only);
  virtual ~T_Group ();

  /** enqueues a packet from L3 */
  void Send_L_Data (L_Data_PDU * l);
  /** dequeues APDU of a group telegram; aborts with NULL if stop occurs */
  GroupComm *Get (pth_event_t stop);
  /** send APDU to L3 */
  void Send (const CArray & c);
};
typedef std::shared_ptr<T_Group> T_GroupPtr;

/** Layer 4 raw individual connection */
class T_TPDU:public Layer4common
{
  /** output queue */
  Queue < TpduComm > outqueue;
  /** semaphore for output queue */
  pth_sem_t sem;
  /** source address to use */
  eibaddr_t src;

public:
  T_TPDU (Trace * t, eibaddr_t src);
  virtual ~T_TPDU ();

  /** enqueues a packet from L3 */
  void Send_L_Data (L_Data_PDU * l);
  /** dequeues TPDU of a telegram; aborts with NULL if stop occurs */
  TpduComm *Get (pth_event_t stop);
  /** send APDU to L3 */
  void Send (const TpduComm & c);
};
typedef std::shared_ptr<T_TPDU> T_TPDUPtr;

/** Layer 4 T_Individual connection */
class T_Individual:public Layer4common
{
  /** output queue */
  Queue < CArray > outqueue;
  /** semaphore for output queue */
  pth_sem_t sem;
  /** destination address */
  eibaddr_t dest;

public:
  T_Individual (Trace * t, eibaddr_t dest, int write_only);
  virtual ~T_Individual ();

  /** enqueues a packet from L3 */
  void Send_L_Data (L_Data_PDU * l);
  /** dequeues APDU of a telegram; aborts with NULL if stop occurs */
  CArray *Get (pth_event_t stop);
  /** send APDU to L3 */
  void Send (const CArray & c);
};
typedef std::shared_ptr<T_Individual> T_IndividualPtr;

/** implement a client T_Connection */
class T_Connection:public Layer4common, private Thread
{
  /** input queue */
  Queue < CArray > in;
  /** buffer queue for layer 3 */
  Queue < L_Data_PDU * >buf;
  /** output queue */
  Queue < CArray > out;
  /** receiving sequence number */
  int recvno;
  /** sending sequence number */
  int sendno;
  /** state */
  int mode;
  /** repeat count of the transmitting frame */
  int repcount;
  eibaddr_t dest;
  /** semaphore for input queue */
  pth_sem_t insem;
  /** semaphre for buffer queue */
  pth_sem_t bufsem;
  /** semaphore for output queue */
  pth_sem_t outsem;

  /** sends T_Connect */
  void SendConnect ();
  /** sends T_Disconnect */
  void SendDisconnect ();
  /** send T_ACK */
  void SendAck (int serno);
  /** Sends T_DataConnected */
  void SendData (int serno, const CArray & c);
  void Run (pth_sem_t * stop);
  const char *Name() { return "Tconnection"; }
public:
  T_Connection (Trace * t, eibaddr_t dest);
  virtual ~T_Connection ();

  /** enqueues a packet from L3 */
  void Send_L_Data (L_Data_PDU * l);
  /** dequeues APDU of a telegram; aborts with NULL if stop occurs */
  CArray *Get (pth_event_t stop);
  /** send APDU to L3 */
  void Send (const CArray & c);
};
typedef std::shared_ptr<T_Connection> T_ConnectionPtr;

#endif
