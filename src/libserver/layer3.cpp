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

Layer3::Layer3 (eibaddr_t addr, Trace * tr, bool force_broadcast)
{
  t = tr;
  defaultAddr = addr;
  this->force_broadcast = force_broadcast;
  TRACEPRINTF (t, 3, this, "Open");
  pth_sem_init (&bufsem);
  running = false;
  Start ();
}

Layer3::~Layer3 ()
{
  TRACEPRINTF (t, 3, this, "Close");
  Stop ();
  servers.resize(0);

  // the next loops should do exactly nothing
  while (vbusmonitor.size())
    deregisterVBusmonitor (vbusmonitor[0].cb);

  tracers.resize(0);
}

void
Layer3::recv_L_Data (LPDU * l)
{
  if (running)
    {
      TRACEPRINTF (t, 3, this, "Enqueue %s", l->Decode ()());
      buf.put (l);
      pth_sem_inc (&bufsem, 0);
    }
  else
    {
      TRACEPRINTF (t, 3, this, "Discard(not running) %s", l->Decode ()());
      delete l;
    }
}

bool
Layer3::deregisterBusmonitor (L_Busmonitor_CallBack * c)
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
	TRACEPRINTF (t, 3, this, "deregisterBusmonitor %08X = 1", c);
	return true;
      }
  TRACEPRINTF (t, 3, this, "deregisterBusmonitor %08X = 0", c);
  return false;
}

void
Layer3::deregisterServer (BaseServer * s)
{
  ITER(i,servers)
    if (*i == s)
      {
	TRACEPRINTF (t, 3, this, "deregisterServer %d:%s = 1", s->t->seq, s->t->name.c_str());
	servers.erase(i);
	return;
      }
  TRACEPRINTF (t, 3, this, "deregisterServer %d:%s = 0", s->t->seq, s->t->name.c_str());
}

bool
Layer3::deregisterVBusmonitor (L_Busmonitor_CallBack * c)
{
  ITER(i,vbusmonitor)
    if (i->cb == c)
      {
	TRACEPRINTF (t, 3, this, "deregisterVBusmonitor %08X = 1", c);
	vbusmonitor.erase(i);
	return true;
      }
  TRACEPRINTF (t, 3, this, "deregisterVBusmonitor %08X = 0", c);
  return false;
}

bool
Layer3::deregisterLayer2 (Layer2Ptr l2)
{
  ITER(i,layer2)
    if (*i == l2)
      {
	TRACEPRINTF (t, 3, this, "deregisterLayer2 %d:%s = 1", l2->t->seq, l2->t->name.c_str());
	layer2.erase(i);
	return true;
      }
  TRACEPRINTF (t, 3, this, "deregisterLayer2 %d:%s = 0", l2->t->seq, l2->t->name.c_str());
  return false;
}

bool
Layer3::registerBusmonitor (L_Busmonitor_CallBack * c)
{
  TRACEPRINTF (t, 3, this, "registerBusmonitor %08X", c);
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
  TRACEPRINTF (t, 3, this, "registerBusmontitr %08X = 1", c);
  return true;
}

bool
Layer3::registerVBusmonitor (L_Busmonitor_CallBack * c)
{
  vbusmonitor.push_back((Busmonitor_Info){.cb=c});
  TRACEPRINTF (t, 3, this, "registerVBusmonitor %08X", c);
  return true;
}

bool
Layer3::registerLayer2 (Layer2Ptr l2)
{
  TRACEPRINTF (t, 3, this, "registerLayer2 %d:%s", l2->t->seq, l2->t->name.c_str());
  if (! busmonitor.size() || ! l2->enterBusmonitor ())
    if (! l2->Open ())
      {
        TRACEPRINTF (t, 3, this, "registerLayer2 %d:%s = 0", l2->t->seq, l2->t->name.c_str());
        return false;
      }
  layer2.push_back(l2);
  TRACEPRINTF (t, 3, this, "registerLayer2 %d:%s = 1", l2->t->seq, l2->t->name.c_str());
  return true;
}

bool
Layer3::hasAddress (eibaddr_t addr, Layer2Ptr l2)
{
  if (addr == defaultAddr)
    return true;

  ITER(i,layer2)
    if (*i != l2 && (*i)->hasAddress (addr))
      return true;

  return false;
}

bool
Layer3::hasGroupAddress (eibaddr_t addr, Layer2Ptr l2 UNUSED)
{
  if (addr == 0) // always accept broadcast
    return true;

  ITER(i, layer2)
    if ((*i)->hasGroupAddress (addr))
      return true;

  return false;
}

void
Layer3::set_client_block (eibaddr_t r_start, int r_len)
{
  client_addrs_start = r_start;
  client_addrs_len = r_len;
}

eibaddr_t
Layer3::get_client_addr ()
{
  /*
   * Start allocating after the last request.
   * Otherwise we'd need locking to protect concurrent requests
   * This is less bug-prone
   */
  if (client_addrs_len)
    for (int i = 1; i <= client_addrs_len; i++)
      {
        eibaddr_t a = client_addrs_start + (client_addrs_pos + i) % client_addrs_len;
        if (! hasAddress (a))
          {
            TRACEPRINTF (t, 3, this, "Allocate %s", FormatEIBAddr (a)());
            /* remember for next pass */
            client_addrs_pos = a - client_addrs_start;
            return a;
          }
      }

  /* Fall back to our own address */
  TRACEPRINTF (t, 3, this, "Allocate: falling back to %s", FormatEIBAddr (defaultAddr)());
  return 0;
}

