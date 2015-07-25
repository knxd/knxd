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

A_Broadcast::A_Broadcast (ClientConnection * cc)
{
  con = cc;
  TRACEPRINTF (con->t, 7, this, "OpenBroadcast");
  c = nullptr;
  if (con->size != 5)
    {
      TRACEPRINTF (con->t, 7, this, "OpenBroadcast size bad %d", con->size);
      return;
    }
  c = T_BroadcastPtr(new T_Broadcast (con->l3, con->t, con->buf[4] != 0 ? 1 : 0));
  if (!c->init ())
    {
      TRACEPRINTF (con->t, 7, this, "OpenBroadcast init bad");
      return;
    }
  Start ();
  TRACEPRINTF (con->t, 7, this, "OpenBroadcast complete");
}

A_Group::A_Group (ClientConnection * cc)
{
  con = cc;
  TRACEPRINTF (con->t, 7, this, "OpenGroup");
  c = nullptr;
  if (con->size != 5)
    {
      TRACEPRINTF (con->t, 7, this, "OpenGroup size bad %d", con->size);
      return;
    }
  c = T_GroupPtr(new T_Group (con->l3, cc->t, (con->buf[2] << 8) | (con->buf[3]),
		 con->buf[4] != 0 ? 1 : 0));
  if (!c->init ())
    {
      TRACEPRINTF (con->t, 7, this, "OpenGroup init bad");
      return;
    }
  Start ();
  TRACEPRINTF (con->t, 7, this, "OpenGroup complete");
}

A_TPDU::A_TPDU (ClientConnection * cc)
{
  con = cc;
  TRACEPRINTF (con->t, 7, this, "OpenTPDU");
  c = nullptr;
  if (con->size != 5)
    {
      TRACEPRINTF (con->t, 7, this, "OpenTPDU size bad %d", con->size);
      return;
    }
  c = T_TPDUPtr(new T_TPDU (con->l3, con->t, (con->buf[2] << 8) | (con->buf[3])));
  if (!c->init ())
    {
      TRACEPRINTF (con->t, 7, this, "OpenTPDU init bad");
      return;
    }
  Start ();
  TRACEPRINTF (con->t, 7, this, "OpenTPDU complete");
}

A_Individual::A_Individual (ClientConnection * cc)
{
  con = cc;
  TRACEPRINTF (con->t, 7, this, "OpenIndividual");
  c = nullptr;
  if (con->size != 5)
    {
      TRACEPRINTF (con->t, 7, this, "OpenIndividual size bad %d", con->size);
      return;
    }
  c = T_IndividualPtr(
    new T_Individual (con->l3, con->t, (con->buf[2] << 8) | (con->buf[3]),
		      con->buf[4] != 0 ? 1 : 0));
  if (!c->init ())
    {
      TRACEPRINTF (con->t, 7, this, "OpenIndividual init bad");
      return;
    }
  Start ();
  TRACEPRINTF (con->t, 7, this, "OpenIndividual complete");
}

A_Connection::A_Connection (ClientConnection * cc)
{
  con = cc;
  TRACEPRINTF (con->t, 7, this, "OpenConnection");
  c = nullptr;
  if (con->size != 5)
    {
      TRACEPRINTF (con->t, 7, this, "OpenConnection size bad %d", con->size);
      return;
    }
  c = T_ConnectionPtr(new T_Connection (con->l3, con->t, (con->buf[2] << 8) | (con->buf[3])));
  if (!c->init ())
    {
      TRACEPRINTF (con->t, 7, this, "OpenConnection init bad");
      return;
    }
  Start ();
  TRACEPRINTF (con->t, 7, this, "OpenConnection complete");
}

A_GroupSocket::A_GroupSocket (ClientConnection * cc)
{
  con = cc;
  TRACEPRINTF (con->t, 7, this, "OpenGroupSocket");
  c = nullptr;
  if (con->size != 5)
    {
      TRACEPRINTF (con->t, 7, this, "OpenGroupSocket size bad %d", con->size);
      return;
    }
  c = GroupSocketPtr(new GroupSocket (con->l3, con->t, con->buf[4] != 0 ? 1 : 0));
  if (!c->init ())
    {
      TRACEPRINTF (con->t, 7, this, "OpenGroupSocket init bad");
      return;
    }
  Start ();
  TRACEPRINTF (con->t, 7, this, "OpenGroupSocket complete");
}

