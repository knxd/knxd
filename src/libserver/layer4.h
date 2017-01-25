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

class Layer4common:public Layer2single
{
protected:
  Layer4common(TracePtr tr);
  bool init_ok;
public:
  bool init (Layer3 *l3);
};

template<class T>
class T_Reader
{
public:
  virtual void send(T &) = 0;
  eibaddr_t addr; // phys address of this client
};

/** Broadcast Layer 4 connection */
class T_Broadcast:public Layer4common
{
  T_Reader<BroadcastComm> *app;

public:
  T_Broadcast (TracePtr t, int write_only);
  virtual ~T_Broadcast ();

  bool init (T_Reader<BroadcastComm> *app, Layer3 *l3);

  /** enqueues a packet */
  void send_L_Data (L_Data_PDU * l);
  /** send APDU c */
  void recv (const CArray & c);
};
typedef std::shared_ptr<T_Broadcast> T_BroadcastPtr;

/** Group Communication socket */
class GroupSocket:public Layer4common
{
  T_Reader<GroupAPDU> *app;

public:
  GroupSocket (TracePtr t, int write_only);
  virtual ~GroupSocket ();

  bool init (T_Reader<GroupAPDU> *app, Layer3 *l3);

  /** enqueues a packet from L3 */
  void send_L_Data (L_Data_PDU * l);
  /** send APDU to L3 */
  void recv (const GroupAPDU & c);
};
typedef std::shared_ptr<GroupSocket> GroupSocketPtr;

/** Group Layer 4 connection */
class T_Group:public Layer4common
{
  T_Reader<GroupComm> *app;
  /** group address */
  eibaddr_t groupaddr;

public:
  T_Group (TracePtr t, eibaddr_t dest, int write_only);
  virtual ~T_Group ();

  bool init (T_Reader<GroupComm> *app, Layer3 *l3);

  /** enqueues a packet from L3 */
  void send_L_Data (L_Data_PDU * l);
  /** send APDU to L3 */
  void recv (const CArray & c);
};
typedef std::shared_ptr<T_Group> T_GroupPtr;

/** Layer 4 raw individual connection */
class T_TPDU:public Layer4common
{
  T_Reader<TpduComm> *app;
  /** source address to use */
  eibaddr_t src;

public:
  T_TPDU (TracePtr t, eibaddr_t src);
  virtual ~T_TPDU ();

  bool init (T_Reader<TpduComm> *app, Layer3 *l3);

  /** enqueues a packet from L3 */
  void send_L_Data (L_Data_PDU * l);
  /** send APDU to L3 */
  void recv (const TpduComm & c);
};
typedef std::shared_ptr<T_TPDU> T_TPDUPtr;

/** Layer 4 T_Individual connection */
class T_Individual:public Layer4common
{
  T_Reader<CArray> *app;

  /** destination address */
  eibaddr_t dest;

public:
  T_Individual (TracePtr t, eibaddr_t dest, int write_only);
  virtual ~T_Individual ();

  bool init (T_Reader<CArray> *app, Layer3 *l3);

  /** enqueues a packet from L3 */
  void send_L_Data (L_Data_PDU * l);
  /** send APDU to L3 */
  void recv (const CArray & c);
};
typedef std::shared_ptr<T_Individual> T_IndividualPtr;

/** implement a client T_Connection */
class T_Connection:public Layer4common
{
  T_Reader<CArray> *app;

  ev::timer timer; void timer_cb(ev::timer &w, int revents);

  /** input queue */
  Queue < CArray > in;
  CArray current;
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

  /** sends T_Connect */
  void SendConnect ();
  /** sends T_Disconnect */
  void SendDisconnect ();
  /** send T_ACK */
  void SendAck (int serno);
  /** Sends T_DataConnected */
  void SendData (int serno, const CArray & c);
  /** process the next bit from sendq if mode==1 */
  void SendCheck (); 
  const char *Name() { return "Tconnection"; }
public:
  T_Connection (TracePtr t, eibaddr_t dest);
  virtual ~T_Connection ();

  bool init (T_Reader<CArray> *app, Layer3 *l3);

  /** enqueues a packet from L3 */
  void send_L_Data (L_Data_PDU * l);
  /** send APDU to L3 */
  void recv (const CArray & c);

  void stop();
};
typedef std::shared_ptr<T_Connection> T_ConnectionPtr;

#endif
