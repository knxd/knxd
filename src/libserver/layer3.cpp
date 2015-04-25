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

Layer2Runner::Layer2Runner()
{
}

Layer2Runner::~Layer2Runner()
{
}

void Layer2Runner::Run(pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  unsigned i;

  TRACEPRINTF (l2->t, 3, this, "L2r running: %08X", l2);
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      LPDU *l = l2->Get_L_Data (stop);
      if (!l)
	continue;
      l3->recv_L_Data(l);
    }
  TRACEPRINTF (l2->t, 3, this, "L2r stopped: %08X", l2);
}

Layer3::Layer3 (eibaddr_t addr, Trace * tr)
{
  t = tr;
  defaultAddr = addr;
  TRACEPRINTF (t, 3, this, "Open");
  pth_sem_init (&bufsem);
  mode = 0;
  running = false;
  Start ();
}

Layer3::~Layer3 ()
{
  TRACEPRINTF (t, 3, this, "Close");
  Stop ();
  for (int i = 0; i < layer2 (); i++)
    {
      layer2[i].Stop ();
      if (mode)
        layer2[i].l2->leaveBusmonitor ();
      else
        layer2[i].l2->Close ();
    }
  while (vbusmonitor ())
    deregisterVBusmonitor (vbusmonitor[0].cb);
  while (group ())
    deregisterGroupCallBack (group[0].cb, group[0].dest);
  while (individual ())
    deregisterIndividualCallBack (individual[0].cb, individual[0].src,
				  individual[0].dest);
}

void
Layer3::send_L_Data (L_Data_PDU * l)
{
  TRACEPRINTF (t, 3, this, "Send %s", l->Decode ()());
  if (l->source == 0)
    l->source = defaultAddr;
  for (int i = 0; i < layer2 (); i++)
    if (l->l2 != layer2[i].l2)
      layer2[i].l2->Send_L_Data (l);
}

void
Layer3::recv_L_Data (LPDU * l)
{
  TRACEPRINTF (t, 3, this, "Recv %s", l->Decode ()());
  buf.put (l);
  pth_sem_inc (&bufsem, 0);
}

bool
Layer3::deregisterBusmonitor (L_Busmonitor_CallBack * c)
{
  unsigned i;
  for (i = 0; i < busmonitor (); i++)
    if (busmonitor[i].cb == c)
      {
	busmonitor[i] = busmonitor[busmonitor () - 1];
	busmonitor.resize (busmonitor () - 1);
	if (busmonitor () == 0)
	  {
	    mode = 0;
            for (int i = 0; i < layer2 (); i++)
              {
	        layer2[i].l2->leaveBusmonitor ();
	        layer2[i].l2->Open ();
              }
	  }
	TRACEPRINTF (t, 3, this, "deregisterBusmonitor %08X = 1", c);
	return 1;
      }
  TRACEPRINTF (t, 3, this, "deregisterBusmonitor %08X = 0", c);
  return 0;
}

bool
Layer3::deregisterVBusmonitor (L_Busmonitor_CallBack * c)
{
  unsigned i;
  for (i = 0; i < vbusmonitor (); i++)
    if (vbusmonitor[i].cb == c)
      {
	vbusmonitor[i] = vbusmonitor[vbusmonitor () - 1];
	vbusmonitor.resize (vbusmonitor () - 1);
	if (vbusmonitor () == 0)
	  {
            for (int i = 0; i < layer2 (); i++)
	      layer2[i].l2->closeVBusmonitor ();
	  }
	TRACEPRINTF (t, 3, this, "deregisterVBusmonitor %08X = 1", c);
	return 1;
      }
  TRACEPRINTF (t, 3, this, "deregisterVBusmonitor %08X = 0", c);
  return 0;
}

bool
Layer3::deregisterBroadcastCallBack (L_Data_CallBack * c)
{
  unsigned i;
  for (i = 0; i < broadcast (); i++)
    if (broadcast[i].cb == c)
      {
	broadcast[i] = broadcast[broadcast () - 1];
	broadcast.resize (broadcast () - 1);
	TRACEPRINTF (t, 3, this, "deregisterBroadcast %08X = 1", c);
	return 1;
      }
  TRACEPRINTF (t, 3, this, "deregisterBroadcast %08X = 0", c);
  return 0;
}

bool
Layer3::deregisterLayer2 (Layer2Interface * l2)
{
  unsigned i;
  for (i = 0; i < layer2 (); i++)
    if (layer2[i].l2 == l2)
      {
        if (running)
	  layer2[i].Stop ();
	layer2[i] = layer2[layer2 () - 1];
	layer2.resize (layer2 () - 1);
        if (mode)
          l2->leaveBusmonitor ();
        else
          l2->Close ();
	TRACEPRINTF (t, 3, this, "deregisterLayer2 %08X = 1", l2);
	return 1;
      }
  TRACEPRINTF (t, 3, this, "deregisterLayer2 %08X = 0", l2);
  return 0;
}

