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

#include "emi.h"
#include "emi_common.h"
#include "layer3.h"

unsigned int maxPacketLen() { return 0x10; }

EMI_Common::EMI_Common (LowLevelDriver * i, 
                        L2options *opt) : Layer2(opt)
{
  TRACEPRINTF (t, 2, "Open");
  iface = i;
  if (opt && opt->send_delay) {
    send_delay = opt->send_delay / 1000;
    opt->send_delay = 0;
  } else
    send_delay = 0;

  if (!iface->init ())
    {
      delete iface;
      iface = 0;
      return;
    }
  trigger.set<EMI_Common, &EMI_Common::trigger_cb>(this);
  trigger.start();
  iface->on_recv.set<EMI_Common,&EMI_Common::on_recv_cb>(this);

  TRACEPRINTF (t, 2, "Opened");
}

bool
EMI_Common::init (Layer3 * l3)
{
  if (iface == 0)
    return false;
  if (! addGroupAddress(0))
    return false;
  return Layer2::init (l3);
}

EMI_Common::~EMI_Common ()
{
  TRACEPRINTF (t, 2, "Destroy");
  trigger.stop();
  if (iface)
    delete iface;
}

bool
EMI_Common::enterBusmonitor ()
{
  if (!Layer2::enterBusmonitor ())
    return false;
  TRACEPRINTF (t, 2, "OpenBusmon");
  iface->SendReset ();
  cmdEnterMonitor();

  // TODO: EMI1/EMI2: wait for queue-empty?
  return true;
}

bool
EMI_Common::leaveBusmonitor ()
{
  if (!Layer2::leaveBusmonitor ())
    return false;
  TRACEPRINTF (t, 2, "CloseBusmon");
  cmdLeaveMonitor();
  // TODO: EMI1/EMI2: wait for queue-empty?
  return true;
}

bool
EMI_Common::Open ()
{
  if (!Layer2::Open ())
    return false;
  TRACEPRINTF (t, 2, "OpenL2");
  iface->SendReset ();
  cmdOpen();

  return true;
}

bool
EMI_Common::Close ()
{
  if (!Layer2::Close ())
    return false;
  TRACEPRINTF (t, 2, "CloseL2");
  cmdClose();
  return true;
}

void
EMI_Common::send_L_Data (LDataPtr l)
{
  TRACEPRINTF (t, 2, "Send %s", l->Decode ().c_str());
  assert (l->data.size() >= 1);
  /* discard long frames, as they are not supported by EMI 1 */
  if (l->data.size() > maxPacketLen())
    {
      TRACEPRINTF (t, 2, "Oversize (%d), discarded", l->data.size());
      return;
    }
  assert ((l->hopcount & 0xf8) == 0);

  send_q.push (std::move(l));
  if (!wait_confirm)
    trigger.send();
}

void
EMI_Common::timeout_cb(ev::timer &w, int revents)
{
  wait_confirm = false;
  if (!send_q.isempty())
    trigger.send();
}
void

EMI_Common::Send (LDataPtr l)
{
  TRACEPRINTF (t, 1, "Send %s", l->Decode ().c_str());

  CArray pdu = lData2EMI (0x11, l);
  iface->Send_Packet (pdu);
}

void
EMI_Common::trigger_cb (ev::async &w, int revents)
{
  if (wait_confirm)
    return;
  while (!send_q.isempty())
    {
      Send(send_q.get());
      if (send_delay)
        {
          timeout.start(send_delay,0);
          wait_confirm = true;
          return;
        }
    }
}

void
EMI_Common::on_recv_cb(CArray *c)
{
  t->TracePacket (1, "RecvEMI", *c);
  const uint8_t *ind = getIndTypes();
  if (c->size() == 1 && (*c)[0] == 0xA0 && (mode & BUSMODE_UP))
    {
      TRACEPRINTF (t, 2, "Reopen");
      busmode_t old_mode = mode;
      mode = BUSMODE_DOWN;
      if (Open ())
        mode = old_mode; // restore VMONITOR
    }
  if (c->size() == 1 && (*c)[0] == 0xA0 && mode == BUSMODE_MONITOR)
    {
      TRACEPRINTF (t, 2, "Reopen Busmonitor");
      mode = BUSMODE_DOWN;
      enterBusmonitor ();
    }
  if (c->size() && (*c)[0] == ind[I_CONFIRM])
    wait_confirm = false;
  if (c->size() && (*c)[0] == ind[I_DATA] && (mode & BUSMODE_UP))
    {
      LDataPtr p = EMI2lData (*c, real_l2 ? real_l2 : shared_from_this());
      if (p)
        {
          delete c;
          TRACEPRINTF (t, 2, "Recv %s", p->Decode ().c_str());
          l3->recv_L_Data (std::move(p));
          return;
        }
    }
  if (c->size() > 4 && (*c)[0] == ind[I_BUSMON] && mode == BUSMODE_MONITOR)
    {
      LBusmonPtr p = LBusmonPtr(new L_Busmonitor_PDU (real_l2 ? real_l2 : shared_from_this()));
      p->status = (*c)[1];
      p->timestamp = ((*c)[2] << 24) | ((*c)[3] << 16);
      p->pdu.set (c->data() + 4, c->size() - 4);
      delete c;
      TRACEPRINTF (t, 2, "Recv %s", p->Decode ().c_str());
      l3->recv_L_Busmonitor (std::move(p));
      return;
    }
  delete c;
}

