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

#include "busmonitor.h"
#include "server.h"

A_Busmonitor::~A_Busmonitor ()
{
  TRACEPRINTF (t, 7, "Close A_Busmonitor");
  stop();
}

void
A_Busmonitor::start()
{
  if (running)
    return;
  Router& r = con->router;
  if (v)
    r.registerVBusmonitor (this);
  else
    r.registerBusmonitor (this);
  running = true;
}

void
A_Busmonitor::stop()
{
  if (!running)
    return;
  Router& r = con->router;
  if (v)
    r.deregisterVBusmonitor (this);
  else
    r.deregisterBusmonitor (this);
  running = false;
}

A_Busmonitor::A_Busmonitor (ClientConnPtr c, bool virt, bool TS)
  : L_Busmonitor_CallBack(c->t->name),A__Base(c),router(static_cast<Router&>(c->server->router))
{
  t = TracePtr(new Trace(*c->t));
  t->setAuxName("BusMon");
  TRACEPRINTF (t, 7, "Open A_Busmonitor");
  con = c;
  v = virt;
  ts = TS;
}

bool
A_Busmonitor::setup (uint8_t *buf, size_t len)
{
  CArray resp;

  if (len != 2)
    {
      con->sendreject ();
      return false;
    }

  resp.setpart (buf, 0, 2);
  if (ts)
    {
      resp.resize (6);
      uint32_t tt;
      struct timeval tv;
      gettimeofday(&tv,NULL);
      tt = tv.tv_sec*65536 + tv.tv_usec/(1000000/65536+1);
      resp[2] = tt>>24;
      resp[3] = tt>>16;
      resp[4] = tt>>8;
      resp[5] = tt;
    }

  con->sendmessage (resp.size(), resp.data());
  return true;
}

void
A_Busmonitor::send_L_Busmonitor (LBusmonPtr p)
{
  CArray buf;
  if (ts)
    {
      buf.resize (7);
      EIBSETTYPE (buf, EIB_BUSMONITOR_PACKET_TS);
      buf[2] = p->status;
      buf[3] = (p->timestamp >> 24) & 0xff;
      buf[4] = (p->timestamp >> 16) & 0xff;
      buf[5] = (p->timestamp >> 8) & 0xff;
      buf[6] = (p->timestamp) & 0xff;
    }
  else
    {
      buf.resize (2);
      EIBSETTYPE (buf, EIB_BUSMONITOR_PACKET);
    }
  buf += p->pdu;

  con->sendmessage (buf.size(), buf.data());
}

void
A_Text_Busmonitor::send_L_Busmonitor (LBusmonPtr p)
{
  CArray buf;
  String s = p->Decode (t);
  buf.resize (2 + s.length() + 1);
  EIBSETTYPE (buf, EIB_BUSMONITOR_PACKET);
  buf.setpart ((uint8_t *)s.c_str(), 2, s.length()+1);

  con->sendmessage (buf.size(), buf.data());
}

