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

#include "eibnetrouter.h"
#include "emi.h"
#include "config.h"

EIBNetIPRouter::EIBNetIPRouter (const LinkConnectPtr_& c, IniSectionPtr& s)
  : BusDriver(c,s)
{
  t->setAuxName("ip");
}

void
EIBNetIPRouter::start()
{
  struct sockaddr_in baddr;
  struct ip_mreq mcfg;
  TRACEPRINTF (t, 2, "Open");
  memset (&baddr, 0, sizeof (baddr));
#ifdef HAVE_SOCKADDR_IN_LEN
  baddr.sin_len = sizeof (baddr);
#endif
  baddr.sin_family = AF_INET;
  baddr.sin_port = htons (port);
  baddr.sin_addr.s_addr = htonl (INADDR_ANY);
  sock = new EIBNetIPSocket (baddr, 1, t);
  if (!sock->init ())
    goto err_out;
  sock->on_recv.set<EIBNetIPRouter,&EIBNetIPRouter::read_cb>(this);

  if (! sock->SetInterface(interface))
    {
      ERRORPRINTF (t, E_ERROR | 58, "interface %s not recognized", interface);
      goto err_out;
    }

  sock->recvall = 2;
  if (GetHostIP (t, &sock->sendaddr, multicastaddr) == 0)
    goto err_out;
  sock->sendaddr.sin_port = htons (port);
  if (!GetSourceAddress (t, &sock->sendaddr, &sock->localaddr))
    goto err_out;
  sock->localaddr.sin_port = sock->sendaddr.sin_port;

  mcfg.imr_multiaddr = sock->sendaddr.sin_addr;
  mcfg.imr_interface.s_addr = htonl (INADDR_ANY);
  if (!sock->SetMulticast (mcfg))
    goto err_out;
  TRACEPRINTF (t, 2, "Opened");
  BusDriver::start();
  return;

err_out:
  delete sock;
  sock = 0;
  stopped();
}

void
EIBNetIPRouter::stop()
{
  stop_();
  BusDriver::stop();
}

void
EIBNetIPRouter::stop_()
{
  if (sock)
    {
      delete sock;
      sock = nullptr;
    }
}

EIBNetIPRouter::~EIBNetIPRouter ()
{
  stop_();
  TRACEPRINTF (t, 2, "Destroy");
}

bool
EIBNetIPRouter::setup()
{
  if (!assureFilter("pace"))
    return false;
  if(!BusDriver::setup())
    return false;
  multicastaddr = cfg->value("multicast-address","224.0.23.12");
  port = cfg->value("port",3671);
  interface = cfg->value("interface","");
  monitor = cfg->value("monitor",false);
  return true;
}

void
EIBNetIPRouter::send_L_Data (LDataPtr l)
{
  EIBNetIPPacket p;
  p.data = L_Data_ToCEMI (0x29, l);
  p.service = ROUTING_INDICATION;
  sock->Send (p);
  send_Next();
}

void
EIBNetIPRouter::read_cb(EIBNetIPPacket *p)
{
  if (p->service != ROUTING_INDICATION)
    {
      delete p;
      return;
    }
  if (p->data.size() < 2 || p->data[0] != 0x29)
    {
      if (p->data.size() < 2)
        {
          TRACEPRINTF (t, 2, "No payload (%d)", p->data.size());
        }
      else
        {
          TRACEPRINTF (t, 2, "Payload not L_Data.ind (%02x)", p->data[0]);
        }
      delete p;
      return;
    }

  LDataPtr c = CEMI_to_L_Data (p->data, t);
  delete p;
  if (c)
    {
      if (!monitor)
        recv_L_Data (std::move(c));
      else
        {
          LBusmonPtr p1 = LBusmonPtr(new L_Busmonitor_PDU ());
          p1->pdu = c->ToPacket ();
          recv_L_Busmonitor (std::move(p1));
        }
    }
}

