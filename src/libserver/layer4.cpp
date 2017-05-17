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

#include "layer4.h"
#include "tpdu.h"

/***************** Layer4Common *****************/

template<class COMM>
Layer4common<COMM>::~Layer4common() {}

/***************** T_Brodcast *****************/

T_Broadcast::T_Broadcast (T_Reader<BroadcastComm> *app, LinkConnectClientPtr lc, bool write_only)
  : Layer4commonWO(app, lc,write_only)
{
  t->setAuxName("TBr");
  TRACEPRINTF (t, 4, "OpenBroadcast %s", write_only ? "WO" : "RW");
}

T_Broadcast::~T_Broadcast ()
{
  TRACEPRINTF (t, 4, "CloseBroadcast");
}

void
T_Broadcast::send_L_Data (LDataPtr l)
{
  BroadcastComm c;
  TPDUPtr t = TPDU::fromPacket (l->data, this->t);
  if (t->getType () == T_DATA_XXX_REQ)
    {
      T_DATA_XXX_REQ_PDU *t1 = (T_DATA_XXX_REQ_PDU *) &*t;
      c.data = t1->data;
      c.src = l->source;
      app->send(c);
    }
  send_Next();
}

void
T_Broadcast::recv_Data (const CArray & c)
{
  T_DATA_XXX_REQ_PDU t;
  t.data = c;
  String s = t.Decode (this->t);
  TRACEPRINTF (this->t, 4, "Recv Broadcast %s", s);
  LDataPtr l = LDataPtr(new L_Data_PDU ());
  l->source = 0;
  l->dest = 0;
  l->AddrType = GroupAddress;
  l->data = t.ToPacket ();
  l->hopcount = 0x07;
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Data (std::move(l));
}

/***************** T_Group *****************/

T_Group::T_Group (T_Reader<GroupComm> *app, LinkConnectClientPtr lc, eibaddr_t group, bool write_only)
  : Layer4commonWO(app, lc, write_only)
{
  t->setAuxName("TGr");
  TRACEPRINTF (t, 4, "OpenGroup %s %s", FormatGroupAddr (group),
	       write_only ? "WO" : "RW");
  groupaddr = group;
}

void
T_Group::send_L_Data (LDataPtr l)
{
  GroupComm c;
  TPDUPtr t = TPDU::fromPacket (l->data, this->t);
  if (t->getType () == T_DATA_XXX_REQ)
    {
      T_DATA_XXX_REQ_PDU *t1 = (T_DATA_XXX_REQ_PDU *) &*t;
      c.data = t1->data;
      c.src = l->source;
      app->send(c);
    }
  send_Next();
}

void
T_Group::recv_Data (const CArray & c)
{
  T_DATA_XXX_REQ_PDU t;
  t.data = c;
  String s = t.Decode (this->t);
  TRACEPRINTF (this->t, 4, "Recv Group %s", s);
  LDataPtr l = LDataPtr(new L_Data_PDU ());
  l->source = 0;
  l->dest = groupaddr;
  l->AddrType = GroupAddress;
  l->data = t.ToPacket ();
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Data (std::move(l));
}

T_Group::~T_Group ()
{
  TRACEPRINTF (t, 4, "CloseGroup");
}

/***************** T_TPDU *****************/

T_TPDU::T_TPDU (T_Reader<TpduComm> *app, LinkConnectClientPtr lc, eibaddr_t src)
  : Layer4common(app, lc)
{
  t->setAuxName("TPdu");
  TRACEPRINTF (t, 4, "OpenTPDU %s", FormatEIBAddr (src));
  this->src = src;
}

void
T_TPDU::send_L_Data (LDataPtr l)
{
  TpduComm t;
  t.data = l->data;
  t.addr = l->source;
  app->send(t);
  send_Next();
}

void
T_TPDU::recv_Data (const TpduComm & c)
{
  t->TracePacket (4, "Recv TPDU", c.data);
  LDataPtr l = LDataPtr(new L_Data_PDU ());
  l->source = src;
  l->dest = c.addr;
  l->AddrType = IndividualAddress;
  l->data = c.data;
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Data (std::move(l));
}

