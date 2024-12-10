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

#ifndef EIBNETIP_H
#define EIBNETIP_H

#include <ev++.h>
#include <netinet/in.h>

#include "apdu.h"
#include "cm_ip.h"
#include "common.h"
#include "iobuf.h" // for nonblocking
#include "ipsupport.h"
#include "lpdu.h"

// all values are from 03_08_01 5.* unless otherwise specified

#define KNXNETIP_VERSION_10   0x10
#define HEADER_SIZE_10        0x06

/** Service type identifiers */
enum ServiceType : uint16_t
{
  /* Core (0x0200 .. 0x020F) */
  SEARCH_REQUEST = 0x0201,
  SEARCH_RESPONSE = 0x0202,
  DESCRIPTION_REQUEST = 0x0203,
  DESCRIPTION_RESPONSE = 0x0204,
  CONNECTION_REQUEST = 0x0205,
  CONNECTION_RESPONSE = 0x0206,
  CONNECTIONSTATE_REQUEST = 0x0207,
  CONNECTIONSTATE_RESPONSE = 0x0208,
  DISCONNECT_REQUEST = 0x0209,
  DISCONNECT_RESPONSE = 0x020A,

  /* Device Management (0x0310 .. 0x031F) */
  DEVICE_CONFIGURATION_REQUEST = 0x0310,
  DEVICE_CONFIGURATION_ACK = 0x0311,

  /* Tunnelling (0x0420 .. 0x042F) */
  TUNNEL_REQUEST = 0x0420,
  TUNNEL_RESPONSE = 0x0421,

  /* Routing (0x0530 .. 0x053F)  */
  ROUTING_INDICATION = 0x0530,
  ROUTING_LOST_MESSAGE = 0x0531,
  ROUTING_BUSY = 0x532, // 03.02.06, 4.1.3

  /* Remote Logging (0x0600 .. 0x06FF) */

  /* Remote Configuration and Diagnosis (0x0740 .. 0x07FF) */
  REMOTE_DIAGNOSTIC_REQUEST = 0x0740, // 03.08.07, 4.4.1
  REMOTE_DIAGNOSTIC_RESPONSE = 0x0741, // 03.08.07, 4.4.2
  REMOTE_BASIC_CONFIGURATION_REQUEST = 0x0742, // 03.08.07, 4.4.3
  REMOTE_RESET_REQUEST = 0x0743, // 03.08.07, 4.4.4

  /* Object Server (0x0800 .. 0x8FF) */
};

/** Connection types */
enum ConnectionType : uint8_t
{
  DEVICE_MGMT_CONNECTION = 0x03,
  TUNNEL_CONNECTION = 0x04,
  REMLOG_CONNECTION = 0x06,
  REMCONF_CONNECTION = 0x07,
  OBJSRV_CONNECTION = 0x08,
};

/** Error codes */
enum ErrorCode : uint8_t
{
  E_NO_ERROR = 0x00,
  E_HOST_PROTOCOL_TYPE = 0x01,
  E_VERSION_NOT_SUPPORTED = 0x02,
  E_SEQUENCE_NUMBER = 0x04,
  E_CONNECTION_ID = 0x21,
  E_CONNECTION_TYPE = 0x22,
  E_CONNECTION_OPTION = 0x23,
  E_NO_MORE_CONNECTIONS = 0x24,
  E_NO_MORE_UNIQUE_CONNECTIONS = 0x25, // 03.08.04, 2.2.2
  E_DATA_CONNECTION = 0x26,
  E_KNX_CONNECTION = 0x27,
  E_TUNNELING_LAYER = 0x29,
};

/** Description type codes */
enum DIBcode : uint8_t
{
  DEVICE_INFO = 0x01,
  SUPP_SVC_FAMILIES = 0x02,
  MFR_DATA = 0xFE,
};

/** Medium codes */
enum MediumCode : uint8_t
{
  M_TP1 = 0x02,
  M_PL110 = 0x04,
  M_RF = 0x10,
  M_IP = 0x20,
};

/** Host protocol codes */
enum HostProtocolCode : uint8_t
{
  IPV4_UDP = 0x01,
  IPV4_TCP = 0x02,
};

