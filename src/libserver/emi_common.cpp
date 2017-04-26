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

EMIVer
cfgEMIVersion(IniSectionPtr& s)
{
  int v = s->value("version",vERROR);
  if (v > vRaw || v < vERROR)
    return vERROR;
  else if (v != vERROR)
    return EMIVer(v);

  std::string sv = s->value("version","");
  if (!sv.size())
    return vUnknown;
  else if (sv == "EMI1")
    return vEMI1;
  else if (sv == "EMI2")
    return vEMI2;
  else if (sv == "CEMI")
    return vCEMI;
  else if (sv == "raw")
    return vRaw;
  else
    return vERROR;
}

EMI_Common::EMI_Common (LowLevelIface* c, IniSectionPtr& s, LowLevelDriver *i) : LowLevelFilter(c,s,i)
{
  timeout.set<EMI_Common, &EMI_Common::timeout_cb>(this);
  t->setAuxName("EMI_common");
  iface = i;
}

bool
EMI_Common::setup()
{
  if (iface == nullptr)
    return false;
  if(!LowLevelFilter::setup())
    return false;
  send_timeout = cfg->value("send-timeout",300) / 1000.;
  max_retries = cfg->value("send-retries",3);
  monitor = cfg->value("monitor",false);

  return true;
}

EMI_Common::~EMI_Common ()
{
}

void
EMI_Common::start()
{
  state = E_idle;
  LowLevelFilter::start();
}

void
EMI_Common::started()
{
  TRACEPRINTF (t, 2, "OpenL2");
  if (monitor)
    cmdEnterMonitor();
  else
    cmdOpen();
}

void
EMI_Common::stop ()
{
  TRACEPRINTF (t, 2, "CloseL2");
  if (monitor)
    cmdLeaveMonitor();
  else
    cmdClose();
}

void
EMI_Common::send_L_Data (LDataPtr l)
{
  if (state != E_idle)
    {
      ERRORPRINTF(t, E_ERROR, "EMI_common: send while waiting");
      return;
    }
    
  assert (l->data.size() >= 1);
  // discard long frames, they are not supported yet
  // TODO add support for long frames!
  if (l->data.size() > maxPacketLen())
    {
      TRACEPRINTF (t, 2, "Oversize (%d), discarded", l->data.size());
      LowLevelFilter::do_send_Next();
      return;
    }
  CArray pdu = lData2EMI (0x11, l);
  out = pdu;
  retries = 0;
  send_Data (pdu);
}

void
EMI_Common::do_send_Next()
{
  if (state == E_wait)
    state = E_wait_confirm;
  else if (state == E_idle)
    LowLevelFilter::do_send_Next();
}

void
EMI_Common::send_Data(CArray &pdu)
{
  state = E_wait;
  timeout.start(send_timeout,0);
  iface->send_Data(pdu);
}

void
EMI_Common::timeout_cb(ev::timer &w UNUSED, int revents UNUSED)
{
  if (state <= E_timed_out)
    return;
  if (state == E_wait)
    {
      state = E_timed_out;
      errored();
      return;
    }
  assert (state == E_wait_confirm);
  if (++retries <= max_retries)
    {
      TRACEPRINTF (t, 1, "No confirm, repeating");
      send_Data(out);
      return;
    }

  // TODO raise an error instead?
  ERRORPRINTF(t, E_WARNING, "EMI: No confirm, packet discarded");

  state = E_idle;
  LowLevelFilter::do_send_Next();
}

void
EMI_Common::recv_Data(CArray& c)
{
  const uint8_t *ind = getIndTypes();
  if (c.size() > 0 && c[0] == 0xA0)
    {
      TRACEPRINTF (t, 2, "got reset ind");
      errored();
    }
  else if (c.size() > 0 && c[0] == ind[I_CONFIRM])
    {
      if (state == E_wait_confirm)
        {
          TRACEPRINTF (t, 2, "Confirmed");
          state = E_idle;
          timeout.stop();
          LowLevelFilter::do_send_Next();
        }
      else
        TRACEPRINTF (t, 2, "spurious Confirm %d",(int)state);
    }
  else if (c.size() > 0 && c[0] == ind[I_DATA] && !monitor)
    {
      LDataPtr p = EMI2lData (c);
      if (p)
        master->recv_L_Data (std::move(p));
      else
        t->TracePacket (2, "unparseable EMI data", c);
    }
  else if (c.size() > 4 && c[0] == ind[I_BUSMON] && monitor)
    {
      LBusmonPtr p = LBusmonPtr(new L_Busmonitor_PDU ());
      p->status = c[1];
      p->timestamp = (c[2] << 24) | (c[3] << 16);
      p->pdu.set (c.data() + 4, c.size() - 4);
      master->recv_L_Busmonitor (std::move(p));
    }
  else
    {
      TRACEPRINTF (t, 2, "unknown data");
    }
}