T_TPDU::~T_TPDU ()
{
  TRACEPRINTF (t, 4, "CloseTPDU");
}

/***************** T_Individual *****************/

T_Individual::T_Individual (T_Reader<CArray> *app, LinkConnectClientPtr lc, eibaddr_t dest, bool write_only)
  : Layer4commonWO(app, lc, write_only)
{
  t->setAuxName("TInd");
  TRACEPRINTF (t, 4, "OpenIndividual %s %s",
               FormatEIBAddr (dest).c_str(), write_only ? "WO" : "RW");
  this->dest = dest;
}

void
T_Individual::send_L_Data (LDataPtr l)
{
  CArray c;
  TPDUPtr t = TPDU::fromPacket (l->data, this->t);
  if (t->getType () == T_DATA_XXX_REQ)
    {
      T_DATA_XXX_REQ_PDU *t1 = (T_DATA_XXX_REQ_PDU *) &*t;
      c = t1->data;
      app->send(c);
    }
  send_Next();
}

void
T_Individual::recv_Data (const CArray & c)
{
  T_DATA_XXX_REQ_PDU t;
  t.data = c;
  String s = t.Decode (this->t);
  TRACEPRINTF (this->t, 4, "Recv Individual %s", s);
  LDataPtr l = LDataPtr(new L_Data_PDU ());
  l->source = 0;
  l->dest = dest;
  l->AddrType = IndividualAddress;
  l->data = t.ToPacket ();
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Data (std::move(l));
}

T_Individual::~T_Individual ()
{
  TRACEPRINTF (t, 4, "CloseIndividual");
}

/***************** T_Connection *****************/

T_Connection::T_Connection (T_Reader<CArray> *app, LinkConnectClientPtr lc, eibaddr_t d)
	: Layer4common (app, lc)
{
  t->setAuxName("TConn");
  TRACEPRINTF (t, 4, "OpenConnection %s", FormatEIBAddr (d));
  timer.set <T_Connection, &T_Connection::timer_cb> (this);

  dest = d;
  recvno = 0;
  sendno = 0;
  mode = 0;
}

T_Connection::~T_Connection ()
{
  TRACEPRINTF (t, 4, "CloseConnection");
  stop ();
  while (!buf.isempty ())
    delete buf.get ();
}

void
T_Connection::send_L_Data (LDataPtr l)
{
  TPDUPtr t = TPDU::fromPacket (l->data, this->t);
  switch (t->getType ())
    {
    case T_DISCONNECT_REQ:
      stop();
      break;
    case T_CONNECT_REQ:
      stop();
      break;
    case T_DATA_CONNECTED_REQ:
      {
        T_DATA_CONNECTED_REQ_PDU *t1 = (T_DATA_CONNECTED_REQ_PDU *) &*t;
        if (t1->serno != recvno && t1->serno != ((recvno - 1) & 0x0f))
          stop();
        else if (t1->serno == recvno)
          {
            t1->data[0] = t1->data[0] & 0x03;
            app->send(t1->data);
            SendAck (recvno);
            recvno = (recvno + 1) & 0x0f;
          }
        else if (t1->serno == ((recvno - 1) & 0x0f))
          SendAck (t1->serno);
      }
      break;
    case T_NACK:
      {
        T_NACK_PDU *t1 = (T_NACK_PDU *) &*t;
        if (t1->serno != sendno)
          stop();
        else if (repcount >= 3 || mode != 2)
          stop();
        else
          {
            repcount++;
            SendData (sendno, current);
          }
      }
      break;
    case T_ACK:
      {
        T_ACK_PDU *t1 = (T_ACK_PDU *) &*t;
        if (t1->serno != sendno)
          stop();
        else if (mode != 2)
          stop();
        else
          {
            mode = 1;
            timer.stop();
            sendno = (sendno + 1) & 0x0f;
            SendCheck();
          }
      }
      break;
    default:
      /* ignore */ ;
    }
  send_Next();
}

void
T_Connection::recv_Data (const CArray & c)
{
  t->TracePacket (4, "Recv", c);
  in.push (c);
  SendCheck();
}

