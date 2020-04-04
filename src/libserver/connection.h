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
 * @ingroup KNX_03_03_07
 * Application Layer
 * @{
 */

#ifndef CONNECTION_H
#define CONNECTION_H

#include "client.h"
#include "layer4.h"

class A__Base
{
public:
  TracePtr t;
  A__Base(ClientConnPtr cc)
  {
    t = TracePtr(new Trace(*(cc->t), cc->t->name+'@'+FormatEIBAddr(cc->addr)));
    con = cc;
    on_error.set<A__Base,&A__Base::error_cb>(this);
  }
  virtual ~A__Base() = default;

  ClientConnPtr con;
  LinkConnectSinglePtr lc;
  InfoCallback on_error;
  void error_cb() {}

  virtual void recv_Data(uint8_t *buf, size_t len) = 0; // to socket

  virtual bool setup (uint8_t *buf,size_t len) = 0;
  virtual void start() { }
  virtual void stop(bool err) { }
};

template<class TC>
class A_Base : public A__Base
{
public:

  A_Base(ClientConnPtr cc) : A__Base(cc)
  {
    t->setAuxName("Base");
  }
  virtual ~A_Base();

protected:
  TC c = nullptr;
};

/** implements client interface to a broadcast connection */
class A_Broadcast : public T_Reader<BroadcastComm>, public A_Base<T_BroadcastPtr>
{
public:
  A_Broadcast (ClientConnPtr cc);
  virtual ~A_Broadcast ();
  virtual bool setup (uint8_t *buf,size_t len) override;

  virtual void recv_Data(uint8_t *buf, size_t len) override; // to socket
  void send(BroadcastComm &); // from socket

private:
  const char *Name() const
  {
    return "broadcast";
  }
};

/** implements client interface to a group connection */
class A_Group : public T_Reader<GroupComm>, public A_Base<T_GroupPtr>
{
public:
  A_Group (ClientConnPtr cc);
  virtual ~A_Group ();
  virtual bool setup (uint8_t *buf,size_t len) override;

  virtual void recv_Data(uint8_t *buf, size_t len) override; // to socket
  void send(GroupComm &); // from socket

private:
  const char *Name() const
  {
    return "group";
  }
};

/** implements client interface to a raw connection */
class A_TPDU : public T_Reader<TpduComm>, public A_Base<T_TPDUPtr>
{
public:
  A_TPDU (ClientConnPtr cc);
  virtual ~A_TPDU ();
  virtual bool setup (uint8_t *buf,size_t len) override;

  virtual void recv_Data(uint8_t *buf, size_t len) override; // to socket
  void send(TpduComm &); // from socket

private:
  const char *Name() const
  {
    return "tpdu";
  }
};

/** implements client interface to a T_Indivdual connection */
class A_Individual : public T_Reader<CArray>, public A_Base<T_IndividualPtr>
{
public:
  A_Individual (ClientConnPtr cc);
  virtual ~A_Individual ();
  virtual bool setup (uint8_t *buf,size_t len) override;

  virtual void recv_Data(uint8_t *buf, size_t len) override; // to socket
  void send(CArray &); // from socket

private:
  const char *Name() const
  {
    return "individual";
  }
};

/** implements client interface to a T_Connection connection */
class A_Connection : public T_Reader<CArray>, public A_Base<T_ConnectionPtr>
{
public:
  A_Connection (ClientConnPtr cc);
  virtual ~A_Connection ();
  virtual bool setup(uint8_t *buf,size_t len) override;

  virtual void recv_Data(uint8_t *buf, size_t len) override; // to socket
  void send(CArray &); // from socket

private:
  const char *Name() const
  {
    return "connection";
  }
};

/** implements client interface to a group socket */
class A_GroupSocket : public T_Reader<GroupAPDU>, public A_Base<GroupSocketPtr>
{
public:
  A_GroupSocket (ClientConnPtr cc);
  virtual ~A_GroupSocket ();
  virtual bool setup(uint8_t *buf,size_t len) override;

  virtual void recv_Data(uint8_t *buf, size_t len) override;
  /** start processing */
  void send(GroupAPDU &);

private:
  const char *Name() const
  {
    return "groupsocket";
  }
};

#endif

/** @} */