A_Broadcast::~A_Broadcast ()
{
  TRACEPRINTF (con->t, 7, this, "CloseBroadcast");
  Stop ();
}

A_Group::~A_Group ()
{
  TRACEPRINTF (con->t, 7, this, "CloseGroup");
  Stop ();
}

A_TPDU::~A_TPDU ()
{
  TRACEPRINTF (con->t, 7, this, "CloseTPDU");
  Stop ();
}

A_Individual::~A_Individual ()
{
  TRACEPRINTF (con->t, 7, this, "CloseIndividual");
  Stop ();
}

A_Connection::~A_Connection ()
{
  TRACEPRINTF (con->t, 7, this, "CloseConnection");
  Stop ();
}

A_GroupSocket::~A_GroupSocket ()
{
  TRACEPRINTF (con->t, 7, this, "CloseGroupSocket");
  Stop ();
}

void
A_Broadcast::Do (pth_event_t stop)
{
  if (!c)
    {
      con->sendreject (stop, EIB_PROCESSING_ERROR);
      return;
    }
  if (con->sendmessage (2, con->buf, stop) == -1)
    return;
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      if (con->readmessage (stop) == -1)
	break;
      if (EIBTYPE (con->buf) == EIB_RESET_CONNECTION)
	break;
      if (con->size >= 2)
	{
	  if (EIBTYPE (con->buf) != EIB_APDU_PACKET)
	    break;
	  con->t->TracePacket (7, this, "Send", con->size - 2, con->buf + 2);
	  c->Send (CArray (con->buf + 2, con->size - 2));
	}
    }
}

void
A_Group::Do (pth_event_t stop)
{
  if (!c)
    {
      con->sendreject (stop, EIB_PROCESSING_ERROR);
      return;
    }
  if (con->sendmessage (2, con->buf, stop) == -1)
    return;
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      if (con->readmessage (stop) == -1)
	break;
      if (EIBTYPE (con->buf) == EIB_RESET_CONNECTION)
	break;
      if (con->size >= 2)
	{
	  if (EIBTYPE (con->buf) != EIB_APDU_PACKET)
	    break;
	  con->t->TracePacket (7, this, "Send", con->size - 2, con->buf + 2);
	  c->Send (CArray (con->buf + 2, con->size - 2));
	}
    }
}

void
A_TPDU::Do (pth_event_t stop)
{
  if (!c)
    {
      con->sendreject (stop, EIB_PROCESSING_ERROR);
      return;
    }
  if (con->sendmessage (2, con->buf, stop) == -1)
    return;
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      if (con->readmessage (stop) == -1)
	break;
      if (EIBTYPE (con->buf) == EIB_RESET_CONNECTION)
	break;
      if (con->size >= 4)
	{
	  if (EIBTYPE (con->buf) != EIB_APDU_PACKET)
	    break;
	  con->t->TracePacket (7, this, "Send", con->size - 4, con->buf + 4);
	  TpduComm p;
	  p.data = CArray (con->buf + 4, con->size - 4);
	  p.addr = (con->buf[2] << 8) | (con->buf[3]);
	  c->Send (p);
	}
    }
}

void
A_Individual::Do (pth_event_t stop)
{
  if (!c)
    {
      con->sendreject (stop, EIB_PROCESSING_ERROR);
      return;
    }
  if (con->sendmessage (2, con->buf, stop) == -1)
    return;
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      if (con->readmessage (stop) == -1)
	break;
      if (EIBTYPE (con->buf) == EIB_RESET_CONNECTION)
	break;
      if (con->size >= 2)
	{
	  if (EIBTYPE (con->buf) != EIB_APDU_PACKET)
	    break;
	  con->t->TracePacket (7, this, "Send", con->size - 2, con->buf + 2);
	  c->Send (CArray (con->buf + 2, con->size - 2));
	}
    }
}

void
A_Connection::Do (pth_event_t stop)
{
  if (!c)
    {
      con->sendreject (stop, EIB_PROCESSING_ERROR);
      return;
    }
  if (con->sendmessage (2, con->buf, stop) == -1)
    return;
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      if (con->readmessage (stop) == -1)
	break;
      if (EIBTYPE (con->buf) == EIB_RESET_CONNECTION)
	break;
      if (con->size >= 2)
	{
	  if (EIBTYPE (con->buf) != EIB_APDU_PACKET)
	    break;
	  con->t->TracePacket (7, this, "Send", con->size - 2, con->buf + 2);
	  c->Send (CArray (con->buf + 2, con->size - 2));
	}
    }
}

