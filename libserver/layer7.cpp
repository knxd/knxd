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

#include "apdu.h"
#include "layer7.h"

Layer7_Broadcast::Layer7_Broadcast (Layer3 * l3, Trace * tr)
{
  t = tr;
  TRACEPRINTF (t, 5, this, "L7Broadcast Open");
  l4 = new T_Broadcast (l3, tr, 0);
  if (!l4->init ())
    {
      delete l4;
      l4 = 0;
    }
}

Layer7_Broadcast::~Layer7_Broadcast ()
{
  TRACEPRINTF (t, 5, this, "L7Broadcast Close");
  if (l4)
    delete l4;
}

bool Layer7_Broadcast::init ()
{
  return l4 != 0;
}

void
Layer7_Broadcast::A_IndividualAddress_Write (eibaddr_t addr)
{
  A_IndividualAddress_Write_PDU a;
  a.addr = addr;
  l4->Send (a.ToPacket ());
}

Array < eibaddr_t >
  Layer7_Broadcast::A_IndividualAddress_Read (unsigned timeout)
{
  Array < eibaddr_t > addrs;
  A_IndividualAddress_Read_PDU r;
  APDU *a;
  l4->Send (r.ToPacket ());
  pth_event_t t = pth_event (PTH_EVENT_RTIME, pth_time (timeout, 0));
  while (pth_event_status (t) != PTH_STATUS_OCCURRED)
    {
      BroadcastComm *c = l4->Get (t);
      if (c)
	{
	  a = APDU::fromPacket (c->data);
	  if (a->isResponse (&r))
	    {
	      addrs.resize (addrs () + 1);
	      addrs[addrs () - 1] = c->src;
	    }
	  delete a;
	  delete c;
	}
    }
  pth_event_free (t, PTH_FREE_THIS);
  return addrs;
}

Layer7_Connection::Layer7_Connection (Layer3 * l3, Trace * tr, eibaddr_t d)
{
  t = tr;
  dest = d;
  l4 = new T_Connection (l3, tr, d);
  if (!l4->init ())
    {
      delete l4;
      l4 = 0;
    }
}

Layer7_Connection::~Layer7_Connection ()
{
  if (l4)
    delete l4;
}

bool Layer7_Connection::init ()
{
  return l4 != 0;
}

void
Layer7_Connection::A_Restart ()
{
  A_Restart_PDU a;
  l4->Send (a.ToPacket ());
}

APDU *
Layer7_Connection::Request_Response (APDU * r)
{
  APDU *a;
  CArray *c;
  l4->Send (r->ToPacket ());
  pth_event_t t = pth_event (PTH_EVENT_RTIME, pth_time (6, 100));
  while (pth_event_status (t) != PTH_STATUS_OCCURRED)
    {
      c = l4->Get (t);
      if (c)
	{
	  if (c->len () == 0)
	    {
	      delete c;
	      pth_event_free (t, PTH_FREE_THIS);
	      return 0;
	    }
	  a = APDU::fromPacket (*c);
	  delete c;
	  if (a->isResponse (r))
	    {
	      pth_event_free (t, PTH_FREE_THIS);
	      return a;
	    }
	  delete a;
	  pth_event_free (t, PTH_FREE_THIS);
	  return 0;
	}
    }
  pth_event_free (t, PTH_FREE_THIS);
  return 0;
}

int
Layer7_Connection::A_Property_Read (uchar obj, uchar propertyid,
				    uint16_t start, uchar count, CArray & erg)
{
  A_PropertyValue_Read_PDU r;
  r.obj = obj;
  r.prop = propertyid;
  r.start = start & 0x0fff;
  r.count = count & 0x0f;
  APDU *a = Request_Response (&r);
  if (!a)
    return -1;
  A_PropertyValue_Response_PDU *a1 = (A_PropertyValue_Response_PDU *) a;
  erg = a1->data;
  delete a;
  return 0;
}

int
Layer7_Connection::A_Property_Write (uchar obj, uchar propertyid,
				     uint16_t start, uchar count,
				     const CArray & data, CArray & result)
{
  A_PropertyValue_Write_PDU r;
  r.obj = obj;
  r.prop = propertyid;
  r.start = start & 0x0fff;
  r.count = count & 0x0f;
  r.data = data;
  APDU *a = Request_Response (&r);
  if (!a)
    return -1;
  A_PropertyValue_Response_PDU *a1 = (A_PropertyValue_Response_PDU *) a;
  result = a1->data;
  delete a;
  return 0;
}

