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
cfgEMIVersion(IniSection& s)
{
  int v = s.value("version",vERROR);
  if (v > vRaw || v < vERROR)
    return vERROR;
  else if (v != vERROR)
    return EMIVer(v);

  std::string sv = s.value("version","");
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

EMI_Common::EMI_Common (LowLevelDriver * i, LowLevelIface* c, IniSection& s) : LowLevelFilter(i,c,s)
{
  timeout.set<EMI_Common, &EMI_Common::timeout_cb>(this);
  t->setAuxName("EMI_common");
  iface = i;
}

EMI_Common::EMI_Common (LowLevelIface* c, IniSection& s) : LowLevelFilter(c,s)
{
  t->setAuxName("EMIcommon");
}

bool
EMI_Common::setup()
{
  if (iface == nullptr)
    return false;
  if(!LowLevelFilter::setup())
    return false;
  if (!iface->setup ())
    return false;
  send_timeout = cfg.value("send-timeout",300) / 1000.;
  monitor = cfg.value("monitor",false);

  return true;
}

EMI_Common::~EMI_Common ()
{
  TRACEPRINTF (t, 2, "Destroy");
  if (iface)
    delete iface;
}

void
EMI_Common::started()
{
  TRACEPRINTF (t, 2, "OpenL2");
  if (monitor)
    cmdEnterMonitor();
  else
    cmdOpen();

  LowLevelFilter::started();
}

void
EMI_Common::stop ()
{
  TRACEPRINTF (t, 2, "CloseL2");
  if (monitor)
    cmdLeaveMonitor();
  else
    cmdClose();
  LowLevelFilter::stop();
}

void
EMI_Common::send_L_Data (LDataPtr l)
{
  if (wait_confirm)
    ERRORPRINTF(t, E_ERROR, "EMI_common: send while waiting");
    
  assert (l->data.size() >= 1);
  /* discard long frames, as they are not supported by EMI 1 */
  if (l->data.size() > maxPacketLen())
    {
      TRACEPRINTF (t, 2, "Oversize (%d), discarded", l->data.size());
      send_Next();
      return;
    }
  CArray pdu = lData2EMI (0x11, l);
  send_Data (pdu);
}

void
EMI_Common::send_Data(CArray &pdu)
{
  wait_confirm = true;
  timeout.start(send_timeout,0);
  iface->send_Data(pdu);
}

void
EMI_Common::timeout_cb(ev::timer &w UNUSED, int revents UNUSED)
{
  TRACEPRINTF (t, 1, "No confirm, continuing");

  wait_confirm = false;
  send_Next();
}

void
EMI_Common::recv_Data(CArray& c)
{
  t->TracePacket (1, "RecvEMI", c);
  const uint8_t *ind = getIndTypes();
  if (c.size() == 1 && c[0] == 0xA0)
    {
      TRACEPRINTF (t, 2, "Stopped");
      stopped();
    }
  else if (c.size() && c[0] == ind[I_CONFIRM])
    {
      if (wait_confirm)
        {
          TRACEPRINTF (t, 2, "Confirmed");
          wait_confirm = false;
          timeout.stop();
          send_Next();
        }
      else
        TRACEPRINTF (t, 2, "spurious Confirm");
    }
  else if (c.size() && c[0] == ind[I_DATA] && !monitor)
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

