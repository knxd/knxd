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

#include "emi1.h"
#include "emi.h"

bool
EMI1Layer2Interface::Connection_Lost ()
{
  return iface->Connection_Lost ();
}

bool
EMI1Layer2Interface::addAddress (eibaddr_t addr)
{
  return 0;
}

bool
EMI1Layer2Interface::removeAddress (eibaddr_t addr)
{
  return 0;
}

bool
EMI1Layer2Interface::openVBusmonitor ()
{
  vmode = 1;
  return 1;
}

bool
EMI1Layer2Interface::closeVBusmonitor ()
{
  vmode = 0;
  return 1;
}

EMI1Layer2Interface::EMI1Layer2Interface (LowLevelDriverInterface * i,
					  Layer3 * l3, int flags) : Layer2Interface(l3)
{
  TRACEPRINTF (t, 2, this, "Open");
  iface = i;
  mode = 0;
  vmode = 0;
  noqueue = flags & FLAG_B_EMI_NOQUEUE;
  pth_sem_init (&out_signal);
  pth_sem_init (&in_signal);
  getwait = pth_event (PTH_EVENT_SEM, &out_signal);
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
EMI1Layer2Interface::init ()
{
  return iface != 0;
}

EMI1Layer2Interface::~EMI1Layer2Interface ()
{
  TRACEPRINTF (t, 2, this, "Destroy");
  Stop ();
  pth_event_free (getwait, PTH_FREE_THIS);
  while (!outqueue.isempty ())
    delete outqueue.get ();
  while (!inqueue.isempty ())
    delete inqueue.get ();
  if (iface)
    delete iface;
}

bool
EMI1Layer2Interface::enterBusmonitor ()
{
  const uchar t1[] = { 0x46, 0x01, 0x00, 0x60, 0x90 };
  TRACEPRINTF (t, 2, this, "OpenBusmon");
  if (mode != 0)
    return 0;
  iface->SendReset ();
  pth_usleep (1000000);
  iface->Send_Packet (CArray (t1, sizeof (t1)));

  if (!iface->Send_Queue_Empty ())
    {
      pth_event_t
	e = pth_event (PTH_EVENT_SEM, iface->Send_Queue_Empty_Cond ());
      pth_wait (e);
      pth_event_free (e, PTH_FREE_THIS);
    }
  mode = 1;
  return 1;
}

bool
EMI1Layer2Interface::leaveBusmonitor ()
{
  if (mode != 1)
    return 0;
  TRACEPRINTF (t, 2, this, "CloseBusmon");
  uchar t[] =
  {
  0x46, 0x01, 0x00, 0x60, 0xc0};
  iface->Send_Packet (CArray (t, sizeof (t)));
  while (!iface->Send_Queue_Empty ())
    {
      pth_event_t
	e = pth_event (PTH_EVENT_SEM, iface->Send_Queue_Empty_Cond ());
      pth_wait (e);
      pth_event_free (e, PTH_FREE_THIS);
    }
  mode = 0;
  pth_usleep (1000000);
  return 1;
}

bool
EMI1Layer2Interface::Open ()
{
  const uchar t1[] = { 0x46, 0x01, 0x00, 0x60, 0x12 };
  TRACEPRINTF (t, 2, this, "OpenL2");
  if (mode != 0)
    return 0;
  iface->SendReset ();
  iface->Send_Packet (CArray (t1, sizeof (t1)));

  while (!iface->Send_Queue_Empty ())
    {
      pth_event_t
	e = pth_event (PTH_EVENT_SEM, iface->Send_Queue_Empty_Cond ());
      pth_wait (e);
      pth_event_free (e, PTH_FREE_THIS);
    }
  mode = 2;
  return 1;
}

bool
EMI1Layer2Interface::Close ()
{
  if (mode != 2)
    return 0;
  TRACEPRINTF (t, 2, this, "CloseL2");
  uchar t[] =
  {
  0x46, 0x01, 0x00, 0x60, 0xc0};
  iface->Send_Packet (CArray (t, sizeof (t)));
  if (!iface->Send_Queue_Empty ())
    {
      pth_event_t
	e = pth_event (PTH_EVENT_SEM, iface->Send_Queue_Empty_Cond ());
      pth_wait (e);
      pth_event_free (e, PTH_FREE_THIS);
    }
  mode = 0;
  return 1;
}

bool
EMI1Layer2Interface::Send_Queue_Empty ()
{
  return iface->Send_Queue_Empty () && inqueue.isempty();
}

void
EMI1Layer2Interface::Send_L_Data (LPDU * l)
{
  TRACEPRINTF (t, 2, this, "Send %s", l->Decode ()());
  if (l->getType () != L_Data)
    {
      delete l;
      return;
    }
  L_Data_PDU *l1 = (L_Data_PDU *) l;
  assert (l1->data () >= 1);
  /* discard long frames, as they are not supported by EMI 1 */
  if (l1->data () > 0x10)
    return;
  assert (l1->data () <= 0x10);
  assert ((l1->hopcount & 0xf8) == 0);

  inqueue.put (l);
  pth_sem_inc (&in_signal, 1);
}

void
EMI1Layer2Interface::Send (LPDU * l)
{
  TRACEPRINTF (t, 1, this, "Send %s", l->Decode ()());
  L_Data_PDU *l1 = (L_Data_PDU *) l;

  CArray pdu = L_Data_ToEMI (0x11, *l1);
  iface->Send_Packet (pdu);
  if (vmode)
    {
      L_Busmonitor_PDU *l2 = new L_Busmonitor_PDU (this);
      l2->pdu.set (l->ToPacket ());
      outqueue.put (l2);
      pth_sem_inc (&out_signal, 1);
    }
  outqueue.put (l);
  pth_sem_inc (&out_signal, 1);
}

LPDU *
EMI1Layer2Interface::Get_L_Data (pth_event_t stop)
{
  if (stop != NULL)
    pth_event_concat (getwait, stop, NULL);

  pth_wait (getwait);

  if (stop)
    pth_event_isolate (getwait);

  if (pth_event_status (getwait) == PTH_STATUS_OCCURRED)
    {
      pth_sem_dec (&out_signal);
      LPDU *l = outqueue.get ();
      TRACEPRINTF (t, 2, this, "Recv %s", l->Decode ()());
      return l;
    }
  else
    return 0;
}

void
EMI1Layer2Interface::Run (pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  pth_event_t input = pth_event (PTH_EVENT_SEM, &in_signal);
  pth_event_t timeout = pth_event (PTH_EVENT_RTIME, pth_time (0, 0));
  sendmode = 0;
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      if (sendmode == 0)
	pth_event_concat (stop, input, NULL);
      if (sendmode == 1)
	pth_event_concat (stop, timeout, NULL);
      CArray *c = iface->Get_Packet (stop);
      pth_event_isolate(input);
      pth_event_isolate(timeout);
      if (!inqueue.isempty() && sendmode == 0)
	{
	  Send(inqueue.get());
	  if (noqueue)
	    {
	      pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout,
			 pth_time (1, 0));
	      sendmode = 1;
	    }
	  else
	    sendmode = 0;
	}
      if (sendmode == 1 && pth_event_status(timeout) == PTH_STATUS_OCCURRED)
	sendmode = 0;
      if (!c)
	continue;
      if (c->len () == 1 && (*c)[0] == 0xA0 && mode == 2)
	{
	  TRACEPRINTF (t, 2, this, "Reopen");
	  mode = 0;
	  Open ();
	}
      if (c->len () == 1 && (*c)[0] == 0xA0 && mode == 1)
	{
	  TRACEPRINTF (t, 2, this, "Reopen Busmonitor");
	  mode = 0;
	  enterBusmonitor ();
	}
      if (c->len () && (*c)[0] == 0x4E)
	sendmode = 0;
      if (c->len () && (*c)[0] == 0x49 && mode == 2)
	{
	  L_Data_PDU *p = EMI_to_L_Data (*c, this);
	  if (p)
	    {
	      delete c;
	      if (p->AddrType == IndividualAddress)
		p->dest = 0;
	      TRACEPRINTF (t, 2, this, "Recv %s", p->Decode ()());
	      if (vmode)
		{
		  L_Busmonitor_PDU *l2 = new L_Busmonitor_PDU (this);
		  l2->pdu.set (p->ToPacket ());
		  outqueue.put (l2);
		  pth_sem_inc (&out_signal, 1);
		}
	      outqueue.put (p);
	      pth_sem_inc (&out_signal, 1);
	      continue;
	    }
	}
      if (c->len () > 4 && (*c)[0] == 0x49 && mode == 1)
	{
	  L_Busmonitor_PDU *p = new L_Busmonitor_PDU (this);
	  p->status = (*c)[1];
	  p->timestamp = ((*c)[2] << 24) | ((*c)[3] << 16);
	  p->pdu.set (c->array () + 4, c->len () - 4);
	  delete c;
	  TRACEPRINTF (t, 2, this, "Recv %s", p->Decode ()());
	  outqueue.put (p);
	  pth_sem_inc (&out_signal, 1);
	  continue;
	}
      delete c;
    }
  pth_event_free (stop, PTH_FREE_THIS);
  pth_event_free (input, PTH_FREE_THIS);
  pth_event_free (timeout, PTH_FREE_THIS);
}
