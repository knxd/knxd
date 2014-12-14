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

T_Broadcast::T_Broadcast (Layer3 * l3, Trace * tr, int write_only)
{
  TRACEPRINTF (tr, 4, this, "OpenBroadcast %s", write_only ? "WO" : "RW");
  layer3 = l3;
  t = tr;
  pth_sem_init (&sem);
  init_ok = false;
  if (!write_only)
    if (!layer3->registerBroadcastCallBack (this))
      return;
  init_ok = true;
}

T_Broadcast::~T_Broadcast ()
{
  TRACEPRINTF (t, 4, this, "CloseBroadcast");
  layer3->deregisterBroadcastCallBack (this);
}

bool T_Broadcast::init ()
{
  return init_ok;
}

void
T_Broadcast::Get_L_Data (L_Data_PDU * l)
{
  BroadcastComm c;
  TPDU *t = TPDU::fromPacket (l->data);
  if (t->getType () == T_DATA_XXX_REQ)
    {
      T_DATA_XXX_REQ_PDU *t1 = (T_DATA_XXX_REQ_PDU *) t;
      c.data = t1->data;
      c.src = l->source;
      outqueue.put (c);
      pth_sem_inc (&sem, 0);
    }
  delete t;
  delete l;
}

void
T_Broadcast::Send (const CArray & c)
{
  T_DATA_XXX_REQ_PDU t;
  t.data = c;
  String s = t.Decode ();
  TRACEPRINTF (this->t, 4, this, "Send Broadcast %s", s ());
  L_Data_PDU *l = new L_Data_PDU;
  l->source = 0;
  l->dest = 0;
  l->AddrType = GroupAddress;
  l->data = t.ToPacket ();
  layer3->send_L_Data (l);
}

BroadcastComm *
T_Broadcast::Get (pth_event_t stop)
{
  pth_event_t s = pth_event (PTH_EVENT_SEM, &sem);

  pth_event_concat (s, stop, NULL);
  pth_wait (s);
  pth_event_isolate (s);

  if (pth_event_status (s) == PTH_STATUS_OCCURRED)
    {
      pth_sem_dec (&sem);
      BroadcastComm *c = new BroadcastComm (outqueue.get ());

      pth_event_free (s, PTH_FREE_THIS);
      t->TracePacket (4, this, "Recv Broadcast", c->data);
      return c;
    }
  pth_event_free (s, PTH_FREE_THIS);
  return 0;
}

T_Group::T_Group (Layer3 * l3, Trace * tr, eibaddr_t group, int write_only)
{
  TRACEPRINTF (tr, 4, this, "OpenGroup %d/%d/%d %s", (group >> 11) & 0x1f,
	       (group >> 8) & 0x07, (group) & 0xff, write_only ? "WO" : "RW");
  layer3 = l3;
  t = tr;
  groupaddr = group;
  pth_sem_init (&sem);
  init_ok = false;
  if (group == 0)
    return;

  if (!write_only)
    if (!layer3->registerGroupCallBack (this, group))
      return;
  init_ok = true;
}

bool T_Group::init ()
{
  return init_ok;
}

void
T_Group::Get_L_Data (L_Data_PDU * l)
{
  GroupComm c;
  TPDU *t = TPDU::fromPacket (l->data);
  if (t->getType () == T_DATA_XXX_REQ)
    {
      T_DATA_XXX_REQ_PDU *t1 = (T_DATA_XXX_REQ_PDU *) t;
      c.data = t1->data;
      c.src = l->source;
      outqueue.put (c);
      pth_sem_inc (&sem, 0);
    }
  delete t;
  delete l;
}

void
T_Group::Send (const CArray & c)
{
  T_DATA_XXX_REQ_PDU t;
  t.data = c;
  String s = t.Decode ();
  TRACEPRINTF (this->t, 4, this, "Send Group %s", s ());
  L_Data_PDU *l = new L_Data_PDU;
  l->source = 0;
  l->dest = groupaddr;
  l->AddrType = GroupAddress;
  l->data = t.ToPacket ();
  layer3->send_L_Data (l);
}

