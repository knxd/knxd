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

#include "eibusb.h"
#include "emi1.h"
#include "emi2.h"
#include "cemi.h"
#include "usblowlevel.h"

USBConverterInterface::USBConverterInterface (LowLevelIface * p, IniSectionPtr& s)
    : LowLevelFilter(p,s)
{
  t->setAuxName("Conv");
  sendLocal_done.set<USBConverterInterface,&USBConverterInterface::sendLocal_done_cb>(this);
  iface = new USBLowLevelDriver(this, s);
}

USBConverterInterface::~USBConverterInterface ()
{
}

void
USBConverterInterface::send_Data (CArray& l)
{
  if (version == vRaw)
    {
      iface->send_Data (l);
      return;
    }
  if (version < vEMI1 || version > vCEMI)
    {
      ERRORPRINTF (t, E_ERROR, "EMI version %s during send",version);
      return;
    }
  t->TracePacket (0, "Send-EMI", l);
  CArray out;
  unsigned int j, l1;
  l1 = l.size();
  out.resize (64);
  if (l1 + 11 > 64)
    l1 = 64 - 11;
  for (j = 0; j < out.size(); j++)
    out[j] = 0;
  for (j = 0; j < l1; j++)
    out[j + 11] = l[j];
  out[0] = 1;
  out[1] = 0x13;
  out[2] = l1 + 8;
  out[3] = 0x0;
  out[4] = 0x08;
  out[5] = 0x00;
  out[6] = l1;
  out[7] = 0x01;
  out[8] = version;
  iface->send_Data (out);
}

void
USBConverterInterface::recv_Data(CArray& res)
{
  if (version == vRaw)
    {
      //t->TracePacket (0, "RecvEMI raw", res);
      LowLevelFilter::recv_Data(res);
      return;
    }

  if (res.size() != 64)
    goto out;
  if (res[0] != 0x01)
    goto out;
  if ((res[1] & 0x0f) != 3) // single-frame packet // TODO
    goto out;
  if (res[2] > 64 - 8) // full packet length
    goto out;
  if (res[3] != 0)
    goto out;
  if (res[4] != 8)
    goto out;
  if (res[5] != 0)
    goto out;
  if (res[6] + 11 > 64) // len
    goto out;
  if (res[7] != 1)
    goto out;
  switch (version)
    {
    case vEMI1:
      if (res[8] != 1)
        goto out;
      break;
    case vEMI2:
      if (res[8] != 2)
        goto out;
      break;
    case vCEMI:
      if (res[8] != 3)
        goto out;
      break;
    default:
      goto out;
    }

  {
    CArray res1(res.data() + 11, res[6]);
    t->TracePacket (0, "RecvEMI", res1);
    LowLevelFilter::recv_Data(res1);
    return;
  }

out:
  t->TracePacket (8, "RecvEMI:deleted", res);
  return;
}

USBDriver::USBDriver (const LinkConnectPtr_& c, IniSectionPtr& s) : LowLevelAdapter (c,s)
{
  t->setAuxName("USBdr");
  sendLocal_done.set<USBDriver,&USBDriver::sendLocal_done_cb>(this);
}

void
USBConverterInterface::send_Init()
{
  TRACEPRINTF (t, 2, "send_Init %d",version);

  uchar init[64] = {
    0x01, 0x13, 0x0a, 0x00, 0x08, 0x00, 0x02, 0x0f, 0x03, 0x00, 0x00, 0x05, 0x01
  };
  init[12] = version;
  send_Local (CArray (init, sizeof (init)), 1);
}

void
USBConverterInterface::sendLocal_done_cb(bool success)
{
  if (!success)
    errored();
  else
    LowLevelFilter::started();
}

void 
USBDriver::started()
{
  if (version == vUnknown)
    {
      version = vDiscovery;
      timeout.set<USBDriver,&USBDriver::timeout_cb>(this);
      xmit();
      return;
    }

  LowLevelAdapter::started();
}

void 
USBDriver::timeout_cb(ev::timer &w UNUSED, int revents UNUSED)
{
  if (++cnt < 5)
    xmit();
  else
    {
      ERRORPRINTF (t, E_ERROR, "No reply to setup");
      version = vTIMEOUT;
      errored();
    }
}

