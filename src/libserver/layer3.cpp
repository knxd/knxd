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

#include "layer3.h"
#include "layer2.h"
#include "server.h"
#include <typeinfo>

Layer3::Layer3() { }
Layer3::~Layer3() { }

Layer3real::Layer3real (eibaddr_t addr, TracePtr tr, bool force_broadcast)
    : Layer3()
{
  _tr = tr;
  defaultAddr = addr;
  this->force_broadcast = force_broadcast;
  TRACEPRINTF (_tr, 3, this, "Open");
  running = true;
  trigger.set<Layer3real, &Layer3real::trigger_cb>(this);
  trigger.start();
  mtrigger.set<Layer3real, &Layer3real::mtrigger_cb>(this);
  mtrigger.start();
  cleanup.set<Layer3real, &Layer3real::cleanup_cb>(this);
  cleanup.start();

  TRACEPRINTF (_tr, 3, this, "L3 started");
}

Layer3real::~Layer3real ()
{
  TRACEPRINTF (tr(), 3, this, "L3 stopping");
  running = false;
  cache = nullptr;
  trigger.stop();
  mtrigger.stop();
  while (!buf.isempty())
    delete buf.get();
  while (!mbuf.isempty())
    delete mbuf.get();

  R_ITER(i,layer2)
    (*i)->stop();
  ITER(i,layer2)
    ERRORPRINTF (tr(), E_WARNING | 54, this, "Layer2 '%s' didn't de-register!", (*i)->Name());

  layer2.clear();
  servers.clear();

  R_ITER(i,vbusmonitor)
    ERRORPRINTF (tr(), E_WARNING | 55, this, "VBusmonitor '%s' didn't de-register!", i->cb->Name());
  vbusmonitor.clear();

  R_ITER(i,busmonitor)
    ERRORPRINTF (tr(), E_WARNING | 56, this, "Busmonitor '%s' didn't de-register!", i->cb->Name());
  busmonitor.clear();

  cleanup.stop();
  TRACEPRINTF (tr(), 3, this, "Closed");
}

void
Layer3real::recv_L_Data (L_Data_PDU * l)
{
  if (running)
    {
      TRACEPRINTF (tr(), 9, this, "Enqueue %s", l->Decode ().c_str());
      buf.put (l);
      trigger.send();
    }
  else
    {
      TRACEPRINTF (tr(), 3, this, "Discard(not running) %s", l->Decode ().c_str());
      delete l;
    }
}

void
Layer3real::recv_L_Busmonitor (L_Busmonitor_PDU * l)
{
  if (running)
    {
      TRACEPRINTF (tr(), 3, this, "Enqueue %s", l->Decode ().c_str());
      mbuf.put (l);
      mtrigger.send();
    }
  else
    {
      TRACEPRINTF (tr(), 3, this, "Discard(not running) %s", l->Decode ().c_str());
      delete l;
    }
}

bool
Layer3real::deregisterBusmonitor (L_Busmonitor_CallBack * c)
{
  unsigned i;
  for (i = 0; i < busmonitor.size(); i++)
    if (busmonitor[i].cb == c)
      {
	busmonitor.erase(busmonitor.begin()+i);
	if (busmonitor.size() == 0)
          for (unsigned int i = 0; i < layer2.size(); i++)
            if (layer2[i]->leaveBusmonitor ())
              layer2[i]->Open ();
	TRACEPRINTF (tr(), 3, this, "deregisterBusmonitor %08X = 1", c);
	return true;
      }
  TRACEPRINTF (tr(), 3, this, "deregisterBusmonitor %08X = 0", c);
  return false;
}

void
Layer3real::deregisterServer (BaseServer * s)
{
  ITER(i,servers)
    if (*i == s)
      {
	TRACEPRINTF (tr(), 3, this, "deregisterServer %d:%s = 1", s->t->seq, s->t->name.c_str());
	servers.erase(i);
	return;
      }
  TRACEPRINTF (tr(), 3, this, "deregisterServer %d:%s = 0", s->t->seq, s->t->name.c_str());
}

bool
Layer3real::deregisterVBusmonitor (L_Busmonitor_CallBack * c)
{
  ITER(i,vbusmonitor)
    if (i->cb == c)
      {
	TRACEPRINTF (tr(), 3, this, "deregisterVBusmonitor %08X = 1", c);
	vbusmonitor.erase(i);
	return true;
      }
  TRACEPRINTF (tr(), 3, this, "deregisterVBusmonitor %08X = 0", c);
  return false;
}

