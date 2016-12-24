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

#ifndef CONNECTION_H
#define CONNECTION_H

#include "layer4.h"
#include "client.h"

class A_Base
{
  void on_error() {}
protected:
  ClientConnPtr con = 0; // left NULL on constructor error
public:
  error_cb on_error_cb;

  A_Base() {
    on_error_cb.set<A_Base,&A_Base::on_error>(this);
  }
  virtual ~A_Base() { }
  inline bool is_connected() { return con != 0; }
  virtual void recv(uint8_t *buf, size_t len); // to socket
};

/** implements client interface to a broadcast connection */
class A_Broadcast:public T_Reader<BroadcastComm>, public A_Base
{
  T_BroadcastPtr c;

  const char *Name() { return "broadcast"; }
public:
  A_Broadcast (ClientConnPtr cc, uint8_t *buf,size_t len);
  ~A_Broadcast ();

  void send(BroadcastComm &); // from socket
  void recv(uint8_t *buf, size_t len); // to socket
};

/** implements client interface to a group connection */
class A_Group:public T_Reader<GroupComm>, public A_Base
{
  T_GroupPtr c;

  const char *Name() { return "group"; }
public:
  A_Group (ClientConnPtr cc, uint8_t *buf,size_t len);
  ~A_Group ();

  void send(GroupComm &); // from socket
  void recv(uint8_t *buf, size_t len); // to socket
};

/** implements client interface to a raw connection */
class A_TPDU:public T_Reader<TpduComm>, public A_Base
{
  T_TPDUPtr c;

  const char *Name() { return "tpdu"; }
public:
  A_TPDU (ClientConnPtr cc, uint8_t *buf,size_t len);
  ~A_TPDU ();

  void send(TpduComm &); // from socket
  void recv(uint8_t *buf, size_t len); // to socket
};

/** implements client interface to a T_Indivdual connection */
class A_Individual:public T_Reader<CArray>, public A_Base
{
  T_IndividualPtr c;

  const char *Name() { return "individual"; }
public:
  A_Individual (ClientConnPtr cc, uint8_t *buf,size_t len);
  ~A_Individual ();

  void send(CArray &); // from socket
  void recv(uint8_t *buf, size_t len); // to socket
};

/** implements client interface to a T_Connection connection */
class A_Connection:public T_Reader<CArray>, public A_Base
{
  T_ConnectionPtr c;

  const char *Name() { return "connection"; }
public:
  A_Connection (ClientConnPtr cc, uint8_t *buf,size_t len);
  ~A_Connection ();

  void send(CArray &); // from socket
  void recv(uint8_t *buf, size_t len); // to socket
};

/** implements client interface to a group socket */
class A_GroupSocket:public T_Reader<GroupAPDU>, public A_Base
{
  GroupSocketPtr c;

  const char *Name() { return "groupsocket"; }
public:
  A_GroupSocket (ClientConnPtr cc, uint8_t *buf,size_t len);
  ~A_GroupSocket ();

  /** start processing */
  void send(GroupAPDU &);
  void recv(uint8_t *buf, size_t len);
};

#endif