T_Group::~T_Group ()
{
  TRACEPRINTF (t, 4, this, "CloseGroup");
  layer3->deregisterGroupCallBack (this, groupaddr);
}

GroupComm *
T_Group::Get (pth_event_t stop)
{
  pth_event_t s = pth_event (PTH_EVENT_SEM, &sem);

  pth_event_concat (s, stop, NULL);
  pth_wait (s);
  pth_event_isolate (s);

  if (pth_event_status (s) == PTH_STATUS_OCCURRED)
    {
      pth_sem_dec (&sem);
      GroupComm *c = new GroupComm (outqueue.get ());

      pth_event_free (s, PTH_FREE_THIS);
      t->TracePacket (4, this, "Recv Group", c->data);
      return c;
    }
  pth_event_free (s, PTH_FREE_THIS);
  return 0;
}

T_TPDU::T_TPDU (Layer3 * l3, Trace * tr, eibaddr_t d)
{
  TRACEPRINTF (tr, 4, this, "OpenTPDU %d.%d.%d", (d >> 12) & 0x0f,
	       (d >> 8) & 0x0f, (d) & 0xff);
  layer3 = l3;
  t = tr;
  src = d;
  pth_sem_init (&sem);
  init_ok = false;
  if (!layer3->registerIndividualCallBack
      (this, Individual_Lock_None, 0, src))
    return;
  init_ok = true;
}

bool T_TPDU::init ()
{
  return init_ok;
}

void
T_TPDU::Get_L_Data (L_Data_PDU * l)
{
  TpduComm t;
  t.data = l->data;
  t.addr = l->source;
  outqueue.put (t);
  pth_sem_inc (&sem, 0);
  delete l;
}

void
T_TPDU::Send (const TpduComm & c)
{
  t->TracePacket (4, this, "Send TPDU", c.data);
  L_Data_PDU *l = new L_Data_PDU;
  l->source = src;
  l->dest = c.addr;
  l->AddrType = IndividualAddress;
  l->data = c.data;
  layer3->send_L_Data (l);
}

T_TPDU::~T_TPDU ()
{
  TRACEPRINTF (t, 4, this, "CloseTPDU");
  layer3->deregisterIndividualCallBack (this, 0, src);
}

TpduComm *
T_TPDU::Get (pth_event_t stop)
{
  pth_event_t s = pth_event (PTH_EVENT_SEM, &sem);

  pth_event_concat (s, stop, NULL);
  pth_wait (s);
  pth_event_isolate (s);

  if (pth_event_status (s) == PTH_STATUS_OCCURRED)
    {
      pth_sem_dec (&sem);
      TpduComm *c = new TpduComm (outqueue.get ());

      pth_event_free (s, PTH_FREE_THIS);
      t->TracePacket (4, this, "Recv TPDU", c->data);
      return c;
    }
  pth_event_free (s, PTH_FREE_THIS);
  return 0;
}

T_Individual::T_Individual (Layer3 * l3, Trace * tr, eibaddr_t d,
			    int write_only)
{
  TRACEPRINTF (tr, 4, this, "OpenIndividual %d.%d.%d %s", (d >> 12) & 0x0f,
	       (d >> 8) & 0x0f, (d) & 0xff, write_only ? "WO" : "RW");
  layer3 = l3;
  t = tr;
  dest = d;
  pth_sem_init (&sem);
  init_ok = false;
  if (!write_only)
    if (!layer3->registerIndividualCallBack
	(this, Individual_Lock_None, dest))
      return;
  init_ok = true;
}

bool T_Individual::init ()
{
  return init_ok;
}

void
T_Individual::Get_L_Data (L_Data_PDU * l)
{
  CArray c;
  TPDU *t = TPDU::fromPacket (l->data);
  if (t->getType () == T_DATA_XXX_REQ)
    {
      T_DATA_XXX_REQ_PDU *t1 = (T_DATA_XXX_REQ_PDU *) t;
      c = t1->data;
      outqueue.put (c);
      pth_sem_inc (&sem, 0);
    }
  delete t;
  delete l;
}

