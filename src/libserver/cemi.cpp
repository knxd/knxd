/*
    EIBD eib bus access and management daemon
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>
 
    cEMI support for USB
    Copyright (C) 2013 Meik Felser <felser@cs.fau.de>

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

#include "cemi.h"
#include "emi.h"
#include "layer3.h"

CEMILayer2::CEMILayer2 (LowLevelDriver * i, Layer3 * l3,
                        L2options *opt) : Layer2 (l3, opt)
{
  TRACEPRINTF (t, 2, this, "Open");
  iface = i;
  noqueue = opt ? (opt->flags & FLAG_B_EMI_NOQUEUE) : false;
  if (opt->flags)
    opt->flags &=~ FLAG_B_EMI_NOQUEUE;
  pth_sem_init (&in_signal);
  if (!iface->init ())
    {
      delete iface;
      iface = 0;
      return;
    }
  Start ();
  TRACEPRINTF (t, 2, this, "Opened");
}

bool
CEMILayer2::init ()
{
  if (! layer2_is_bus())
    return false;
  return iface != 0;
}

CEMILayer2::~CEMILayer2 ()
{
  TRACEPRINTF (t, 2, this, "Destroy");
  Stop ();
  while (!inqueue.isempty ())
    delete inqueue.get ();
  if (iface)
    delete iface;
}

bool
CEMILayer2::enterBusmonitor ()
{
  if (!Layer2::enterBusmonitor ())
    return false;
  TRACEPRINTF (t, 2, this, "(CEMILayer2)  OpenBusmon not implemented");
  iface->SendReset ();

  return true;
}

bool
CEMILayer2::Open ()
{
  if (!Layer2::Open ())
    return false;
  TRACEPRINTF (t, 2, this, "(CEMILayer2) Open");
  iface->SendReset ();
  
  return true;
}

bool
CEMILayer2::Send_Queue_Empty ()
{
  return iface->Send_Queue_Empty () && inqueue.isempty();
}

void
CEMILayer2::Send_L_Data (LPDU * l)
{
  TRACEPRINTF (t, 2, this, "Send (CEMILayer2) Send_L_Data %s", l->Decode ()());
  if (l->getType () != L_Data)
    {
      delete l;
      return;
    }
  L_Data_PDU *l1 = (L_Data_PDU *) l;
  assert (l1->data () >= 1);
  /* discard long frames */
  /* actually cEMI supports long frames and splits long payload into multiple frames */
  /* but this is not implemented!! */
  if (l1->data () > 50)
    {
      TRACEPRINTF (t, 2, this, "Send (CEMILayer2) long_data! (%d)", l1->data ());
      delete l;
      return;
    }
  assert ((l1->hopcount & 0xf8) == 0);

  inqueue.put (l);
  pth_sem_inc (&in_signal, 1);
}

void
CEMILayer2::Send (LPDU * l)
{
  TRACEPRINTF (t, 1, this, "(CEMILayer2) Send %s", l->Decode ()());
  L_Data_PDU *l1 = (L_Data_PDU *) l;

  CArray pdu = L_Data_ToCEMI (0x11, *l1);
  iface->Send_Packet (pdu);
  delete l;
}

void
CEMILayer2::Run (pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  pth_event_t input = pth_event (PTH_EVENT_SEM, &in_signal);
  pth_event_t timeout = pth_event (PTH_EVENT_RTIME, pth_time (0, 0));
  bool wait_confirm = false;
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      if (!wait_confirm)
	pth_event_concat (stop, input, NULL);
      if (wait_confirm)
	pth_event_concat (stop, timeout, NULL);
      CArray *c = iface->Get_Packet (stop);
      pth_event_isolate(input);
      pth_event_isolate(timeout);
      if (!wait_confirm && !inqueue.isempty())
	{
	  pth_sem_dec (&in_signal);
	  Send(inqueue.get());
	  if (noqueue)
	    {
	      pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout,
			 pth_time (1, 0));
	      wait_confirm = true;
	    }
	}
      if (wait_confirm && pth_event_status(timeout) == PTH_STATUS_OCCURRED)
	wait_confirm = false;
      if (!c)
	continue;
      if (c->len () == 1 && (*c)[0] == 0xA0 && mode != BUSMODE_DOWN)
	{
	  TRACEPRINTF (t, 2, this, "ReInit");
	  iface->SendReset ();
	}
      if (c->len () && (*c)[0] == 0x2E)  /* 2Eh = L_Data.con */
	wait_confirm = false;
      if (c->len () && (*c)[0] == 0x29 && (mode & BUSMODE_UP)) /* 29h = L_Data.ind */
	{
	  L_Data_PDU *p = CEMI_to_L_Data (*c, this);
	  if (p)
	    {
	      delete c;
	      if (p->AddrType == IndividualAddress)
		p->dest = 0;
	      TRACEPRINTF (t, 2, this, "Recv %s", p->Decode ()());
	      l3->recv_L_Data (p);
	      continue;
	    }
	}
      if (c->len () > 4 && (*c)[0] == 0x2B && mode == BUSMODE_MONITOR) /* 2Bh = L_Busmon.ind */
	{
	  /* untested for cEMI !! */
	  L_Busmonitor_PDU *p = new L_Busmonitor_PDU (this);
	  p->status = (*c)[1];
	  p->timestamp = ((*c)[2] << 24) | ((*c)[3] << 16);
	  p->pdu.set (c->array () + 4, c->len () - 4);
	  delete c;
	  TRACEPRINTF (t, 2, this, "Recv %s", p->Decode ()());
	  l3->recv_L_Data (p);
	  continue;
	}
      delete c;
    }
  pth_event_free (stop, PTH_FREE_THIS);
  pth_event_free (input, PTH_FREE_THIS);
  pth_event_free (timeout, PTH_FREE_THIS);
}
