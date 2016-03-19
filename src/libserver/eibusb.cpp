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

LowLevelDriver *
initUSBDriver (LowLevelDriver * i, Trace * tr)
{
  CArray r1, *r = 0;
  LowLevelDriver *iface;
  uchar emiver;
  int cnt = 0;
  const uchar ask[64] = {
    0x01, 0x13, 0x09, 0x00, 0x08, 0x00, 0x01, 0x0f, 0x01, 0x00, 0x00, 0x01
  };
  uchar init[64] = {
    0x01, 0x13, 0x0a, 0x00, 0x08, 0x00, 0x02, 0x0f, 0x03, 0x00, 0x00, 0x05,
    0x01
  };

  if (!i->init ())
    {
      delete i;
      return 0;
    }

  do
    {
      cnt++;
      i->Send_Packet (CArray (ask, sizeof (ask)));
      do
	{
	  r = i->Get_Packet (0);
	  if (r)
	    {
	      r1 = *r;
	      if (r1 () != 64)
		goto cont;
	      if (r1[0] != 01)
		goto cont;
	      if ((r1[1] & 0x0f) != 0x03)
		goto cont;
	      if (r1[2] != 0x0b)
		goto cont;
	      if (r1[3] != 0x00)
		goto cont;
	      if (r1[4] != 0x08)
		goto cont;
	      if (r1[5] != 0x00)
		goto cont;
	      if (r1[6] != 0x03)
		goto cont;
	      if (r1[7] != 0x0F)
		goto cont;
	      if (r1[8] != 0x02)
		goto cont;
	      if (r1[11] != 0x01)
		goto cont;
	      break;
	    cont:
	      delete r;
	      r = 0;
	    }
	}
      while (!r);
      delete r;
      if (r1[13] & 0x2)
	emiver = 2;
      else if (r1[13] & 0x1)
	emiver = 1;
      else if (r1[13] & 0x4)
	emiver = 3;
      else
	emiver = 0;
      if (emiver == 0 && cnt < 5)
	pth_sleep (5);
    }
  while (emiver == 0 && cnt < 5);

  switch (emiver)
    {
    case 1:
      init[12] = 1;
      i->Send_Packet (CArray (init, sizeof (init)));
      iface =
	new USBConverterInterface (i, tr, LowLevelDriver::vEMI1);
      break;
    case 2:
      init[12] = 2;
      i->Send_Packet (CArray (init, sizeof (init)));
      iface =
	new USBConverterInterface (i, tr, LowLevelDriver::vEMI2);
      break;
    case 3:
      init[12] = 3;
      i->Send_Packet (CArray (init, sizeof (init)));
      iface =
	new USBConverterInterface (i, tr, LowLevelDriver::vCEMI);
      break;
    default:
      TRACEPRINTF (tr, 1, i, "Unsupported EMI %02x %02x", r1[12], r1[13]);
      delete i;
      return 0;
    }
  return iface;
}

USBConverterInterface::USBConverterInterface (LowLevelDriver * iface,
					      Trace * tr, EMIVer ver)
{
  t = tr;
  i = iface;
  v = ver;
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
  l1 = l ();
  out.resize (64);
  if (l1 + 11 > 64)
    l1 = 64 - 11;
  for (j = 0; j < out (); j++)
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

CArray *
USBConverterInterface::Get_Packet (pth_event_t stop)
{
  CArray *res1 = i->Get_Packet (stop);
  if (res1)
    {
      CArray res = *res1;
      if (res () != 64)
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
      res1->set (res.array () + 11, res[6]);
      t->TracePacket (0, this, "RecvEMI", *res1);
    }
  return res1;

out:
  delete res1;
  return 0;
}

void
USBConverterInterface::SendReset ()
{
  return i->SendReset ();
}

LowLevelDriver::EMIVer USBConverterInterface::getEMIVer ()
{
  return v;
}

pth_sem_t *
USBConverterInterface::Send_Queue_Empty_Cond ()
{
  return i->Send_Queue_Empty_Cond ();
}

bool
USBConverterInterface::Send_Queue_Empty ()
{
  return i->Send_Queue_Empty ();
}


USBLayer2::USBLayer2 (LowLevelDriver * i, Layer3 * l3,
                      L2options *opt) : Layer2 (l3, opt)
{
  emi = nullptr;
  LowLevelDriver *iface = initUSBDriver (i, t);
  if (!iface)
    return;

  switch (iface->getEMIVer ())
    {
    case LowLevelDriver::vEMI1:
      emi = std::shared_ptr<EMI1Layer2> (new EMI1Layer2 (iface, l3, opt));
      break;
    case LowLevelDriver::vEMI2:
      emi = std::shared_ptr<EMI2Layer2>(new EMI2Layer2 (iface, l3, opt));
      break;
    case LowLevelDriver::vCEMI:
      emi = std::shared_ptr<CEMILayer2>(new CEMILayer2 (iface, l3, opt));
      break;
    default:
      TRACEPRINTF (t, 2, this, "Unsupported EMI");
      delete iface;
      return;
    }
}

bool USBLayer2::init ()
{
  if (emi == 0)
    return false;
  if (! addGroupAddress(0))
    return false;
  return Layer2::init();
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

bool USBLayer2::Send_Queue_Empty ()
{
  return emi->Send_Queue_Empty ();
}


void
USBLayer2::Send_L_Data (LPDU * l)
{
  emi->Send_L_Data (l);
}