void
T_Individual::Send (const CArray & c)
{
  T_DATA_XXX_REQ_PDU t;
  t.data = c;
  String s = t.Decode ();
  TRACEPRINTF (this->t, 4, this, "Send Individual %s", s ());
  L_Data_PDU *l = new L_Data_PDU;
  l->source = 0;
  l->dest = dest;
  l->AddrType = IndividualAddress;
  l->data = t.ToPacket ();
  layer3->send_L_Data (l);
}

T_Individual::~T_Individual ()
{
  TRACEPRINTF (t, 4, this, "CloseIndividual");
  layer3->deregisterIndividualCallBack (this, dest);
}

CArray *
T_Individual::Get (pth_event_t stop)
{
  pth_event_t s = pth_event (PTH_EVENT_SEM, &sem);

  pth_event_concat (s, stop, NULL);
  pth_wait (s);
  pth_event_isolate (s);

  if (pth_event_status (s) == PTH_STATUS_OCCURRED)
    {
      pth_sem_dec (&sem);
      CArray *c = new CArray (outqueue.get ());

      pth_event_free (s, PTH_FREE_THIS);
      t->TracePacket (4, this, "Recv Individual", *c);
      return c;
    }
  pth_event_free (s, PTH_FREE_THIS);
  return 0;
}

T_Connection::T_Connection (Layer3 * l3, Trace * tr, eibaddr_t d)
{
  TRACEPRINTF (tr, 4, this, "OpenConnection %d.%d.%d", (d >> 12) & 0x0f,
	       (d >> 8) & 0x0f, (d) & 0xff);
  layer3 = l3;
  t = tr;
  dest = d;
  pth_sem_init (&insem);
  pth_sem_init (&outsem);
  pth_sem_init (&bufsem);
  recvno = 0;
  sendno = 0;
  mode = 0;
  init_ok = false;
  if (!layer3->registerIndividualCallBack
      (this, Individual_Lock_Connection, dest))
    return;
  Start ();
  init_ok = true;
}

T_Connection::~T_Connection ()
{
  TRACEPRINTF (t, 4, this, "CloseConnection");
  Stop ();
  while (!buf.isempty ())
    delete buf.get ();
  layer3->deregisterIndividualCallBack (this, dest);
}

bool T_Connection::init ()
{
  return init_ok;
}

void
T_Connection::Get_L_Data (L_Data_PDU * l)
{
  buf.put (l);
  pth_sem_inc (&bufsem, 0);
}

void
T_Connection::Send (const CArray & c)
{
  t->TracePacket (4, this, "Send", c);
  in.put (c);
  pth_sem_inc (&insem, 1);
}

CArray *
T_Connection::Get (pth_event_t stop)
{
  pth_event_t s = pth_event (PTH_EVENT_SEM, &outsem);

  pth_event_concat (s, stop, NULL);
  pth_wait (s);
  pth_event_isolate (s);

  if (pth_event_status (s) == PTH_STATUS_OCCURRED)
    {
      pth_sem_dec (&outsem);
      CArray *c = new CArray (out.get ());

      pth_event_free (s, PTH_FREE_THIS);
      t->TracePacket (4, this, "RecvConnection", *c);
      return c;
    }
  pth_event_free (s, PTH_FREE_THIS);
  return 0;
}

void
T_Connection::SendConnect ()
{
  TRACEPRINTF (t, 4, this, "SendConnect");
  T_CONNECT_REQ_PDU p;
  L_Data_PDU *l = new L_Data_PDU;
  l->source = 0;
  l->dest = dest;
  l->AddrType = IndividualAddress;
  l->data = p.ToPacket ();
  l->prio = PRIO_SYSTEM;
  layer3->send_L_Data (l);
}

