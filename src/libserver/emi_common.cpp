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

unsigned int maxPacketLen() { return 0x10; }

EMI_Common::EMI_Common (LowLevelDriver * i, 
                        const LinkConnectPtr_& c, IniSection& s) : BusDriver(c,s)
{
  iface = i;
}

EMI_Common::EMI_Common (const LinkConnectPtr_& c, IniSection& s) : BusDriver(c,s)
{
}

bool
EMI_Common::setup()
{
  if(!BusDriver::setup())
    return false;
  if (!iface)
    {
      auto cn = conn.lock();
      if (cn == nullptr)
        return false;

      std::string ll;
      const std::string& n = name();
      if (n == "ft12")
        ll = "ft12";
      else if (n == "ft12cemi")
        ll = "ft12";
      else
        ll = "";
      ll = cfg.value("subdriver",ll);
      if (ll.size() == 0)
        {
          ERRORPRINTF(t, E_ERROR, "EMI_common: %s: no default subdriver= value known", n);
          return false;
        }
      iface = static_cast<Router&>(cn->router).get_lowlevel(std::dynamic_pointer_cast<Driver>(shared_from_this()), cfg, ll);
      if (iface == nullptr)
        {
          ERRORPRINTF(t, E_ERROR, "EMI_common: %s: no subdriver '%s' known", n, ll);
          return false;
        }
    }
  if (!iface->setup (std::dynamic_pointer_cast<Driver>(shared_from_this())))
    return false;
  iface->on_recv.set<EMI_Common,&EMI_Common::on_recv_cb>(this);
  send_delay = cfg.value("send-delay",0);
  monitor = cfg.value("monitor",false);
  return true;
}

EMI_Common::~EMI_Common ()
{
  TRACEPRINTF (t, 2, "Destroy");
  trigger.stop();
  if (iface)
    delete iface;
}

void
EMI_Common::start()
{
  TRACEPRINTF (t, 2, "OpenL2");
  trigger.set<EMI_Common, &EMI_Common::trigger_cb>(this);
  trigger.start();

  iface->SendReset ();
  if (monitor)
    cmdEnterMonitor();
  else
    cmdOpen();

  BusDriver::start();
}

void
EMI_Common::stop ()
{
  TRACEPRINTF (t, 2, "CloseL2");
  if (monitor)
    cmdLeaveMonitor();
  else
    cmdClose();
  trigger.stop();
  BusDriver::stop();
}

void
EMI_Common::send_L_Data (LDataPtr l)
{
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
  TRACEPRINTF (t, 1, "Send %s", l->Decode (t));

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
  if (c->size() == 1 && (*c)[0] == 0xA0)
    {
      TRACEPRINTF (t, 2, "Stopped");
      stopped();
    }
  if (c->size() && (*c)[0] == ind[I_CONFIRM])
    {
      if (wait_confirm)
        {
          wait_confirm = false;
          timeout.stop();
        }
    }
  if (c->size() && (*c)[0] == ind[I_DATA] && !monitor)
    {
      LDataPtr p = EMI2lData (*c);
      if (p)
        {
          auto r = recv.lock();
          if (r != nullptr)
            r->recv_L_Data (std::move(p));
        }
      else
        t->TracePacket (2, "unparseable EMI data", *c);
    }
  else if (c->size() > 4 && (*c)[0] == ind[I_BUSMON] && monitor)
    {
      LBusmonPtr p = LBusmonPtr(new L_Busmonitor_PDU ());
      p->status = (*c)[1];
      p->timestamp = ((*c)[2] << 24) | ((*c)[3] << 16);
      p->pdu.set (c->data() + 4, c->size() - 4);
      auto r = recv.lock();
      if (r != nullptr)
        r->recv_L_Busmonitor (std::move(p));
    }
  delete c;
}