void
Layer3::Run (pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  unsigned i;

  running = true;

  TRACEPRINTF (t, 3, this, "L3 started");
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      pth_event_t bufev = pth_event (PTH_EVENT_SEM, &bufsem);
      pth_event_concat (bufev, stop, NULL);
      pth_wait (bufev);
      pth_event_isolate (bufev);

      if (pth_event_status (bufev) != PTH_STATUS_OCCURRED)
        {
          pth_event_free (bufev, PTH_FREE_THIS);
          continue;
        }
      pth_event_free (bufev, PTH_FREE_THIS);

      pth_sem_dec (&bufsem);
      LPDU *l = buf.get ();

      if (!l)
	continue;

      if (l->getType () == L_Data)
	{
	  L_Data_PDU *l1 = dynamic_cast<L_Data_PDU *>(l);

          if (vbusmonitor.size())
            {
              L_Busmonitor_PDU *l2 = new L_Busmonitor_PDU (l->l2);
              l2->pdu.set (l->ToPacket ());

              ITER(i,vbusmonitor)
                {
                  L_Busmonitor_PDU *l2x = new L_Busmonitor_PDU (*l2);
                  i->cb->Send_L_Busmonitor (l2x);
                }
              delete l2;
            }
          if (!l1->hopcount)
            {
              TRACEPRINTF (t, 3, this, "Hopcount zero: %s", l1->Decode ()());
              delete l;
              continue;
            }
          if (l1->hopcount < 7 || !force_broadcast)
            l1->hopcount--;

	  if (l1->repeated)
	    {
	      CArray d1 = l1->ToPacket ();
	      ITER (i,ignore)
		if (d1 == i->data)
		  {
		    TRACEPRINTF (t, 3, this, "Repeated discareded");
		    goto wt;
		  }
	    }
	  l1->repeated = 1;
	  ignore.push_back((IgnoreInfo){.data = l1->ToPacket (), .end = getTime () + 1000000});
	  l1->repeated = 0;

          {
            Layer2Ptr l2 = l1->l2;

            if (l1->source == 0)
              l1->source = l2->remoteAddr ? l2->remoteAddr : defaultAddr;
            if (l1->source != defaultAddr)
              l2->addAddress (l1->source);
            else if (l1->AddrType == IndividualAddress && l1->dest != defaultAddr)
              l2->addReverseAddress (l1->dest);
          }

	  if (l1->AddrType == IndividualAddress
	      && l1->dest == defaultAddr)
	    l1->dest = 0;
	  TRACEPRINTF (t, 3, this, "RecvData %s", l1->Decode ()());

	  if (l1->source == 0)
	    l1->source = defaultAddr;

	  if (l1->AddrType == GroupAddress)
	    {
	      // This is easy: send to all other L2 which subscribe to the
	      // group.
	      ITER(i, layer2)
                {
		  if ((l1->hopcount == 7)
		      || ((*i != l1->l2) && (*i)->hasGroupAddress(l1->dest)))
		    (*i)->Send_L_Data (new L_Data_PDU (*l1));
                }
	    }
	  if (l1->AddrType == IndividualAddress)
	    {
	      // This is not so easy: we want to send to whichever
	      // interface on which the address has appeared. If it hasn't
	      // been seen yet, we send to all interfaces which are buses.
	      // which get marked by accepting the otherwise-illegal physical
	      // address 0.
	      bool found = false;
	      ITER(i, layer2)
                {
                  if (*i == l1->l2)
		    continue;
                  if (l1->dest ? (*i)->hasAddress (l1->dest)
		               : (*i)->hasReverseAddress (l1->source))
		    {
		      found = true;
		      break;
		    }
		}
	      ITER (i, layer2)
		if ((l1->hopcount == 7)
                    || (*i != l1->l2
		     && l1->dest ? (*i)->hasAddress (found ? l1->dest : 0)
		                 : (*i)->hasReverseAddress (l1->source)))
		  (*i)->Send_L_Data (new L_Data_PDU (*l1));
	    }
	}
      else if (l->getType () == L_Busmonitor)
	{
	  L_Busmonitor_PDU *l1, *l2;
	  l1 = dynamic_cast<L_Busmonitor_PDU *>(l);

	  TRACEPRINTF (t, 3, this, "RecvMon %s", l1->Decode ()());
	  ITER (i, busmonitor)
	    {
	      l2 = new L_Busmonitor_PDU (*l1);
	      i->cb->Send_L_Busmonitor (l2);
	    }
	}
      // ignore[] is ordered, any timed-out items are at the front
      ITER (i, ignore)
	if (i->end >= getTime ())
          {
            ignore.erase (ignore.begin(), i);
            break;
          }
    wt:
      delete l;

    }
  TRACEPRINTF (t, 3, this, "L3 stopping");

  running = false;
  ITER(i,layer2)
    {
      Layer2Ptr l2 = *i;

      // TODO: This depends on runtime typing. Ugh. Fix me.
      std::shared_ptr<Thread> l2t = std::dynamic_pointer_cast<Thread>(l2);
      if (l2t)
        l2t->Stop();
    }
  layer2.resize (0);

  pth_event_free (stop, PTH_FREE_THIS);
}
