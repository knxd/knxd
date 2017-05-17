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

#include "connection.h"
#include "server.h"
#include "trace.h"

template<class C>
A_Base<C>::~A_Base()
{
}

A_Broadcast::A_Broadcast (ClientConnPtr cc) : A_Base<T_BroadcastPtr>(cc)
{
  t->setAuxName("Base");
  TRACEPRINTF (t, 7, "OpenBroadcast");
}

/* TODO refactor this mess */

bool
A_Broadcast::setup(uint8_t *buf,size_t len)
{
  if (len != 5)
    {
      TRACEPRINTF (t, 7, "OpenBroadcast size bad %d", len);
      return false;
    }
  lc = LinkConnectSinglePtr(new LinkConnectSingle(con->server, con->server->cfg, t));
  lc->setAddress(con->addr);
  c = T_BroadcastPtr(new T_Broadcast (this, lc, buf[4] != 0));
  lc->set_driver(c);
  if (!lc->setup())
    {
      TRACEPRINTF (t, 7, "OpenBroadcast init bad");
      return false;
    }
  con->sendmessage (2, buf);
  TRACEPRINTF (t, 7, "OpenBroadcast complete");
  return true;
}

A_Group::A_Group (ClientConnPtr cc) : A_Base(cc)
{
  t->setAuxName("Group");
  TRACEPRINTF (t, 7, "OpenGroup");
}

bool
A_Group::setup(uint8_t *buf,size_t len)
{
  if (len != 5)
    {
      TRACEPRINTF (t, 7, "OpenGroup size bad %d", len);
      return false;
    }
  lc = LinkConnectSinglePtr(new LinkConnectSingle(con->server, con->server->cfg, t));
  lc->setAddress(con->addr);
  c = T_GroupPtr(new T_Group (this, lc, (buf[2] << 8) | (buf[3]), buf[4] != 0));
  lc->set_driver(c);
  if (!lc->setup())
    {
      TRACEPRINTF (t, 7, "OpenGroup init bad");
      return false;
    }
  con->sendmessage (2, buf);
  TRACEPRINTF (t, 7, "OpenGroup complete");
  return true;
}

A_TPDU::A_TPDU (ClientConnPtr cc) : A_Base(cc)
{
  t->setAuxName("TPDU");
  TRACEPRINTF (t, 7, "OpenTPDU");
}
bool
A_TPDU::setup(uint8_t *buf,size_t len)
{
  if (len != 5)
    {
      TRACEPRINTF (t, 7, "OpenTPDU size bad %d", len);
      return false;
    }
  lc = LinkConnectSinglePtr(new LinkConnectSingle(con->server, con->server->cfg, t));
  lc->setAddress(con->addr);
  c = T_TPDUPtr(new T_TPDU (this, lc, (buf[2] << 8) | (buf[3])));
  lc->set_driver(c);
  if (!lc->setup())
    {
      TRACEPRINTF (t, 7, "OpenTPDU init bad");
      return false;
    }
  con->sendmessage (2, buf);
  TRACEPRINTF (t, 7, "OpenTPDU complete");
  return true;
}

A_Individual::A_Individual (ClientConnPtr cc) : A_Base(cc)
{
  t->setAuxName("Ind");
  TRACEPRINTF (t, 7, "OpenIndividual");
}

bool
A_Individual::setup(uint8_t *buf,size_t len)
{
  if (len != 5)
    {
      TRACEPRINTF (t, 7, "OpenIndividual size bad %d", len);
      return false;
    }
  lc = LinkConnectSinglePtr(new LinkConnectSingle(con->server, con->server->cfg, t));
  lc->setAddress(con->addr);
  c = T_IndividualPtr(
    new T_Individual (this, lc, (buf[2] << 8) | (buf[3]),
		      buf[4] != 0));
  lc->set_driver(c);
  if (!lc->setup())
    {
      TRACEPRINTF (t, 7, "OpenIndividual init bad");
      return false;
    }
  con->sendmessage (2, buf);
  TRACEPRINTF (t, 7, "OpenIndividual complete");
  return true;
}