/** Tunnelling KNX layers */
enum TunnellingLayer : uint8_t
{
  TUNNEL_LINKLAYER = 0x02,
  TUNNEL_RAW = 0x04,
  TUNNEL_BUSMONITOR = 0x80,
};

/* Timeout constants */
constexpr ev::tstamp CONNECT_REQUEST_TIMEOUT = 10;
constexpr ev::tstamp CONNECTIONSTATE_REQUEST_TIMEOUT = 10;
constexpr ev::tstamp DEVICE_CONFIGURATION_REQUEST_TIMEOUT = 10;
constexpr ev::tstamp TUNNELING_REQUEST_TIMEOUT = 1;
constexpr ev::tstamp CONNECTION_ALIVE_TIME = 120;

enum SockMode
{
  S_RDWR, S_RD, S_WR,
};

/** Description Block service entry */
struct DIB_service_Entry
{
  uint8_t family;
  uint8_t version;
};

// A sockaddr_in for "Route Back" HPAIs
// See ISO 22510:2019 section 5.2.8.6.2 b)
inline sockaddr_in routeBackAddr()
{
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  return addr;
}

/** represents a EIBnet/IP packet */
class EIBNetIPPacket
{
public:
  /** service code*/
  int service;
  /** payload */
  CArray data;
  /** the protocol over which the packet is transported */
  HostProtocolCode protocol;
  /** source address */
  struct sockaddr_in src;

  EIBNetIPPacket ();
  virtual ~EIBNetIPPacket () = default;

  /** create from character array */
  static EIBNetIPPacket *fromPacket (const CArray & c,
                                     const struct sockaddr_in src,
                                     HostProtocolCode protocol = IPV4_UDP);
  /** convert to character array */
  CArray ToPacket () const;
};

class EIBnet_SearchRequest
{
public:
  EIBnet_SearchRequest ();
  struct sockaddr_in caddr;
  bool nat = false;
  EIBNetIPPacket ToPacket (HostProtocolCode protocol = IPV4_UDP) const;
};

int parseEIBnet_SearchRequest (const EIBNetIPPacket & p,
                               EIBnet_SearchRequest & r);

class EIBnet_SearchResponse
{
public:
  EIBnet_SearchResponse ();
  uint8_t KNXmedium = 0;
  uint8_t devicestatus = 0;
  eibaddr_t individual_addr = 0;
  uint16_t installid = 0;
  serialnumber_t serial;
  std::vector<DIB_service_Entry> services;
  struct in_addr multicastaddr;
  uint8_t MAC[6];
  char name[30];
  struct sockaddr_in caddr;
  bool nat = false;
  EIBNetIPPacket ToPacket (HostProtocolCode protocol = IPV4_UDP) const;
};

int parseEIBnet_SearchResponse (const EIBNetIPPacket & p,
                                EIBnet_SearchResponse & r);

class EIBnet_DescriptionRequest
{
public:
  EIBnet_DescriptionRequest ();
  struct sockaddr_in caddr;
  bool nat = false;
  EIBNetIPPacket ToPacket (HostProtocolCode protocol = IPV4_UDP) const;
};

int parseEIBnet_DescriptionRequest (const EIBNetIPPacket & p,
                                    EIBnet_DescriptionRequest & r);

class EIBnet_DescriptionResponse
{
public:
  EIBnet_DescriptionResponse ();
  uint8_t KNXmedium = 0;
  uint8_t devicestatus = 0;
  eibaddr_t individual_addr = 0;
  uint16_t installid = 0;
  serialnumber_t serial;
  std::vector < DIB_service_Entry > services;
  struct in_addr multicastaddr;
  uint8_t MAC[6];
  char name[30];
  CArray optional;
  EIBNetIPPacket ToPacket (HostProtocolCode protocol = IPV4_UDP) const;
};

int parseEIBnet_DescriptionResponse (const EIBNetIPPacket & p,
                                     EIBnet_DescriptionResponse & r);

class EIBnet_ConnectRequest
{
public:
  EIBnet_ConnectRequest ();
  struct sockaddr_in caddr;
  struct sockaddr_in daddr;
  CArray CRI;
  bool nat = false;
  EIBNetIPPacket ToPacket (HostProtocolCode protocol = IPV4_UDP) const;
};