void
T_Connection::SendDisconnect ()
{
  TRACEPRINTF (t, 4, this, "SendDisconnect");
  T_DISCONNECT_REQ_PDU p;
  L_Data_PDU *l = new L_Data_PDU;
  l->source = 0;
  l->dest = dest;
  l->AddrType = IndividualAddress;
  l->data = p.ToPacket ();
  l->prio = PRIO_SYSTEM;
  layer3->send_L_Data (l);
}

void
T_Connection::SendAck (int serno)
{
  TRACEPRINTF (t, 4, this, "SendACK %d", serno);
  T_ACK_PDU p;
  p.serno = serno;
  L_Data_PDU *l = new L_Data_PDU;
  l->source = 0;
  l->dest = dest;
  l->AddrType = IndividualAddress;
  l->data = p.ToPacket ();
  layer3->send_L_Data (l);
}

void
T_Connection::SendData (int serno, const CArray & c)
{
  T_DATA_CONNECTED_REQ_PDU p;
  p.data = c;
  p.serno = serno;
  TRACEPRINTF (t, 4, this, "SendData %s", p.Decode ()());
  L_Data_PDU *l = new L_Data_PDU;
  l->source = 0;
  l->dest = dest;
  l->AddrType = IndividualAddress;
  l->data = p.ToPacket ();
  layer3->send_L_Data (l);
}

/*
 * States:
 * 0   CLOSED
 * 1   IDLE
 * 2   ACK_WAIT
 *
*/

void
T_Connection::Run (pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  pth_event_t inev = pth_event (PTH_EVENT_SEM, &insem);
  pth_event_t bufev = pth_event (PTH_EVENT_SEM, &bufsem);

  while (!buf.isempty ())
    delete buf.get ();

  pth_sem_set_value (&bufsem, 0);

  mode = 0;
  SendConnect ();
  mode = 1;
  sendno = 0;
  recvno = 0;
  repcount = 0;
  pth_event_t timeout = pth_event (PTH_EVENT_RTIME, pth_time (6, 0));
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED && mode != 0)
    {
      pth_event_concat (bufev, stop, timeout, NULL);
      if (mode == 1)
	pth_event_concat (bufev, inev, NULL);

      pth_wait (bufev);

      pth_event_isolate (bufev);
      pth_event_isolate (inev);
      pth_event_isolate (timeout);
      if (pth_event_status (bufev) == PTH_STATUS_OCCURRED)
	{
	  pth_sem_dec (&bufsem);
	  L_Data_PDU *l = buf.get ();
	  TPDU *t = TPDU::fromPacket (l->data);
	  switch (t->getType ())
	    {
	    case T_DISCONNECT_REQ:
	      mode = 0;
	      break;
	    case T_CONNECT_REQ:
	      mode = 0;
	      break;
	    case T_DATA_CONNECTED_REQ:
	      {
		T_DATA_CONNECTED_REQ_PDU *t1 = (T_DATA_CONNECTED_REQ_PDU *) t;
		if (t1->serno != recvno && t1->serno != ((recvno - 1) & 0x0f))
		  mode = 0;
		else if (t1->serno == recvno)
		  {
		    t1->data[0] = t1->data[0] & 0x03;
		    out.put (t1->data);
		    pth_sem_inc (&outsem, 0);
		    SendAck (recvno);
		    recvno = (recvno + 1) & 0x0f;
		  }
		else if (t1->serno == ((recvno - 1) & 0x0f))
		  SendAck (t1->serno);

		if (mode == 1)
		  timeout =
		    pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout,
			       pth_time (6, 0));
	      }
	      break;
	    case T_NACK:
	      {
		T_NACK_PDU *t1 = (T_NACK_PDU *) t;
		if (t1->serno != sendno)
		  mode = 0;
		else if (in.isempty ())
		  mode = 0;
		else if (repcount >= 3 || mode != 2)
		  mode = 0;
		else
		  {
		    repcount++;
		    SendData (sendno, in.top ());
		    timeout =
		      pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout,
				 pth_time (3, 0));
		  }
	      }
	      break;
	    case T_ACK:
	      {
		T_ACK_PDU *t1 = (T_ACK_PDU *) t;
		if (t1->serno != sendno)
		  mode = 0;
		else if (mode != 2)
		  mode = 0;
		else
		  {
		    timeout =
		      pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout,
				 pth_time (6, 0));
		    mode = 1;
		    in.get ();
		    sendno = (sendno + 1) & 0x0f;
		  }
	      }
	      break;
	    default:
	      /* ignore */ ;
	    }
	  delete t;
	  delete l;
	}
      else if (pth_event_status (inev) == PTH_STATUS_OCCURRED && mode == 1)
	{
	  repcount = 0;
	  pth_sem_dec (&insem);
	  SendData (sendno, in.top ());
	  mode = 2;
	  timeout =
	    pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout,
		       pth_time (3, 0));
	}
      else if (pth_event_status (timeout) == PTH_STATUS_OCCURRED)
	{
	  if (mode == 2 && repcount < 3)
	    {
	      repcount++;
	      SendData (sendno, in.top ());
	      timeout =
		pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout,
			   pth_time (3, 0));
	    }
	  else
	    mode = 0;
	}
    }
  pth_event_free (stop, PTH_FREE_THIS);
  pth_event_free (inev, PTH_FREE_THIS);
  pth_event_free (bufev, PTH_FREE_THIS);
  pth_event_free (timeout, PTH_FREE_THIS);
  SendDisconnect ();
  mode = 0;
  layer3->deregisterIndividualCallBack (this, dest);
  out.put (CArray ());
  pth_sem_inc (&outsem, 0);
}