bool
Layer3::deregisterGroupCallBack (L_Data_CallBack * c, eibaddr_t addr)
{
  unsigned i;
  for (i = 0; i < group (); i++)
    if (group[i].cb == c && group[i].dest == addr)
      {
	group[i] = group[group () - 1];
	group.resize (group () - 1);
	TRACEPRINTF (t, 3, this, "deregisterGroupCallBack %08X = 1", c);
	for (i = 0; i < group (); i++)
	  {
	    if (group[i].dest == addr)
	      return 1;
	  }
	if (addr)
          for (i = 0; i < layer2 (); i++)
	    layer2[i].l2->removeGroupAddress (addr);
	return 1;
      }
  TRACEPRINTF (t, 3, this, "deregisterGroupCallBack %08X = 0", c);
  return 0;

}

bool
  Layer3::deregisterIndividualCallBack (L_Data_CallBack * c, eibaddr_t src,
					eibaddr_t dest)
{
  unsigned i;
  for (i = 0; i < individual (); i++)
    if (individual[i].cb == c && individual[i].src == src
	&& individual[i].dest == dest)
      {
	individual[i] = individual[individual () - 1];
	individual.resize (individual () - 1);
	TRACEPRINTF (t, 3, this, "deregisterIndividual %08X = 1", c);
	for (i = 0; i < individual (); i++)
	  {
	    if (individual[i].dest == dest)
	      return 1;
	  }
	if (dest)
          for (i = 0; i < layer2 (); i++)
	    layer2[i].l2->removeAddress (dest);
	return 1;
      }
  TRACEPRINTF (t, 3, this, "deregisterIndividual %08X = 0", c);
  return 0;
}

bool
Layer3::registerBusmonitor (L_Busmonitor_CallBack * c)
{
  TRACEPRINTF (t, 3, this, "registerBusmontior %08X", c);
  if (individual ())
    return 0;
  if (group ())
    return 0;
  if (broadcast ())
    return 0;
  if (mode == 0)
    for (int i = 0; i < layer2 (); i++)
      {
        layer2[i].l2->Close ();
        if (!layer2[i].l2->enterBusmonitor ())
	  {
            while(i--)
              {
	        layer2[i].l2->leaveBusmonitor ();
	        layer2[i].l2->Open ();
              }
	    return 0;
	  }
    }
  mode = 1;
  busmonitor.resize (busmonitor () + 1);
  busmonitor[busmonitor () - 1].cb = c;
  TRACEPRINTF (t, 3, this, "registerBusmontior %08X = 1", c);
  return 1;
}

bool
Layer3::registerVBusmonitor (L_Busmonitor_CallBack * c)
{
  TRACEPRINTF (t, 3, this, "registerVBusmonitor %08X", c);
  if (!vbusmonitor ())
    for (int i = 0; i < layer2 (); i++)
      {
        if (!layer2[i].l2->openVBusmonitor ())
          {
            while (i--)
              layer2[i].l2->closeVBusmonitor ();
            TRACEPRINTF (t, 3, this, "registerVBusmontior %08X = 1", c);
            return 0;
          }
      }

  vbusmonitor.resize (vbusmonitor () + 1);
  vbusmonitor[vbusmonitor () - 1].cb = c;
  TRACEPRINTF (t, 3, this, "registerVBusmontior %08X = 1", c);
  return 1;
}

bool
Layer3::registerBroadcastCallBack (L_Data_CallBack * c)
{
  TRACEPRINTF (t, 3, this, "registerBroadcast %08X", c);
  if (mode == 1)
    return 0;
  broadcast.resize (broadcast () + 1);
  broadcast[broadcast () - 1].cb = c;
  TRACEPRINTF (t, 3, this, "registerBroadcast %08X = 1", c);
  return 1;
}

bool
Layer3::registerLayer2 (Layer2Interface * l2)
{
  TRACEPRINTF (t, 3, this, "registerLayer2 %08X", l2);
  if (!(mode ? l2->enterBusmonitor () : l2->Open ()))
    {
      TRACEPRINTF (t, 3, this, "registerLayer2 %08X = 0", l2);
      return 0;
    }
  layer2.resize (layer2() + 1);
  layer2[layer2 () - 1].l2 = l2;
  layer2[layer2 () - 1].l3 = this;
  if (running)
    layer2[layer2 () - 1].Start ();
  TRACEPRINTF (t, 3, this, "registerLayer2 %08X = 1", l2);
  return 1;
}

bool
Layer3::registerGroupCallBack (L_Data_CallBack * c, eibaddr_t addr)
{
  unsigned i;
  TRACEPRINTF (t, 3, this, "registerGroup %08X", c);
  if (mode == 1)
    return 0;
  for (i = 0; i < group (); i++)
    {
      if (group[i].dest == addr)
	break;
    }
  if (i == group ())
    if (addr)
      for (int i = 0; i < layer2 (); i++)
        layer2[i].l2->addGroupAddress (addr);
  group.resize (group () + 1);
  group[group () - 1].cb = c;
  group[group () - 1].dest = addr;
  TRACEPRINTF (t, 3, this, "registerGroup %08X = 1", c);
  return 1;
}

