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

A_Broadcast::A_Broadcast (Layer3 * l3, Trace * tr, ClientConnection * cc)
{
  t = tr;
  TRACEPRINTF (t, 7, this, "OpenBroadcast");
  layer3 = l3;
  con = cc;
  c = 0;
  if (con->size != 5)
    return;
  c = new T_Broadcast (layer3, t, con->buf[4] != 0 ? 1 : 0);
  if (!c->init ())
    {
      delete c;
      c = 0;
      return;
    }
  Start ();
}

A_Group::A_Group (Layer3 * l3, Trace * tr, ClientConnection * cc)
{
  t = tr;
  TRACEPRINTF (t, 7, this, "OpenGroup");
  layer3 = l3;
  con = cc;
  c = 0;
  if (con->size != 5)
    return;
  c =
    new T_Group (layer3, t, (con->buf[2] << 8) | (con->buf[3]),
		 con->buf[4] != 0 ? 1 : 0);
  if (!c->init ())
    {
      delete c;
      c = 0;
      return;
    }
  Start ();
}

A_TPDU::A_TPDU (Layer3 * l3, Trace * tr, ClientConnection * cc)
{
  t = tr;
  TRACEPRINTF (t, 7, this, "OpenTPDU");
  layer3 = l3;
  con = cc;
  c = 0;
  if (con->size != 5)
    return;
  c = new T_TPDU (layer3, t, (con->buf[2] << 8) | (con->buf[3]));
  if (!c->init ())
    {
      delete c;
      c = 0;
      return;
    }
  Start ();
}

A_Individual::A_Individual (Layer3 * l3, Trace * tr, ClientConnection * cc)
{
  t = tr;
  TRACEPRINTF (t, 7, this, "OpenIndividual");
  layer3 = l3;
  con = cc;
  c = 0;
  if (con->size != 5)
    return;
  c =
    new T_Individual (layer3, t, (con->buf[2] << 8) | (con->buf[3]),
		      con->buf[4] != 0 ? 1 : 0);
  if (!c->init ())
    {
      delete c;
      c = 0;
      return;
    }
  Start ();
}

A_Connection::A_Connection (Layer3 * l3, Trace * tr, ClientConnection * cc)
{
  t = tr;
  TRACEPRINTF (t, 7, this, "OpenGroup");
  layer3 = l3;
  con = cc;
  c = 0;
  if (con->size != 5)
    return;
  c = new T_Connection (layer3, t, (con->buf[2] << 8) | (con->buf[3]));
  if (!c->init ())
    {
      delete c;
      c = 0;
      return;
    }
  Start ();
}

A_GroupSocket::A_GroupSocket (Layer3 * l3, Trace * tr, ClientConnection * cc)
{
  t = tr;
  TRACEPRINTF (t, 7, this, "OpenGroupSocket");
  layer3 = l3;
  con = cc;
  c = 0;
  if (con->size != 5)
    return;
  c = new GroupSocket (layer3, t, con->buf[4] != 0 ? 1 : 0);
  if (!c->init ())
    {
      delete c;
      c = 0;
      return;
    }
  Start ();
}

A_Broadcast::~A_Broadcast ()
{
  TRACEPRINTF (t, 7, this, "CloseBroadcast");
  Stop ();
  if (c)
    delete c;
}

A_Group::~A_Group ()
{
  TRACEPRINTF (t, 7, this, "CloseGroup");
  Stop ();
  if (c)
    delete c;
}

A_TPDU::~A_TPDU ()
{
  TRACEPRINTF (t, 7, this, "CloseTPDU");
  Stop ();
  if (c)
    delete c;
}

A_Individual::~A_Individual ()
{
  TRACEPRINTF (t, 7, this, "CloseIndividual");
  Stop ();
  if (c)
    delete c;
}

A_Connection::~A_Connection ()
{
  TRACEPRINTF (t, 7, this, "CloseConnection");
  Stop ();
  if (c)
    delete c;
}

A_GroupSocket::~A_GroupSocket ()
{
  TRACEPRINTF (t, 7, this, "CloseGroupSocket");
  Stop ();
  if (c)
    delete c;
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
	  t->TracePacket (7, this, "Send", con->size - 2, con->buf + 2);
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
	  t->TracePacket (7, this, "Send", con->size - 2, con->buf + 2);
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
	  t->TracePacket (7, this, "Send", con->size - 4, con->buf + 4);
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
	  t->TracePacket (7, this, "Send", con->size - 2, con->buf + 2);
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
	  t->TracePacket (7, this, "Send", con->size - 2, con->buf + 2);
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
	  t->TracePacket (7, this, "Send", con->size - 4, con->buf + 4);
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
	  t->TracePacket (7, this, "Recv", e->data);
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
	  t->TracePacket (7, this, "Recv", e->data);
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
	  t->TracePacket (7, this, "Recv", e->data);
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
	  t->TracePacket (7, this, "Recv", *e);
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
	  t->TracePacket (7, this, "Recv", *e);
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
	  t->TracePacket (7, this, "Recv", e->data);
	  con->sendmessage (res (), res.array (), stop);
	  delete e;
	}
    }
  pth_event_free (stop, PTH_FREE_THIS);
}