void
USBDriver::stopped()
{
  if (version == vDiscovery)
    timeout.stop();
  LowLevelAdapter::stopped();
}

void
USBDriver::do_send_Next()
{
  if (version >= vEMI1 && version <= vCEMI)
    BusDriver::send_Next();
}

void
USBDriver::xmit()
{
  const uchar ask[64] = {
    0x01, 0x13, 0x09, 0x00, 0x08, 0x00, 0x01, 0x0f, 0x01, 0x00, 0x00, 0x01
  };
  timeout.start(1,0);
  send_Local (CArray (ask, sizeof (ask)), 1);
}

void
USBDriver::sendLocal_done_cb(bool success)
{
  if (!success)
    {
      timeout.stop();
      errored();
    }
  else if (wait_make)
    {
      wait_make = false;
      if(!make_EMI())
        errored();
    }
  else if (version > vERROR && version < vRaw)
    iface->started();
}

void
USBDriver::recv_Data(CArray& c)
{
  t->TracePacket (2, "recv_Data", c);
  if (version > vERROR && version <= vRaw)
    {
      LowLevelAdapter::recv_Data(c);
      return;
    }
  if (version != vDiscovery)
    {
      TRACEPRINTF (t, 2, "Data during version=%d", version);
      return;
    }
  if (c.size() != 64)
    goto cont;
  if (c[0] != 01)
    goto cont;
  if ((c[1] & 0x0f) != 0x03)
    goto cont;
  if (c[2] != 0x0b)
    goto cont;
  if (c[3] != 0x00)
    goto cont;
  if (c[4] != 0x08)
    goto cont;
  if (c[5] != 0x00)
    goto cont;
  if (c[6] != 0x03)
    goto cont;
  if (c[7] != 0x0F)
    goto cont;
  if (c[8] != 0x02)
    goto cont;
  if (c[11] != 0x01)
    goto cont;
  if (c[13] & 0x2)
    version = vEMI2;
  else if (c[13] & 0x1)
    version = vEMI1;
  else if (c[13] & 0x4)
    version = vCEMI;
  else
    {
      TRACEPRINTF (t, 2, "version x%02x not recognized", c[13]);
      version = vERROR;
      timeout.stop();
      errored();
      return;
    }

  timeout.stop();
  TRACEPRINTF (t, 1, "Using EMI version %d", version);

  if (is_local) // the inquiry's send_local has not yet been acked
    {
      TRACEPRINTF (t, 0, "version x%02x recognized, delayed", c[13]);
      wait_make = true;
      return;
    }

  if(!make_EMI())
    {
      errored();
      return;
    }

  return;

cont:
  TRACEPRINTF (t, 2, "not recognized");
  return;
}

bool
USBDriver::make_EMI()
{
  if (version != vUnknown)
    {
      usb_iface->version = version;
      usb_iface->send_Init();
    }
  switch (version)
    {
    case vEMI1:
      iface = new EMI1Driver (this,cfg, iface);
      break;
    case vEMI2:
      iface = new EMI2Driver (this,cfg, iface);
      break;
    case vCEMI:
      iface = new CEMIDriver (this,cfg, iface);
      break;
    case vRaw:
      return true;
    case vUnknown:
      iface = nullptr;
      return true;
    default:
      TRACEPRINTF (t, 2, "Unsupported EMI");
      return false;
    }

  if (!iface->setup())
    {
      // delete only this, not the whole stack.
      dynamic_cast<LowLevelFilter *>(iface)->iface = nullptr;
      return false;
    }
  return true;
}

bool
USBDriver::setup ()
{
  LowLevelDriver *orig_iface;

  version = cfgEMIVersion(cfg);
  if (version == vERROR)
    {
      std::string v = cfg->value("version","");
      ERRORPRINTF (t, E_ERROR, "EMI version %s not recognized",v);
      return false;
    }

  if (iface != nullptr)
    {
      ERRORPRINTF (t, E_WARNING, "interface in setup??");
      delete iface;
    }
  usb_iface = new USBConverterInterface(this,cfg);
  iface = usb_iface;
  if (!iface->setup())
    goto ex1;

  orig_iface = iface;
  if(!make_EMI())
    goto ex1;

  if (iface == nullptr)
    iface = orig_iface;
  if (!LowLevelAdapter::setup())
    return false;
  return true;
ex1:
  delete iface;
  return false;
}