bool
  Layer3::registerIndividualCallBack (L_Data_CallBack * c,
				      Individual_Lock lock, eibaddr_t src,
				      eibaddr_t dest)
{
  unsigned i;
  TRACEPRINTF (t, 3, this, "registerIndividual %08X %d from %s to %s", c, lock, FormatEIBAddr(src).c_str(), FormatEIBAddr(dest).c_str());
  if (mode == 1)
    return 0;
  for (i = 0; i < individual (); i++)
    if (lock == Individual_Lock_Connection &&
	individual[i].src == src &&
	individual[i].lock == Individual_Lock_Connection)
      {
	TRACEPRINTF (t, 3, this, "registerIndividual locked %04X %04X",
		     individual[i].src, individual[i].dest);
	return 0;
      }

  for (i = 0; i < individual (); i++)
    {
      if (individual[i].dest == dest)
	break;
    }
  if (i == individual () && dest)
    for (int i = 0; i < layer2 (); i++)
      layer2[i].l2->addAddress (dest);
  individual.resize (individual () + 1);
  individual[individual () - 1].cb = c;
  individual[individual () - 1].dest = dest;
  individual[individual () - 1].src = src;
  individual[individual () - 1].lock = lock;
  TRACEPRINTF (t, 3, this, "registerIndividual %08X = 1", c);
  return 1;
}

void
Layer3::Run (pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  unsigned i;

  running = true;
  for (i = 0; i < layer2 (); i++)
    layer2[i].Start ();

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
      if (l->getType () == L_Busmonitor)
	{
	  L_Busmonitor_PDU *l1, *l2;
	  l1 = (L_Busmonitor_PDU *) l;

	  TRACEPRINTF (t, 3, this, "Recv %s", l1->Decode ()());
	  for (i = 0; i < busmonitor (); i++)
	    {
	      l2 = new L_Busmonitor_PDU (*l1);
	      busmonitor[i].cb->Send_L_Busmonitor (l2);
	    }
	  for (i = 0; i < vbusmonitor (); i++)
	    {
	      l2 = new L_Busmonitor_PDU (*l1);
	      vbusmonitor[i].cb->Send_L_Busmonitor (l2);
	    }
	}
      if (l->getType () == L_Data)
	{
	  L_Data_PDU *l1;
	  l1 = (L_Data_PDU *) l;
	  if (l1->repeated)
	    {
	      CArray d1 = l1->ToPacket ();
	      for (i = 0; i < ignore (); i++)
		if (d1 == ignore[i].data)
		  {
		    TRACEPRINTF (t, 3, this, "Repeated discareded");
		    goto wt;
		  }
	    }
	  l1->repeated = 1;
	  ignore.resize (ignore () + 1);
	  ignore[ignore () - 1].data = l1->ToPacket ();
	  ignore[ignore () - 1].end = getTime () + 1000000;
	  l1->repeated = 0;

	  if (l1->AddrType == IndividualAddress
	      && l1->dest == defaultAddr)
	    l1->dest = 0;
	  TRACEPRINTF (t, 3, this, "Recv %s", l1->Decode ()());

	  if (l1->AddrType == GroupAddress && l1->dest == 0)
	    {
	      for (i = 0; i < broadcast (); i++)
		broadcast[i].cb->Send_L_Data (new L_Data_PDU (*l1));
	    }
	  if (l1->AddrType == GroupAddress && l1->dest != 0)
	    {
	      for (i = 0; i < group (); i++)
                {
                  Group_Info &grp = group[i];
		  if (grp.dest == l1->dest || grp.dest == 0)
		    grp.cb->Send_L_Data (new L_Data_PDU (*l1));
                }
	    }
	  if (l1->AddrType == IndividualAddress)
	    {
	      for (i = 0; i < individual (); i++)
                {
                  Individual_Info &indiv = individual[i];
		  if (indiv.dest == l1->dest || indiv.dest == 0)
		    if (indiv.src == l1->source || indiv.src == 0)
		      indiv.cb->Send_L_Data (new L_Data_PDU (*l1));
	        }
	    }

          // finally, send to all (other(?)) L2 interfaces
          // TODO: filter by addresses
          send_L_Data(l1);
	}
      // ignore[] is ordered, any timed-out items are at the front
      for (i = 0; i < ignore (); i++)
	if (ignore[i].end >= getTime ())
          break;
      if (i)
        ignore.deletepart (0, i);
    wt:
      delete l;

    }
  TRACEPRINTF (t, 3, this, "L3 stopping");

  running = false;
  for (i = 0; i < layer2 (); i++)
    layer2[i].Stop ();

  pth_event_free (stop, PTH_FREE_THIS);
}