int
Layer7_Connection::A_Property_Desc (uchar obj, uchar & property,
				    uchar property_index, uchar & type,
				    uint16_t & max_nr_elements,
				    uchar & access)
{
  A_PropertyDescription_Read_PDU r;
  r.obj = obj;
  r.prop = property;
  r.property_index = property_index;
  APDU *a = Request_Response (&r);
  if (!a)
    return -1;
  A_PropertyDescription_Response_PDU *a1 =
    (A_PropertyDescription_Response_PDU *) a;
  type = a1->type;
  max_nr_elements = a1->count;
  access = a1->access;
  property = a1->prop;
  delete a;
  return 0;
}

int
Layer7_Connection::A_Device_Descriptor_Read (uint16_t & maskver, uchar type)
{
  A_DeviceDescriptor_Read_PDU r;
  r.type = type & 0x3f;
  APDU *a = Request_Response (&r);
  if (!a)
    return -1;
  A_DeviceDescriptor_Response_PDU *a1 = (A_DeviceDescriptor_Response_PDU *) a;
  maskver = a1->descriptor;
  delete a;
  return 0;
}

int
Layer7_Connection::A_ADC_Read (uchar channel, uchar readcount,
			       int16_t & value)
{
  A_ADC_Read_PDU r;
  r.channel = channel & 0x3f;
  r.count = readcount;
  APDU *a = Request_Response (&r);
  if (!a)
    return -1;
  A_ADC_Response_PDU *a1 = (A_ADC_Response_PDU *) a;
  value = a1->val;
  delete a;
  return 0;
}

int
Layer7_Connection::A_Memory_Read (memaddr_t addr, uchar len, CArray & data)
{
  A_Memory_Read_PDU r;
  r.addr = addr;
  r.count = len & 0x0f;
  APDU *a = Request_Response (&r);
  if (!a)
    return -1;
  A_Memory_Response_PDU *a1 = (A_Memory_Response_PDU *) a;
  data = a1->data;
  delete a;
  return 0;
}

int
Layer7_Connection::A_Memory_Write (memaddr_t addr, const CArray & data)
{
  A_Memory_Write_PDU r;
  r.addr = addr;
  r.count = data () & 0x0f;
  r.data.set (data.array (), data () & 0x0f);
  l4->Send (r.ToPacket ());
  return 0;
}

int
Layer7_Connection::A_Authorize (eibkey_type key, uchar & level)
{
  A_Authorize_Request_PDU r;
  r.key = key;
  APDU *a = Request_Response (&r);
  if (!a)
    return -1;
  A_Authorize_Response_PDU *a1 = (A_Authorize_Response_PDU *) a;
  level = a1->level;
  delete a;
  return 0;
}

int
Layer7_Connection::A_KeyWrite (eibkey_type key, uchar & level)
{
  A_Key_Write_PDU r;
  r.key = key;
  r.level = level;
  APDU *a = Request_Response (&r);
  if (!a)
    return -1;
  A_Key_Response_PDU *a1 = (A_Key_Response_PDU *) a;
  level = a1->level;
  delete a;
  return 0;
}

int
Layer7_Connection::X_Property_Write (uchar obj, uchar propertyid,
				     uint16_t start, uchar count,
				     const CArray & data)
{
  CArray d1;
  if (A_Property_Write (obj, propertyid, start, count, data, d1) == -1)
    return -1;
  if (A_Property_Read (obj, propertyid, start, count, d1) == -1)
    return -1;
  if (d1 != data)
    return -1;
  return 0;
}

int
Layer7_Connection::X_Memory_Write (memaddr_t addr, const CArray & data)
{
  CArray d1;
  if (A_Memory_Write (addr, data) == -1)
    return -1;
  if (A_Memory_Read (addr, data (), d1) == -1)
    return -1;
  if (d1 != data)
    return -2;
  return 0;
}