void
T_Connection::SendCheck ()
{
  if (mode != 1)
    return;
  repcount = 0;
  if (in.isempty())
    return;
  current = in.get();
  SendData (sendno, current);
  mode = 2;
  timer.start(3,0);
}

void
T_Connection::SendConnect ()
{
  TRACEPRINTF (t, 4, "SendConnect");
  T_CONNECT_REQ_PDU p;
  LDataPtr l = LDataPtr(new L_Data_PDU ());
  l->source = 0;
  l->dest = dest;
  l->AddrType = IndividualAddress;
  l->data = p.ToPacket ();
  l->prio = PRIO_SYSTEM;

  mode = 1;
  sendno = 0;
  recvno = 0;
  repcount = 0;
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Data (std::move(l));
}

void
T_Connection::SendDisconnect ()
{
  TRACEPRINTF (t, 4, "SendDisconnect");
  T_DISCONNECT_REQ_PDU p;
  LDataPtr l = LDataPtr(new L_Data_PDU ());
  l->source = 0;
  l->dest = dest;
  l->AddrType = IndividualAddress;
  l->data = p.ToPacket ();
  l->prio = PRIO_SYSTEM;
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Data (std::move(l));
}

void
T_Connection::SendAck (int serno)
{
  TRACEPRINTF (t, 4, "SendACK %d", serno);
  T_ACK_PDU p;
  p.serno = serno;
  LDataPtr l = LDataPtr(new L_Data_PDU ());
  l->source = 0;
  l->dest = dest;
  l->AddrType = IndividualAddress;
  l->data = p.ToPacket ();
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Data (std::move(l));
}

void
T_Connection::SendData (int serno, const CArray & c)
{
  T_DATA_CONNECTED_REQ_PDU p;
  p.data = c;
  p.serno = serno;
  TRACEPRINTF (t, 4, "SendData %s", p.Decode (t));
  LDataPtr l = LDataPtr(new L_Data_PDU ());
  l->source = 0;
  l->dest = dest;
  l->AddrType = IndividualAddress;
  l->data = p.ToPacket ();
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Data (std::move(l));
}

/*
 * States:
 * 0   CLOSED
 * 1   IDLE
 * 2   ACK_WAIT
 *
*/

void T_Connection::timer_cb (ev::timer &w UNUSED, int revents UNUSED)
{
  if (mode == 2 && repcount < 3)
    {
      repcount++;
      SendData (sendno, current);
      timer.start(3,0);
    }
  else
    stop();
}

void
T_Connection::stop()
{
  mode = 0;
  timer.stop();
  SendDisconnect ();

  CArray C;
  app->send(C);
}

/***************** GroupSocket *****************/

GroupSocket::GroupSocket (T_Reader<GroupAPDU> *app, LinkConnectClientPtr lc, bool write_only)
  : Layer4commonWO(app, lc, write_only)
{
  TRACEPRINTF (t, 4, "OpenGroupSocket %s", write_only ? "WO" : "RW");
}

GroupSocket::~GroupSocket ()
{
  TRACEPRINTF (t, 4, "CloseGroupSocket");
}

void
GroupSocket::send_L_Data (LDataPtr l)
{
  GroupAPDU c;
  TPDUPtr t = TPDU::fromPacket (l->data, this->t);
  if (t->getType () == T_DATA_XXX_REQ)
    {
      T_DATA_XXX_REQ_PDU *t1 = (T_DATA_XXX_REQ_PDU *) &*t;
      c.data = t1->data;
      c.src = l->source;
      c.dst = l->dest;
      app->send(c);
    }
  send_Next();
}

void
GroupSocket::recv_Data (const GroupAPDU & c)
{
  T_DATA_XXX_REQ_PDU t;
  t.data = c.data;
  String s = t.Decode (this->t);
  TRACEPRINTF (this->t, 4, "Recv GroupSocket %s %s", FormatGroupAddr(c.dst), s);
  LDataPtr l = LDataPtr(new L_Data_PDU ());
  l->source = 0;
  l->dest = c.dst;
  l->AddrType = GroupAddress;
  l->data = t.ToPacket ();
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Data (std::move(l));
}