bool
Layer3real::deregisterLayer2 (Layer2Ptr l2)
{
  cleanup_q.put(l2);
  cleanup.send();
}

void
Layer3real::cleanup_cb (ev::async &w, int revents)
{
  while (!cleanup_q.isempty())
    {
      Layer2Ptr l2 = cleanup_q.get();

      ITER(i,layer2)
        if (*i == l2)
          {
            TRACEPRINTF (tr(), 3, this, "deregisterLayer2 %d:%s = 1", l2->t->seq, l2->t->name.c_str());
            layer2.erase(i);
            break;
          }
      TRACEPRINTF (tr(), 3, this, "deregisterLayer2 %d:%s = 0", l2->t->seq, l2->t->name.c_str());
    }
}

bool
Layer3real::registerBusmonitor (L_Busmonitor_CallBack * c)
{
  TRACEPRINTF (tr(), 3, this, "registerBusmonitor %08X", c);
  if (!busmonitor.size()) 
    {
      bool have_monitor = false;
      ITER (i, layer2)
        if (i->get()->Close ()) 
          {
            if (i->get()->enterBusmonitor ())
              have_monitor = true;
            else
              i->get()->Open ();
          }
      if (! have_monitor)
        return false;
    }
  busmonitor.push_back((Busmonitor_Info){.cb=c});
  TRACEPRINTF (tr(), 3, this, "registerBusmontitr %08X = 1", c);
  return true;
}

bool
Layer3real::registerVBusmonitor (L_Busmonitor_CallBack * c)
{
  vbusmonitor.push_back((Busmonitor_Info){.cb=c});
  TRACEPRINTF (tr(), 3, this, "registerVBusmonitor %08X", c);
  return true;
}

Layer3 *
Layer3real::registerLayer2 (Layer2Ptr l2)
{
  TRACEPRINTF (tr(), 3, this, "registerLayer2 %d:%s", l2->t->seq, l2->t->name.c_str());
  if (! busmonitor.size() || ! l2->enterBusmonitor ())
    if (! l2->Open ())
      {
        TRACEPRINTF (tr(), 3, this, "registerLayer2 %d:%s = 0", l2->t->seq, l2->t->name.c_str());
        return nullptr;
      }
  layer2.push_back(l2);
  TRACEPRINTF (tr(), 3, this, "registerLayer2 %d:%s = 1", l2->t->seq, l2->t->name.c_str());
  return this;
}

bool
Layer3real::hasAddress (eibaddr_t addr, Layer2Ptr l2)
{
  if (addr == defaultAddr)
    return true;

  ITER(i,layer2)
    if (*i != l2 && (*i)->hasAddress (addr))
      return true;

  return false;
}

bool
Layer3real::hasGroupAddress (eibaddr_t addr, Layer2Ptr l2 UNUSED)
{
  if (addr == 0) // always accept broadcast
    return true;

  ITER(i, layer2)
    if ((*i)->hasGroupAddress (addr))
      return true;

  return false;
}

void
Layer3real::set_client_block (eibaddr_t r_start, int r_len)
{
  client_addrs_start = r_start;
  client_addrs_len = r_len;
  client_addrs_pos = r_len-1;
  client_addrs.resize(r_len);
  ITER(i,client_addrs)
    *i = false;
}

eibaddr_t
Layer3real::get_client_addr ()
{
  /*
   * Start allocating after the last request.
   * Otherwise we'd need locking to protect concurrent requests
   * This is less bug-prone
   */
  for (int i = 1; i <= client_addrs_len; i++)
    {
      unsigned int pos = (client_addrs_pos + i) % client_addrs_len;
      if (client_addrs[pos])
        continue;
      eibaddr_t a = client_addrs_start + pos;
      if (! hasAddress (a))
        {
          TRACEPRINTF (tr(), 3, this, "Allocate %s", FormatEIBAddr (a).c_str());
          /* remember for next pass */
          client_addrs_pos = pos;
          client_addrs[pos] = true;
          return a;
        }
    }

  /* Fall back to our own address */
  TRACEPRINTF (tr(), 3, this, "Allocate: falling back to %s", FormatEIBAddr (defaultAddr).c_str());
  return 0;
}

