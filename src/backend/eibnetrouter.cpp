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
#include "layer3.h"

EIBNetIPRouter::EIBNetIPRouter (const char *multicastaddr, int port, L2options *opt) : Layer2 (opt)
{
  struct sockaddr_in baddr;
  struct ip_mreq mcfg;
  TRACEPRINTF (t, 2, this, "Open");
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
  sock->on_recv.set<EIBNetIPRouter,&EIBNetIPRouter::on_recv_cb>(this);

  sock->recvall = 2;
  if (GetHostIP (t, &sock->sendaddr, multicastaddr) == 0)
    goto err_out;
  sock->sendaddr.sin_port = htons (port);
  if (!GetSourceAddress (&sock->sendaddr, &sock->localaddr))
    goto err_out;
  sock->localaddr.sin_port = sock->sendaddr.sin_port;

  mcfg.imr_multiaddr = sock->sendaddr.sin_addr;
  mcfg.imr_interface.s_addr = htonl (INADDR_ANY);
  if (!sock->SetMulticast (mcfg))
    goto err_out;
  TRACEPRINTF (t, 2, this, "Opened");
  return;

err_out:
  delete sock;
  sock = 0;
  return;
}

EIBNetIPRouter::~EIBNetIPRouter ()
{
  TRACEPRINTF (t, 2, this, "Destroy");
  if (sock)
    delete sock;
}

bool
EIBNetIPRouter::init (Layer3 *l3)
{
  if (sock == 0)
    return false;
  if (! addGroupAddress(0))
    return false;
  return Layer2::init (l3);
}

void
EIBNetIPRouter::Send_L_Data (LPDU * l)
{
  TRACEPRINTF (t, 2, this, "Send %s", l->Decode ().c_str());
  if (l->getType () != L_Data)
    {
      delete l;
      return;
    }
  L_Data_PDU *l1 = (L_Data_PDU *) l;
  EIBNetIPPacket p;
  p.data = L_Data_ToCEMI (0x29, *l1);
  p.service = ROUTING_INDICATION;
  sock->Send (p);
  delete l;
}

void
EIBNetIPRouter::on_recv_cb(EIBNetIPPacket *p)
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
          TRACEPRINTF (t, 2, this, "No payload (%d)", p->data.size());
        }
      else
        {
          TRACEPRINTF (t, 2, this, "Payload not L_Data.ind (%02x)", p->data[0]);
        }
      delete p;
      return;
    }

  const CArray data = p->data;
  delete p;
  L_Data_PDU *c = CEMI_to_L_Data (data, shared_from_this());
  if (c)
    {
      TRACEPRINTF (t, 2, this, "Recv %s", c->Decode ().c_str());
      if (mode & BUSMODE_UP)
        {
          l3->recv_L_Data (c);
          return;
        }
      L_Busmonitor_PDU *p1 = new L_Busmonitor_PDU (shared_from_this());
      p1->pdu = c->ToPacket ();
      delete c;
      l3->recv_L_Data (p1);
      return;
    }
}

