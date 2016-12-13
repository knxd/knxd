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

EMI2Layer2::EMI2Layer2 (LowLevelDriver * i, 
                        L2options *opt) : Layer2 (opt)
{
  TRACEPRINTF (t, 2, this, "Open");
  iface = i;
  if (opt && opt->send_delay) {
    send_delay = pth_time(opt->send_delay / 1000, (opt->send_delay % 1000) * 1000);
    opt->send_delay = 0;
  } else
    send_delay = pth_null_time;

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
EMI2Layer2::init (Layer3 * l3)
{
  if (iface == 0)
    return false;
  if (! addGroupAddress(0))
    return false;
  return Layer2::init (l3);
}

EMI2Layer2::~EMI2Layer2 ()
{
  TRACEPRINTF (t, 2, this, "Destroy");
  Stop ();
  while (!inqueue.isempty ())
    delete inqueue.get ();
  if (iface)
    delete iface;
}

bool
EMI2Layer2::enterBusmonitor ()
{
  if (!Layer2::enterBusmonitor ())
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
EMI2Layer2::leaveBusmonitor ()
{
  if (!Layer2::leaveBusmonitor ())
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
  return true;
}

bool
EMI2Layer2::Open ()
{
  if (!Layer2::Open ())
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
  return true;
}

bool
EMI2Layer2::Close ()
{
  if (!Layer2::Close ())
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
EMI2Layer2::Send_Queue_Empty ()
{
  return iface->Send_Queue_Empty () && inqueue.isempty();
}


void
EMI2Layer2::Send_L_Data (LPDU * l)
{
  TRACEPRINTF (t, 2, this, "Send %s", l->Decode ()());
  if (l->getType () != L_Data)
    {
      delete l;
      return;
    }
  L_Data_PDU *l1 = (L_Data_PDU *) l;
  assert (l1->data.size() >= 1);
  /* discard long frames, as they are not supported by EMI 2 */
  if (l1->data.size() > 0x10)
    return;
  assert (l1->data.size() <= 0x10);
  assert ((l1->hopcount & 0xf8) == 0);

  inqueue.put (l);
  pth_sem_inc (&in_signal, 1);
}

void
EMI2Layer2::Send (LPDU * l)
{
  TRACEPRINTF (t, 1, this, "Send %s", l->Decode ()());
  L_Data_PDU *l1 = (L_Data_PDU *) l;

  CArray pdu = L_Data_ToEMI (0x11, *l1);
  iface->Send_Packet (pdu);
  delete l;
}

void
EMI2Layer2::Run (pth_sem_t * stop1)
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
	  if (send_delay != pth_null_time)
	    {
	      pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE, timeout, send_delay);
	      wait_confirm = true;
	    }
	}
      if (wait_confirm && pth_event_status(timeout) == PTH_STATUS_OCCURRED)
	wait_confirm = false;
      if (!c)
	continue;
      if (c->size() == 1 && (*c)[0] == 0xA0 && (mode & BUSMODE_UP))
	{
	  TRACEPRINTF (t, 2, this, "Reopen");
          busmode_t old_mode = mode;
	  mode = BUSMODE_DOWN;
	  if (Open ())
            mode = old_mode;
	}
      if (c->size() == 1 && (*c)[0] == 0xA0 && mode == BUSMODE_MONITOR)
	{
	  TRACEPRINTF (t, 2, this, "Reopen Busmonitor");
	  mode = BUSMODE_DOWN;
	  enterBusmonitor ();
	}
      if (c->size() && (*c)[0] == 0x2E)
	wait_confirm = false;
      if (c->size() && (*c)[0] == 0x29 && (mode & BUSMODE_UP))
	{
	  L_Data_PDU *p = EMI_to_L_Data (*c, shared_from_this());
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
      if (c->size() > 4 && (*c)[0] == 0x2B && mode == BUSMODE_MONITOR)
	{
	  L_Busmonitor_PDU *p = new L_Busmonitor_PDU (shared_from_this());
	  p->status = (*c)[1];
	  p->timestamp = ((*c)[2] << 24) | ((*c)[3] << 16);
	  p->pdu.set (c->data () + 4, c->size() - 4);
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