GroupSocket::GroupSocket (Layer3 * l3, Trace * tr, int write_only)
{
  TRACEPRINTF (tr, 4, this, "OpenGroupSocket %s", write_only ? "WO" : "RW");
  layer3 = l3;
  t = tr;
  pth_sem_init (&sem);
  init_ok = false;
  if (!write_only)
    if (!layer3->registerGroupCallBack (this, 0))
      return;
  init_ok = true;
}

GroupSocket::~GroupSocket ()
{
  TRACEPRINTF (t, 4, this, "CloseGroupSocket");
  layer3->deregisterGroupCallBack (this, 0);
}

bool GroupSocket::init ()
{
  return init_ok;
}

void
GroupSocket::Get_L_Data (L_Data_PDU * l)
{
  GroupAPDU c;
  TPDU *t = TPDU::fromPacket (l->data);
  if (t->getType () == T_DATA_XXX_REQ)
    {
      T_DATA_XXX_REQ_PDU *t1 = (T_DATA_XXX_REQ_PDU *) t;
      c.data = t1->data;
      c.src = l->source;
      c.dst = l->dest;
      outqueue.put (c);
      pth_sem_inc (&sem, 0);
    }
  delete t;
  delete l;
}

void
GroupSocket::Send (const GroupAPDU & c)
{
  T_DATA_XXX_REQ_PDU t;
  t.data = c.data;
  String s = t.Decode ();
  TRACEPRINTF (this->t, 4, this, "Send GroupSocket %s", s ());
  L_Data_PDU *l = new L_Data_PDU;
  l->source = 0;
  l->dest = c.dst;
  l->AddrType = GroupAddress;
  l->data = t.ToPacket ();
  layer3->send_L_Data (l);
}

GroupAPDU *
GroupSocket::Get (pth_event_t stop)
{
  pth_event_t s = pth_event (PTH_EVENT_SEM, &sem);

  pth_event_concat (s, stop, NULL);
  pth_wait (s);
  pth_event_isolate (s);

  if (pth_event_status (s) == PTH_STATUS_OCCURRED)
    {
      pth_sem_dec (&sem);
      GroupAPDU *c = new GroupAPDU (outqueue.get ());

      pth_event_free (s, PTH_FREE_THIS);
      t->TracePacket (4, this, "Recv GroupSocket", c->data);
      return c;
    }
  pth_event_free (s, PTH_FREE_THIS);
  return 0;
}
