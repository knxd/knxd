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

#include "link.h"
#include "router.h"

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

template<class T>
class T_Reader
{
public:
  virtual void send(T &) = 0;
};

template<class COMM>
class Layer4common:public LineDriver
{
protected:
  T_Reader<COMM> *app;
  Layer4common(T_Reader<COMM> *app, LinkConnectClientPtr lc) : LineDriver(lc)
  {
    this->app = app;
  };
  virtual ~Layer4common();
public:
  bool setup() {
    if (!LineDriver::setup())
      return false;
    return true;
  }

};

template<class COMM>
class Layer4commonWO:public Layer4common<COMM>
{
  bool write_only;
public:
  Layer4commonWO (T_Reader<COMM> *app, LinkConnectClientPtr lc, bool write_only) : Layer4common<COMM>(app,lc)
    { this->write_only = write_only; }

  bool checkAddress(eibaddr_t addr) { return !write_only && addr == this->getAddress(); }
  bool checkGroupAddress(eibaddr_t addr UNUSED) { return !write_only; }
};

/** Broadcast Layer 4 connection */
class T_Broadcast:public Layer4commonWO<BroadcastComm>
{
public:
  T_Broadcast (T_Reader<BroadcastComm> *app, LinkConnectClientPtr lc, bool write_only);
  virtual ~T_Broadcast ();

  /** enqueues a packet */
  void send_L_Data (LDataPtr l);
  /** send APDU c */
  void recv_Data (const CArray & c);
};
typedef std::shared_ptr<T_Broadcast> T_BroadcastPtr;

/** Group Communication socket */
class GroupSocket:public Layer4commonWO<GroupAPDU>
{
public:
  GroupSocket (T_Reader<GroupAPDU> *app, LinkConnectClientPtr lc, bool write_only);
  virtual ~GroupSocket ();

  /** enqueues a packet from L3 */
  void send_L_Data (LDataPtr l);
  /** send APDU to L3 */
  void recv_Data (const GroupAPDU & c);
};
typedef std::shared_ptr<GroupSocket> GroupSocketPtr;

/** Group Layer 4 connection */
class T_Group:public Layer4commonWO<GroupComm>
{
  /** group address */
  eibaddr_t groupaddr;

public:
  T_Group (T_Reader<GroupComm> *app, LinkConnectClientPtr lc, eibaddr_t group, bool write_only);
  virtual ~T_Group ();

  /** enqueues a packet from L3 */
  void send_L_Data (LDataPtr l);
  /** send APDU to L3 */
  void recv_Data (const CArray & c);
};
typedef std::shared_ptr<T_Group> T_GroupPtr;

/** Layer 4 raw individual connection */
class T_TPDU:public Layer4common<TpduComm>
{
  /** source address to use */
  eibaddr_t src;

public:
  T_TPDU (T_Reader<TpduComm> *app, LinkConnectClientPtr lc, eibaddr_t src);
  virtual ~T_TPDU ();

  /** enqueues a packet from L3 */
  void send_L_Data (LDataPtr l);
  /** send APDU to L3 */
  void recv_Data (const TpduComm & c);
};
typedef std::shared_ptr<T_TPDU> T_TPDUPtr;

/** Layer 4 T_Individual connection */
class T_Individual:public Layer4commonWO<CArray>
{
  /** destination address */
  eibaddr_t dest;

public:
  T_Individual (T_Reader<CArray> *app, LinkConnectClientPtr lc, eibaddr_t dest, bool write_only);
  virtual ~T_Individual ();

  /** enqueues a packet from L3 */
  void send_L_Data (LDataPtr l);
  /** send APDU to L3 */
  void recv_Data (const CArray & c);
};
typedef std::shared_ptr<T_Individual> T_IndividualPtr;

/** implement a client T_Connection */
class T_Connection:public Layer4common<CArray>
{
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
public:
  T_Connection (T_Reader<CArray> *app, LinkConnectClientPtr lc, eibaddr_t dest);
  virtual ~T_Connection ();

  /** enqueues a packet from L3 */
  void send_L_Data (LDataPtr l);
  /** send APDU to L3 */
  void recv_Data (const CArray & c);

  void stop();
};
typedef std::shared_ptr<T_Connection> T_ConnectionPtr;

#endif
