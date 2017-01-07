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

class initUSB
{
    LowLevelDriver *i;
    ev::timer timeout;
    int cnt = 0;
    EMIVer emiver;

public:
    initUSB(LowLevelDriver *iface) {
        i = iface;
        timeout.set<initUSB,&initUSB::timeout_cb>(this);
        i->on_recv.set<initUSB,&initUSB::read_cb>(this);
    }
    ~initUSB() {
        timeout.stop();
        i->reset();
    }
private:
    void timeout_cb(ev::timer &w, int revents) {
        if (++cnt < 5)
            xmit();
        else
            emiver = vTIMEOUT;
    }
    void xmit() {
        const uchar ask[64] = {
            0x01, 0x13, 0x09, 0x00, 0x08, 0x00, 0x01, 0x0f, 0x01, 0x00, 0x00, 0x01
        };
        i->Send_Packet (CArray (ask, sizeof (ask)));
        timeout.start(5,0);
    }
    void read_cb(CArray *r1) {
        CArray &r = *r1;
        if (r.size() != 64)
            goto cont;
        if (r[0] != 01)
            goto cont;
        if ((r[1] & 0x0f) != 0x03)
            goto cont;
        if (r[2] != 0x0b)
            goto cont;
        if (r[3] != 0x00)
            goto cont;
        if (r[4] != 0x08)
            goto cont;
        if (r[5] != 0x00)
            goto cont;
        if (r[6] != 0x03)
            goto cont;
        if (r[7] != 0x0F)
            goto cont;
        if (r[8] != 0x02)
            goto cont;
        if (r[11] != 0x01)
            goto cont;
        if (r[13] & 0x2)
            emiver = vEMI2;
        else if (r[13] & 0x1)
            emiver = vEMI1;
        else if (r[13] & 0x4)
            emiver = vCEMI;
        else
            emiver = vERROR;
    cont:
        delete r1;
    }
public:
    EMIVer run() {
        emiver = vDiscovery;
        xmit();
        do {
            ev_run(EV_DEFAULT_ EVRUN_ONCE);
        } while (emiver == vDiscovery);
        assert (emiver != vDiscovery);
        return emiver;
    }
};

LowLevelDriver *
initUSBDriver (LowLevelDriver * i, TracePtr tr)
{
  CArray r1, *r = 0;
  LowLevelDriver *iface;
  EMIVer emiver = vDiscovery;
  int cnt = 0;
  uchar init[64] = {
    0x01, 0x13, 0x0a, 0x00, 0x08, 0x00, 0x02, 0x0f, 0x03, 0x00, 0x00, 0x05,
    0x01
  };

  if (!i->init ())
    {
      delete i;
      return 0;
    }

  {
    initUSB iu(i);
    emiver = iu.run();
  }

  switch (emiver)
    {
    case vEMI1:
    case vEMI2:
    case vCEMI:
      init[12] = emiver;
      i->Send_Packet (CArray (init, sizeof (init)));
      iface = new USBConverterInterface (i, tr, emiver);
      break;
    case vTIMEOUT:
      TRACEPRINTF (tr, 1, i, "Timeout reading EMI version");
      goto ex;
    case vERROR:
      TRACEPRINTF (tr, 1, i, "Error reading EMI version");
      goto ex;
    default:
      TRACEPRINTF (tr, 1, i, "Unsupported EMI %d", emiver);
    ex:
      delete i;
      return 0;
    }
  return iface;
}

USBConverterInterface::USBConverterInterface (LowLevelDriver * iface,
					      TracePtr tr, EMIVer ver)
    : LowLevelDriver(tr)
{
  i = iface;
  v = ver;

  i->on_recv.set<USBConverterInterface, &USBConverterInterface::on_recv_cb>(this);

  switch (v)
    {
    case vEMI1:
      TRACEPRINTF (t, 1, this, "EMI1");
      break;
    case vEMI2:
      TRACEPRINTF (t, 1, this, "EMI2");
      break;
    case vCEMI:
      TRACEPRINTF (t, 1, this, "CEMI");
      break;
    default:
      TRACEPRINTF (t, 1, this, "Unknown EMI");
    }
}

USBConverterInterface::~USBConverterInterface ()
{
  delete i;
}

bool USBConverterInterface::init ()
{
  return i->init ();
}

void
USBConverterInterface::Send_Packet (CArray l)
{
  t->TracePacket (0, this, "Send-EMI", l);
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
  switch (v)
    {
    case vEMI1:
      out[8] = 0x01;
      break;
    case vEMI2:
      out[8] = 0x02;
      break;
    case vCEMI:
      out[8] = 0x03;
      break;
    default:
      return; // should not happen
    }
  i->Send_Packet (out);
}

void
USBConverterInterface::on_recv_cb(CArray *res1)
{
  CArray res = *res1;
  if (res.size() != 64)
    goto out;
  if (res[0] != 0x01)
    goto out;
  if ((res[1] & 0x0f) != 3)
    goto out;
  if (res[2] > 64 - 8)
    goto out;
  if (res[3] != 0)
    goto out;
  if (res[4] != 8)
    goto out;
  if (res[5] != 0)
    goto out;
  if (res[6] + 11 > 64)
    goto out;
  if (res[7] != 1)
    goto out;
  switch (v)
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
  res1->set (res.data() + 11, res[6]);
  t->TracePacket (0, this, "RecvEMI", *res1);
  on_recv(res1);
  return;

out:
  delete res1;
  return;
}

void
USBConverterInterface::SendReset ()
{
  return i->SendReset ();
}

EMIVer USBConverterInterface::getEMIVer ()
{
  return v;
}


USBLayer2::USBLayer2 (LowLevelDriver * i,
                      L2options *opt) : Layer2 (opt)
{
  emi = nullptr;
  LowLevelDriver *iface = initUSBDriver (i, t);
  if (!iface)
    return;

  switch (iface->getEMIVer ())
    {
    case vEMI1:
      emi = std::shared_ptr<EMI1Layer2> (new EMI1Layer2 (iface, opt));
      break;
    case vEMI2:
      emi = std::shared_ptr<EMI2Layer2>(new EMI2Layer2 (iface, opt));
      break;
    case vCEMI:
      emi = std::shared_ptr<CEMILayer2>(new CEMILayer2 (iface, opt));
      break;
    default:
      TRACEPRINTF (t, 2, this, "Unsupported EMI");
      delete iface;
      return;
    }
}

bool USBLayer2::init (Layer3 *l3)
{
  if (emi == 0)
    return false;
  if (! addGroupAddress(0))
    return false;
  if (! emi->init(l3))
    return false;
  return Layer2::init(l3);
}

bool USBLayer2::enterBusmonitor ()
{
  if (! Layer2::enterBusmonitor ())
    return false;
  return emi->enterBusmonitor ();
}

bool USBLayer2::leaveBusmonitor ()
{
  if (! Layer2::leaveBusmonitor ())
    return false;
  return emi->leaveBusmonitor ();
}

bool USBLayer2::Open ()
{
  if (! Layer2::Open ())
    return false;
  return emi->Open ();
}

bool USBLayer2::Close ()
{
  if (! Layer2::Close ())
    return false;
  return emi->Close ();
}

void
USBLayer2::send_L_Data (L_Data_PDU * l)
{
  emi->send_L_Data (l);
}