A_Connection::A_Connection (ClientConnPtr cc) : A_Base(cc)
{
  t->setAuxName("Conn");
  TRACEPRINTF (t, 7, "OpenConnection");
}

bool
A_Connection::setup(uint8_t *buf,size_t len)
{
  if (len != 5)
    {
      TRACEPRINTF (t, 7, "OpenConnection size bad %d", len);
      return false;
    }
  lc = LinkConnectSinglePtr(new LinkConnectSingle(con->server, con->server->cfg, t));
  lc->setAddress(con->addr);
  c = T_ConnectionPtr(new T_Connection (this, lc, (buf[2] << 8) | (buf[3])));
  lc->set_driver(c);
  if (!lc->setup())
    {
      TRACEPRINTF (t, 7, "OpenConnection init bad");
      return false;
    }
  con->sendmessage (2, buf);
  TRACEPRINTF (t, 7, "OpenConnection complete");
  return true;
}

A_GroupSocket::A_GroupSocket (ClientConnPtr cc) : A_Base(cc)
{
  t->setAuxName("GS");
  TRACEPRINTF (t, 7, "OpenGroupSocket");
}

bool
A_GroupSocket::setup(uint8_t *buf,size_t len)
{
  if (len != 5)
    {
      TRACEPRINTF (t, 7, "OpenGroupSocket size bad %d", len);
      return false;
    }
  lc = LinkConnectSinglePtr(new LinkConnectSingle(con->server, con->server->cfg, t));
  lc->setAddress(con->addr);
  c = GroupSocketPtr(new GroupSocket (this, lc, buf[4] != 0));
  lc->set_driver(c);
  if (!lc->setup())
    {
      TRACEPRINTF (t, 7, "OpenGroupSocket init bad");
      return false;
    }
  con->sendmessage (2, buf);
  TRACEPRINTF (t, 7, "OpenGroupSocket complete");
  return true;
}

A_Broadcast::~A_Broadcast ()
{
  TRACEPRINTF (con->t, 7, "CloseBroadcast");
  if (c)
    c->stop();
}

A_Group::~A_Group ()
{
  TRACEPRINTF (con->t, 7, "CloseGroup");
  if (c)
    c->stop();
}

A_TPDU::~A_TPDU ()
{
  TRACEPRINTF (con->t, 7, "CloseTPDU");
  if (c)
    c->stop();
}

A_Individual::~A_Individual ()
{
  TRACEPRINTF (con->t, 7, "CloseIndividual");
  if (c)
    c->stop();
}

A_Connection::~A_Connection ()
{
  TRACEPRINTF (con->t, 7, "CloseConnection");
  if (c)
    c->stop();
}

A_GroupSocket::~A_GroupSocket ()
{
  TRACEPRINTF (con->t, 7, "CloseGroupSocket");
  if (c)
    c->stop();
}

void
A_Broadcast::recv_Data(uint8_t *buf, size_t len)
{
  if (len < 2 || EIBTYPE (buf) != EIB_APDU_PACKET)
    {
      on_error();
      return;
    }
  con->t->TracePacket (7, "recv Broadcast", len - 2, buf + 2);
  c->recv_Data (CArray (buf + 2, len - 2));
}

void
A_Group::recv_Data(uint8_t *buf, size_t len)
{
  if (len < 2 || EIBTYPE (buf) != EIB_APDU_PACKET)
    {
      on_error();
      return;
    }
  con->t->TracePacket (7, "recv Group", len - 2, buf + 2);
  c->recv_Data (CArray (buf + 2, len - 2));
}

void
A_TPDU::recv_Data(uint8_t *buf, size_t len)
{
  if (len < 4 || EIBTYPE (buf) != EIB_APDU_PACKET)
    {
      on_error();
      return;
    }
  con->t->TracePacket (7, "recv TPDU", len - 4, buf + 4);
  TpduComm p;
  p.data = CArray (buf + 4, len - 4);
  p.addr = (buf[2] << 8) | (buf[3]);
  c->recv_Data (p);
}