int parseEIBnet_ConnectRequest (const EIBNetIPPacket & p,
                                EIBnet_ConnectRequest & r);

class EIBnet_ConnectResponse
{
public:
  EIBnet_ConnectResponse ();
  uint8_t channel = 0;
  uint8_t status = 0;
  struct sockaddr_in daddr;
  bool nat = false;
  CArray CRD;
  EIBNetIPPacket ToPacket (HostProtocolCode protocol = IPV4_UDP) const;
};

int parseEIBnet_ConnectResponse (const EIBNetIPPacket & p,
                                 EIBnet_ConnectResponse & r);

class EIBnet_ConnectionStateRequest
{
public:
  EIBnet_ConnectionStateRequest ();
  uint8_t channel = 0;
  uint8_t status = 0;
  struct sockaddr_in caddr;
  bool nat = false;
  EIBNetIPPacket ToPacket (HostProtocolCode protocol = IPV4_UDP) const;
};

int parseEIBnet_ConnectionStateRequest (const EIBNetIPPacket & p,
                                        EIBnet_ConnectionStateRequest & r);

class EIBnet_ConnectionStateResponse
{
public:
  EIBnet_ConnectionStateResponse () = default;
  uint8_t channel = 0;
  uint8_t status = 0;
  EIBNetIPPacket ToPacket (HostProtocolCode protocol = IPV4_UDP) const;
};

int parseEIBnet_ConnectionStateResponse (const EIBNetIPPacket & p,
    EIBnet_ConnectionStateResponse & r);

class EIBnet_DisconnectRequest
{
public:
  EIBnet_DisconnectRequest ();
  struct sockaddr_in caddr;
  uint8_t channel = 0;
  bool nat = false;
  EIBNetIPPacket ToPacket (HostProtocolCode protocol = IPV4_UDP) const;
};

int parseEIBnet_DisconnectRequest (const EIBNetIPPacket & p,
                                   EIBnet_DisconnectRequest & r);

class EIBnet_DisconnectResponse
{
public:
  EIBnet_DisconnectResponse () = default;
  uint8_t channel = 0;
  uint8_t status = 0;
  EIBNetIPPacket ToPacket (HostProtocolCode protocol = IPV4_UDP) const;
};

int parseEIBnet_DisconnectResponse (const EIBNetIPPacket & p,
                                    EIBnet_DisconnectResponse & r);

class EIBnet_ConfigRequest // @todo rename to EIBnet_DeviceConfigurationRequest
{
public:
  EIBnet_ConfigRequest () = default;
  uint8_t channel = 0;
  uint8_t seqno = 0;
  CArray CEMI;
  EIBNetIPPacket ToPacket (HostProtocolCode protocol = IPV4_UDP) const;
};

int parseEIBnet_ConfigRequest (const EIBNetIPPacket & p,
                               EIBnet_ConfigRequest & r); // @todo rename to parseEIBnet_DeviceConfigurationRequest

class EIBnet_ConfigACK // @todo rename to EIBnet_DeviceConfigurationACK
{
public:
  EIBnet_ConfigACK () = default;
  uint8_t channel = 0;
  uint8_t seqno = 0;
  uint8_t status = 0;
  EIBNetIPPacket ToPacket (HostProtocolCode protocol = IPV4_UDP) const;
};

int parseEIBnet_ConfigACK (const EIBNetIPPacket & p, EIBnet_ConfigACK & r); // @todo rename to parseEIBnet_DeviceConfigurationAck

class EIBnet_TunnelRequest
{
public:
  EIBnet_TunnelRequest () = default;
  uint8_t channel = 0;
  uint8_t seqno = 0;
  CArray CEMI;
  EIBNetIPPacket ToPacket (HostProtocolCode protocol = IPV4_UDP) const;
};

int parseEIBnet_TunnelRequest (const EIBNetIPPacket & p,
                               EIBnet_TunnelRequest & r);

class EIBnet_TunnelACK
{
public:
  EIBnet_TunnelACK () = default;
  uint8_t channel = 0;
  uint8_t seqno = 0;
  uint8_t status = 0;
  EIBNetIPPacket ToPacket (HostProtocolCode protocol = IPV4_UDP) const;
};