int
Layer7_Connection::X_Memory_Write_Block (memaddr_t addr, const CArray & data)
{
  CArray prev;
  int i, j, k, res = 0;
  const unsigned blocksize = 12;
  if (X_Memory_Read_Block (addr, data (), prev) == -1)
    return -1;
  for (i = 0; i < data (); i++)
    {
      if (data[i] == prev[i])
	continue;
      j = 0;
      while (data[i + j] != prev[i + j] && j < blocksize && i + j < data ())
	j++;
      k = X_Memory_Write (addr + i, CArray (data.array () + i, j));
      if (k == -1)
	return -1;
      if (k == -2)
	res = -2;
      i += j - 1;
    }

  return res;
}

int
Layer7_Connection::X_Memory_Read_Block (memaddr_t addr, int len, CArray & erg)
{
  unsigned blocksize = 12;
  CArray e;
  erg.resize (len);
  for (unsigned i = 0; i < len; i += blocksize)
    {
    rt:
      if (A_Memory_Read
	  (addr + i, (len - i > blocksize ? blocksize : len - i), e) == -1)
	{
	  if (blocksize == 12)
	    {
	      blocksize = 2;
	      goto rt;
	    }
	  return -1;
	}
      erg.setpart (e, i);
    }
  return 0;
}

int
Layer7_Connection::A_Memory_Write_Block (memaddr_t addr, const CArray & data)
{
  CArray prev;
  int i, j, k, res = 0;
  const unsigned blocksize = 12;

  for (i = 0; i < data (); i += blocksize)
    {
      j = blocksize;
      if (i + j > data ())
	j = data () - i;
      k = A_Memory_Write (addr + i, CArray (data.array () + i, j));
      if (k == -1)
	return -1;
    }

  return res;
}

Layer7_Individual::Layer7_Individual (Layer3 * l3, Trace * tr, eibaddr_t d)
{
  t = tr;
  dest = d;
  l4 = new T_Individual (l3, tr, d, false);
  if (!l4->init ())
    {
      delete l4;
      l4 = 0;
    }
}

Layer7_Individual::~Layer7_Individual ()
{
  if (l4)
    delete l4;
}

bool Layer7_Individual::init ()
{
  return l4 != 0;
}

APDU *
Layer7_Individual::Request_Response (APDU * r)
{
  APDU *a;
  CArray *c;
  l4->Send (r->ToPacket ());
  pth_event_t t = pth_event (PTH_EVENT_RTIME, pth_time (6, 100));
  while (pth_event_status (t) != PTH_STATUS_OCCURRED)
    {
      c = l4->Get (t);
      if (c)
	{
	  if (c->len () == 0)
	    {
	      delete c;
	      pth_event_free (t, PTH_FREE_THIS);
	      return 0;
	    }
	  a = APDU::fromPacket (*c);
	  delete c;
	  if (a->isResponse (r))
	    {
	      pth_event_free (t, PTH_FREE_THIS);
	      return a;
	    }
	  delete a;
	  pth_event_free (t, PTH_FREE_THIS);
	  return 0;
	}
    }
  pth_event_free (t, PTH_FREE_THIS);
  return 0;
}

int
Layer7_Individual::A_Property_Read (uchar obj, uchar propertyid,
				    uint16_t start, uchar count, CArray & erg)
{
  A_PropertyValue_Read_PDU r;
  r.obj = obj;
  r.prop = propertyid;
  r.start = start & 0x0fff;
  r.count = count & 0x0f;
  APDU *a = Request_Response (&r);
  if (!a)
    return -1;
  A_PropertyValue_Response_PDU *a1 = (A_PropertyValue_Response_PDU *) a;
  erg = a1->data;
  delete a;
  return 0;
}

int
Layer7_Individual::A_Property_Write (uchar obj, uchar propertyid,
				     uint16_t start, uchar count,
				     const CArray & data, CArray & result)
{
  A_PropertyValue_Write_PDU r;
  r.obj = obj;
  r.prop = propertyid;
  r.start = start & 0x0fff;
  r.count = count & 0x0f;
  r.data = data;
  APDU *a = Request_Response (&r);
  if (!a)
    return -1;
  A_PropertyValue_Response_PDU *a1 = (A_PropertyValue_Response_PDU *) a;
  result = a1->data;
  delete a;
  return 0;
}