void
Layer3real::release_client_addr(eibaddr_t addr)
{
  TRACEPRINTF (tr(), 3, this, "Release %s", FormatEIBAddr (addr).c_str());
  if (addr < client_addrs_start)
    return;
  unsigned int pos = addr - client_addrs_start;
  if (pos >= client_addrs_len)
    return;
  client_addrs[pos] = false;
}

void
Layer3real::trigger_cb (ev::async &w, int revents)
{
  unsigned i;

  while (!buf.isempty())
    {
      L_Data_PDU *l1 = buf.get ();

      {
        Layer2Ptr l2 = l1->l2;

        if (l1->source == 0)
          l1->source = l2->getRemoteAddr();
        if (l1->source == 0)
          l1->source = defaultAddr;
        if (l1->source != defaultAddr)
          l2->addAddress (l1->source);
      }

      if (vbusmonitor.size())
        {
          L_Busmonitor_PDU *l2 = new L_Busmonitor_PDU (l1->l2);
          l2->pdu.set (l1->ToPacket ());

          ITER(i,vbusmonitor)
            {
              L_Busmonitor_PDU *l2x = new L_Busmonitor_PDU (*l2);
              i->cb->send_L_Busmonitor (l2x);
            }
          delete l2;
        }
      if (!l1->hopcount)
        {
          TRACEPRINTF (tr(), 3, this, "Hopcount zero: %s", l1->Decode ().c_str());
          goto next;
        }
      if (l1->hopcount < 7 || !force_broadcast)
        l1->hopcount--;

      if (l1->repeated)
        {
          CArray d1 = l1->ToPacket ();
          ITER (i,ignore)
            if (d1 == i->data)
              {
                TRACEPRINTF (tr(), 9, this, "Repeated, discarded");
                goto next;
              }
        }
      l1->repeated = 1;
      ignore.push_back((IgnoreInfo){.data = l1->ToPacket (), .end = getTime () + 1000000});
      l1->repeated = 0;

      if (l1->AddrType == IndividualAddress
          && l1->dest == defaultAddr)
        l1->dest = 0;
      TRACEPRINTF (tr(), 3, this, "RecvData %s", l1->Decode ().c_str());

      if (l1->AddrType == GroupAddress)
        {
          // This is easy: send to all other L2 which subscribe to the
          // group.
          ITER(i, layer2)
            {
              if ((l1->hopcount == 7)
                  || ((*i != l1->l2) && (*i)->hasGroupAddress(l1->dest)))
                (*i)->send_L_Data (new L_Data_PDU (*l1));
            }
        }
      if (l1->AddrType == IndividualAddress)
        {
	  if (!l1->dest)
	    {
	      // Common problem with things that are not true gateways
	      ERRORPRINTF (l1->l2->t, E_WARNING | 57, this, "Message without destination. Use the single-node filter ('-B single')?");
	      goto next;
	    }

          // This is not so easy: we want to send to whichever
          // interface on which the address has appeared. If it hasn't
          // been seen yet, we send to all interfaces.
          // Addresses ~0 is special; it's used for programming
          // so can be on different interfaces. Always broadcast these.
          bool found = false;
          if (l1->dest != 0xFFFF)
            ITER(i, layer2)
              {
                if (*i == l1->l2)
                  continue;
                if ((*i)->hasAddress (l1->dest))
                  {
                    found = true;
                    break;
                  }
              }
          ITER (i, layer2)
            if (l1->hopcount == 7
                || (*i != l1->l2
                  && (!found || (*i)->hasAddress (l1->dest))))
              (*i)->send_L_Data (new L_Data_PDU (*l1));
        }
    next:
      delete l1;
    }

  // Timestamps are ordered, so we scan for the first 
  timestamp_t tm = getTime ();
  ITER (i, ignore)
    if (i->end >= tm)
      {
        ignore.erase (ignore.begin(), i);
        break;
      }
}

void
Layer3real::mtrigger_cb (ev::async &w, int revents)
{
  unsigned i;

  while (!mbuf.isempty())
    {
      L_Busmonitor_PDU *l1 = mbuf.get ();

      TRACEPRINTF (tr(), 3, this, "RecvMon %s", l1->Decode ().c_str());
      ITER (i, busmonitor)
        i->cb->send_L_Busmonitor (new L_Busmonitor_PDU (*l1));
      delete l1;
    }
}