void
A_GroupSocket::Do (pth_event_t stop)
{
  if (!c)
    {
      con->sendreject (stop, EIB_PROCESSING_ERROR);
      return;
    }
  if (con->sendmessage (2, con->buf, stop) == -1)
    return;
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      if (con->readmessage (stop) == -1)
	break;
      if (EIBTYPE (con->buf) == EIB_RESET_CONNECTION)
	break;
      if (con->size >= 4)
	{
	  if (EIBTYPE (con->buf) != EIB_GROUP_PACKET)
	    break;
	  con->t->TracePacket (7, this, "Send", con->size - 4, con->buf + 4);
	  GroupAPDU p;
	  p.data = CArray (con->buf + 4, con->size - 4);
	  p.dst = (con->buf[2] << 8) | (con->buf[3]);
	  c->Send (p);
	}
    }
}

void
A_Broadcast::Run (pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      BroadcastComm *e = c->Get (stop);
      if (e)
	{
	  CArray res;
	  res.resize (4 + e->data ());
	  EIBSETTYPE (res, EIB_APDU_PACKET);
	  res[2] = (e->src >> 8) & 0xff;
	  res[3] = (e->src) & 0xff;
	  res.setpart (e->data.array (), 4, e->data ());
	  con->t->TracePacket (7, this, "Recv", e->data);
	  con->sendmessage (res (), res.array (), stop);
	  delete e;
	}
    }
  pth_event_free (stop, PTH_FREE_THIS);
}

void
A_Group::Run (pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      GroupComm *e = c->Get (stop);
      if (e)
	{
	  CArray res;
	  res.resize (4 + e->data ());
	  EIBSETTYPE (res, EIB_APDU_PACKET);
	  res[2] = (e->src >> 8) & 0xff;
	  res[3] = (e->src) & 0xff;
	  res.setpart (e->data.array (), 4, e->data ());
	  con->t->TracePacket (7, this, "Recv", e->data);
	  con->sendmessage (res (), res.array (), stop);
	  delete e;
	}
    }
  pth_event_free (stop, PTH_FREE_THIS);
}

void
A_TPDU::Run (pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      TpduComm *e = c->Get (stop);
      if (e)
	{
	  CArray res;
	  res.resize (4 + e->data ());
	  EIBSETTYPE (res, EIB_APDU_PACKET);
	  res[2] = (e->addr >> 8) & 0xff;
	  res[3] = (e->addr) & 0xff;
	  res.setpart (e->data.array (), 4, e->data ());
	  con->t->TracePacket (7, this, "Recv", e->data);
	  con->sendmessage (res (), res.array (), stop);
	  delete e;
	}
    }
  pth_event_free (stop, PTH_FREE_THIS);
}

void
A_Individual::Run (pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      CArray *e = c->Get (stop);
      if (e)
	{
	  CArray res;
	  res.resize (2 + e->len ());
	  EIBSETTYPE (res, EIB_APDU_PACKET);
	  res.setpart (e->array (), 2, e->len ());
	  con->t->TracePacket (7, this, "Recv", *e);
	  con->sendmessage (res (), res.array (), stop);
	  delete e;
	}
    }
  pth_event_free (stop, PTH_FREE_THIS);
}

void
A_Connection::Run (pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      CArray *e = c->Get (stop);
      if (e)
	{
	  CArray res;
	  res.resize (2 + e->len ());
	  EIBSETTYPE (res, EIB_APDU_PACKET);
	  res.setpart (e->array (), 2, e->len ());
	  con->t->TracePacket (7, this, "Recv", *e);
	  con->sendmessage (res (), res.array (), stop);
	  delete e;
	}
    }
  pth_event_free (stop, PTH_FREE_THIS);
}

void
A_GroupSocket::Run (pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      GroupAPDU *e = c->Get (stop);
      if (e)
	{
	  CArray res;
	  res.resize (6 + e->data ());
	  EIBSETTYPE (res, EIB_GROUP_PACKET);
	  res[2] = (e->src >> 8) & 0xff;
	  res[3] = (e->src) & 0xff;
	  res[4] = (e->dst >> 8) & 0xff;
	  res[5] = (e->dst) & 0xff;
	  res.setpart (e->data.array (), 6, e->data ());
	  con->t->TracePacket (7, this, "Recv", e->data);
	  con->sendmessage (res (), res.array (), stop);
	  delete e;
	}
    }
  pth_event_free (stop, PTH_FREE_THIS);
}