void
A_Individual::recv_Data(uint8_t *buf, size_t len)
{
  if (len < 2 || EIBTYPE (buf) != EIB_APDU_PACKET)
    {
      on_error();
      return;
    }
  con->t->TracePacket (7, "recv Indiv", len - 2, buf + 2);
  c->recv_Data (CArray (buf + 2, len - 2));
}

void
A_Connection::recv_Data(uint8_t *buf, size_t len)
{
  if (len < 2 || EIBTYPE (buf) != EIB_APDU_PACKET)
    {
      on_error();
      return;
    }
  con->t->TracePacket (7, "recv Conn", len - 2, buf + 2);
  c->recv_Data (CArray (buf + 2, len - 2));
}

void
A_GroupSocket::recv_Data(uint8_t *buf, size_t len)
{
  if (len < 4 || EIBTYPE (buf) != EIB_GROUP_PACKET)
    {
      on_error();
      return;
    }
  con->t->TracePacket (7, "recv GroupSock", len - 4, buf + 4);
  GroupAPDU p;
  p.data = CArray (buf + 4, len - 4);
  p.dst = (buf[2] << 8) | (buf[3]);
  c->recv_Data (p);
}

void
A_Broadcast::send (BroadcastComm &e)
{
  CArray res;
  res.resize (4 + e.data.size());
  EIBSETTYPE (res, EIB_APDU_PACKET);
  res[2] = (e.src >> 8) & 0xff;
  res[3] = (e.src) & 0xff;
  res.setpart (e.data.data(), 4, e.data.size());
  con->t->TracePacket (7, "Recv", e.data);
  con->sendmessage (res.size(), res.data());
}

void
A_Group::send (GroupComm &e)
{
  CArray res;
  res.resize (4 + e.data.size());
  EIBSETTYPE (res, EIB_APDU_PACKET);
  res[2] = (e.src >> 8) & 0xff;
  res[3] = (e.src) & 0xff;
  res.setpart (e.data.data(), 4, e.data.size());
  con->t->TracePacket (7, "Recv", e.data);
  con->sendmessage (res.size(), res.data());
}

void
A_TPDU::send (TpduComm &e)
{
  CArray res;
  res.resize (4 + e.data.size());
  EIBSETTYPE (res, EIB_APDU_PACKET);
  res[2] = (e.addr >> 8) & 0xff;
  res[3] = (e.addr) & 0xff;
  res.setpart (e.data.data(), 4, e.data.size());
  con->t->TracePacket (7, "Recv", e.data);
  con->sendmessage (res.size(), res.data());
}

void
A_Individual::send (CArray &e)
{
  CArray res;
  res.resize (2 + e.size());
  EIBSETTYPE (res, EIB_APDU_PACKET);
  res.setpart (e.data(), 2, e.size());
  con->t->TracePacket (7, "Recv", e);
  con->sendmessage (res.size(), res.data());
}

void
A_Connection::send (CArray &e)
{
  CArray res;
  res.resize (2 + e.size());
  EIBSETTYPE (res, EIB_APDU_PACKET);
  res.setpart (e.data(), 2, e.size());
  con->t->TracePacket (7, "Recv", e);
  con->sendmessage (res.size(), res.data());
}

void
A_GroupSocket::send (GroupAPDU &e)
{
  CArray res;
  res.resize (6 + e.data.size());
  EIBSETTYPE (res, EIB_GROUP_PACKET);
  res[2] = (e.src >> 8) & 0xff;
  res[3] = (e.src) & 0xff;
  res[4] = (e.dst >> 8) & 0xff;
  res[5] = (e.dst) & 0xff;
  res.setpart (e.data.data(), 6, e.data.size());
  con->t->TracePacket (7, "Recv", e.data);
  con->sendmessage (res.size(), res.data());
}