int parseEIBnet_TunnelACK (const EIBNetIPPacket & p, EIBnet_TunnelACK & r);

class EIBnet_RoutingIndication
{
  EIBNetIPPacket ToPacket (HostProtocolCode protocol = IPV4_UDP) const;
};

int parseEIBnet_RoutingIndication (const EIBNetIPPacket & p, EIBnet_RoutingIndication & r);

class EIBnet_RoutingLostMessage
{
  EIBNetIPPacket ToPacket (HostProtocolCode protocol = IPV4_UDP) const;
};

int parseEIBnet_RoutingLostMessage (const EIBNetIPPacket & p, EIBnet_RoutingLostMessage & r);

typedef void (*eibpacket_cb_t)(void *data, EIBNetIPPacket *p);

class EIBPacketCallback
{
public:
  // function callback
  template<void (*function)(EIBNetIPPacket *p)>
  void set (void *data = 0) throw ()
  {
    set_ (data, function_thunk<function>);
  }

  template<void (*function)(EIBNetIPPacket *p)>
  static void function_thunk (void *arg, EIBNetIPPacket *p)
  {
    function(p);
  }

  // method callback
  template<class K, void (K::*method)(EIBNetIPPacket *p)>
  void set (K *object)
  {
    set_ (object, method_thunk<K, method>);
  }

  template<class K, void (K::*method)(EIBNetIPPacket *p)>
  static void method_thunk (void *arg, EIBNetIPPacket *p)
  {
    (static_cast<K *>(arg)->*method) (p);
  }

  void operator()(EIBNetIPPacket *p)
  {
    (*cb_code)(cb_data, p);
  }

private:
  eibpacket_cb_t cb_code = 0;
  void *cb_data = 0;

  void set_ (const void *data, eibpacket_cb_t cb)
  {
    this->cb_data = (void *)data;
    this->cb_code = cb;
  }
};

/** represents a EIBnet/IP packet to send */
struct _EIBNetIP_Send
{
  /** packet */
  EIBNetIPPacket data;
  /** destination address */
  struct sockaddr_in addr;
};

/** EIBnet/IP socket */
class EIBNetIPSocket
{
public:
  EIBPacketCallback on_recv;
  InfoCallback on_error;
  InfoCallback on_next;

  EIBNetIPSocket (struct sockaddr_in bindaddr, bool reuseaddr, TracePtr tr,
                  SockMode mode = S_RDWR);
  virtual ~EIBNetIPSocket ();
  bool init ();
  void stop(bool err);

  /** enables multicast */
  bool SetMulticast (struct ip_mreqn multicastaddr);
  /** sends a packet */
  void Send (EIBNetIPPacket p, struct sockaddr_in addr);
  void Send (EIBNetIPPacket p)
  {
    Send (p, sendaddr);
  }

  /** get the port this socket is bound to (network byte order) */
  int port ();

  bool SetInterface(std::string& iface);

  /** default send address */
  struct sockaddr_in sendaddr;

  /** address to accept packets from, if recvall is 0 */
  struct sockaddr_in recvaddr;
  /** address to also accept packets from, if 'recvall' is 3 */
  struct sockaddr_in recvaddr2;
  /** address to NOT accept packets from, if 'recvall' is 2 */
  struct sockaddr_in localaddr;

  void pause();
  void unpause();
  bool paused;

  /** flag whether to accept (almost) all packets */
  uint8_t recvall;

private:
  /** debug output */
  TracePtr t;
  /** input */
  ev::io io_recv;
  void io_recv_cb (ev::io &w, int revents);
  /** output */
  ev::io io_send;
  void io_send_cb (ev::io &w, int revents);
  unsigned int send_error;

  void recv_cb(EIBNetIPPacket *p)
  {
    t->TracePacket (0, "Drop", p->data);
    delete p;
  }

  void error_cb()
  {
    stop(true);
  }
  void next_cb() { }

  /** output queue */
  Queue < struct _EIBNetIP_Send > send_q;
  void send_q_drop();

  /** multicast address */
  struct ip_mreqn maddr;
  /** file descriptor */
  int fd;
  /** multicast in use? */
  bool multicast;
};

#endif

/** @} */
