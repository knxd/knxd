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

#include "emi2.h"
#include "emi.h"
#include "layer3.h"

bool
EMI2Layer2Interface::addAddress (eibaddr_t addr UNUSED)
{
  return 0;
}

bool
EMI2Layer2Interface::removeAddress (eibaddr_t addr UNUSED)
{
  return 0;
}

bool
EMI2Layer2Interface::Connection_Lost ()
{
  return iface->Connection_Lost ();
}

EMI2Layer2Interface::EMI2Layer2Interface (LowLevelDriverInterface * i,
					  Layer3 * l3, int flags) : Layer2Interface (l3)
{
  TRACEPRINTF (t, 2, this, "Open");
  iface = i;
  noqueue = flags & FLAG_B_EMI_NOQUEUE;
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
EMI2Layer2Interface::init ()
{
  return iface != 0;
}

EMI2Layer2Interface::~EMI2Layer2Interface ()
{
  TRACEPRINTF (t, 2, this, "Destroy");
  Stop ();
  if (iface)
    delete iface;
}

bool
EMI2Layer2Interface::enterBusmonitor ()
{
  if (!Layer2Interface::enterBusmonitor ())
    return false;
  const uchar t1[] = { 0xa9, 0x1E, 0x12, 0x34, 0x56, 0x78, 0x9a };
  const uchar t2[] = { 0xa9, 0x90, 0x18, 0x34, 0x45, 0x67, 0x8a };
  TRACEPRINTF (t, 2, this, "OpenBusmon");
  iface->SendReset ();
  iface->Send_Packet (CArray (t1, sizeof (t1)));
  iface->Send_Packet (CArray (t2, sizeof (t2)));

  if (!iface->Send_Queue_Empty ())
    {
      pth_event_t
	e = pth_event (PTH_EVENT_SEM, iface->Send_Queue_Empty_Cond ());
      pth_wait (e);
      pth_event_free (e, PTH_FREE_THIS);
    }
  return true;
}

bool
EMI2Layer2Interface::leaveBusmonitor ()
{
  if (!Layer2Interface::leaveBusmonitor ())
    return false;
  TRACEPRINTF (t, 2, this, "CloseBusmon");
  uchar t[] = { 0xa9, 0x1E, 0x12, 0x34, 0x56, 0x78, 0x9a };
  iface->Send_Packet (CArray (t, sizeof (t)));
  while (!iface->Send_Queue_Empty ())
    {
      pth_event_t
	e = pth_event (PTH_EVENT_SEM, iface->Send_Queue_Empty_Cond ());
      pth_wait (e);
      pth_event_free (e, PTH_FREE_THIS);
    }
  return 1;
}

bool
EMI2Layer2Interface::Open ()
{
  if (!Layer2Interface::Open ())
    return false;
  const uchar t1[] = { 0xa9, 0x1E, 0x12, 0x34, 0x56, 0x78, 0x9a };
  const uchar t2[] = { 0xa9, 0x00, 0x18, 0x34, 0x56, 0x78, 0x0a };
  TRACEPRINTF (t, 2, this, "OpenL2");
  iface->SendReset ();
  iface->Send_Packet (CArray (t1, sizeof (t1)));
  iface->Send_Packet (CArray (t2, sizeof (t2)));

  while (!iface->Send_Queue_Empty ())
    {
      pth_event_t
	e = pth_event (PTH_EVENT_SEM, iface->Send_Queue_Empty_Cond ());
      pth_wait (e);
      pth_event_free (e, PTH_FREE_THIS);
    }
  return 1;
}

bool
EMI2Layer2Interface::Close ()
{
  if (!Layer2Interface::Close ())
    return false;
  TRACEPRINTF (t, 2, this, "CloseL2");
  uchar t[] = { 0xa9, 0x1E, 0x12, 0x34, 0x56, 0x78, 0x9a };
  iface->Send_Packet (CArray (t, sizeof (t)));
  if (!iface->Send_Queue_Empty ())
    {
      pth_event_t
	e = pth_event (PTH_EVENT_SEM, iface->Send_Queue_Empty_Cond ());
      pth_wait (e);
      pth_event_free (e, PTH_FREE_THIS);
    }
  return true;
}

bool
EMI2Layer2Interface::Send_Queue_Empty ()
{
  return iface->Send_Queue_Empty () && inqueue.isempty();
}


void
EMI2Layer2Interface::Send_L_Data (LPDU * l)
{
  TRACEPRINTF (t, 2, this, "Send %s", l->Decode ()());
  if (l->getType () != L_Data)
    {
      delete l;
      return;
    }
  L_Data_PDU *l1 = (L_Data_PDU *) l;
  assert (l1->data () >= 1);
  /* discard long frames, as they are not supported by EMI 2 */
  if (l1->data () > 0x10)
    return;
  assert (l1->data () <= 0x10);
  assert ((l1->hopcount & 0xf8) == 0);

  inqueue.put (l);
  pth_sem_inc (&in_signal, 1);
}

void
EMI2Layer2Interface::Send (LPDU * l)
{
  TRACEPRINTF (t, 1, this, "Send %s", l->Decode ()());
  L_Data_PDU *l1 = (L_Data_PDU *) l;

  CArray pdu = L_Data_ToEMI (0x11, *l1);
  iface->Send_Packet (pdu);
  if (mode == BUSMODE_VMONITOR)
    {
      L_Busmonitor_PDU *l2 = new L_Busmonitor_PDU (this);
      l2->pdu.set (l->ToPacket ());
      l3->recv_L_Data (l2);
    }
  l3->recv_L_Data (l);
}

void
EMI2Layer2Interface::Run (pth_sem_t * stop1)
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
      if (c->len () == 1 && (*c)[0] == 0xA0 && (mode & BUSMODE_UP))
	{
	  TRACEPRINTF (t, 2, this, "Reopen");
          busmode_t old_mode = mode;
	  mode = BUSMODE_DOWN;
	  if (Open ())
            mode = old_mode;
	}
      if (c->len () == 1 && (*c)[0] == 0xA0 && mode == BUSMODE_MONITOR)
	{
	  TRACEPRINTF (t, 2, this, "Reopen Busmonitor");
	  mode = BUSMODE_DOWN;
	  enterBusmonitor ();
	}
      if (c->len () && (*c)[0] == 0x2E)
	sendmode = 0;
      if (c->len () && (*c)[0] == 0x29 && (mode & BUSMODE_UP))
	{
	  L_Data_PDU *p = EMI_to_L_Data (*c, this);
	  if (p)
	    {
	      delete c;
	      if (p->AddrType == IndividualAddress)
		p->dest = 0;
	      TRACEPRINTF (t, 2, this, "Recv %s", p->Decode ()());
	      if (mode == BUSMODE_VMONITOR)
		{
		  L_Busmonitor_PDU *l2 = new L_Busmonitor_PDU (this);
		  l2->pdu.set (p->ToPacket ());
		  l3->recv_L_Data (l2);
		}
	      l3->recv_L_Data (p);
	      continue;
	    }
	}
      if (c->len () > 4 && (*c)[0] == 0x2B && mode == BUSMODE_MONITOR)
	{
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
