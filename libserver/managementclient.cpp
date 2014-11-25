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

#include "managementclient.h"
#include "management.h"
#include "loadimage.h"

void
ReadIndividualAddresses (Layer3 * l3, Trace * t, ClientConnection * c,
			 pth_event_t stop)
{
  Layer7_Broadcast b (l3, t);
  if (!b.init ())
    {
      c->sendreject (stop, EIB_PROCESSING_ERROR);
      return;
    }
  CArray erg;
  Array < eibaddr_t > e = b.A_IndividualAddress_Read ();
  erg.resize (2 + 2 * e ());
  EIBSETTYPE (erg, EIB_M_INDIVIDUAL_ADDRESS_READ);
  for (unsigned i = 0; i < e (); i++)
    {
      erg[2 + i * 2] = (e[i] >> 8) & 0xff;
      erg[2 + i * 2 + 1] = (e[i]) & 0xff;
    }
  c->sendmessage (erg (), erg.array (), stop);
}

void
ChangeProgMode (Layer3 * l3, Trace * t, ClientConnection * c,
		pth_event_t stop)
{
  eibaddr_t dest;
  uchar res[3];
  int i;
  EIBSETTYPE (res, EIB_PROG_MODE);
  res[2] = 0;
  if (c->size < 5)
    {
      c->sendreject (stop);
      return;
    }
  dest = (c->buf[2] << 8) | (c->buf[3]);
  Management_Connection m (l3, t, dest);
  if (!m.init ())
    {
      c->sendreject (stop, EIB_PROCESSING_ERROR);
      return;
    }
  switch (c->buf[4])
    {
    case 0:
      if (m.X_Progmode_Off () == -1)
	c->sendreject (stop);
      else
	c->sendmessage (3, res, stop);
      break;
    case 1:
      if (m.X_Progmode_On () == -1)
	c->sendreject (stop);
      else
	c->sendmessage (3, res, stop);
      break;
    case 2:
      if (m.X_Progmode_Toggle () == -1)
	c->sendreject (stop);
      else
	c->sendmessage (3, res, stop);
      break;
    case 3:
      if ((i = m.X_Progmode_Status ()) == -1)
	c->sendreject (stop);
      else
	{
	  res[2] = i;
	  c->sendmessage (3, res, stop);
	}
      break;
    default:
      c->sendreject (stop);
    }
}

void
GetMaskVersion (Layer3 * l3, Trace * t, ClientConnection * c,
		pth_event_t stop)
{
  eibaddr_t dest;
  uchar res[4];
  uint16_t maskver;
  EIBSETTYPE (res, EIB_MASK_VERSION);
  res[2] = 0;
  if (c->size < 4)
    {
      c->sendreject (stop);
      return;
    }

  dest = (c->buf[2] << 8) | (c->buf[3]);
  Management_Connection m (l3, t, dest);
  if (!m.init ())
    {
      c->sendreject (stop, EIB_PROCESSING_ERROR);
      return;
    }
  if (m.A_Device_Descriptor_Read (maskver) == -1)
    c->sendreject (stop);
  else
    {
      res[2] = (maskver >> 8) & 0xff;
      res[3] = (maskver) & 0xff;
      c->sendmessage (4, res, stop);
    }
}

void
WriteIndividualAddress (Layer3 * l3, Trace * t, ClientConnection * c,
			pth_event_t stop)
{
  eibaddr_t dest;
  uint16_t maskver;
  if (c->size < 4)
    {
      c->sendreject (stop);
      return;
    }

  dest = (c->buf[2] << 8) | (c->buf[3]);
  Layer7_Broadcast b (l3, t);
  if (!b.init ())
    {
      c->sendreject (stop, EIB_PROCESSING_ERROR);
      return;
    }
  {
    Management_Connection m (l3, t, dest);
    if (!m.init ())
      {
	c->sendreject (stop, EIB_PROCESSING_ERROR);
	return;
      }
    if (m.A_Device_Descriptor_Read (maskver) != -1)
      {
	c->sendreject (stop, EIB_ERROR_ADDR_EXISTS);
	return;
      }
  }
  Array < eibaddr_t > addr = b.A_IndividualAddress_Read ();
  if (addr () > 1)
    {
      c->sendreject (stop, EIB_ERROR_MORE_DEVICE);
      return;
    }
  if (addr () == 0)
    {
      c->sendreject (stop, EIB_ERROR_TIMEOUT);
      return;
    }
  b.A_IndividualAddress_Write (dest);
  // wait 100ms
  pth_usleep (100000);

  Management_Connection m1 (l3, t, dest);
  if (!m1.init ())
    {
      c->sendreject (stop, EIB_PROCESSING_ERROR);
      return;
    }
  if (m1.A_Device_Descriptor_Read (maskver) == -1)
    {
      c->sendreject (stop, EIB_PROCESSING_ERROR);
      return;
    }
  if (m1.X_Progmode_Off () == -1)
    {
      c->sendreject (stop, EIB_PROCESSING_ERROR);
      return;
    }
  c->sendreject (stop, EIB_M_INDIVIDUAL_ADDRESS_WRITE);
}

void
ManagementConnection (Layer3 * l3, Trace * t, ClientConnection * c,
		      pth_event_t stop)
{
  eibaddr_t dest;
  uint16_t maskver;
  int16_t val;
  uchar buf[10];
  int i;
  eibkey_type key;

  if (c->size < 4)
    {
      c->sendreject (stop);
      return;
    }

  dest = (c->buf[2] << 8) | (c->buf[3]);
  Management_Connection m (l3, t, dest);
  if (!m.init ())
    {
      c->sendreject (stop, EIB_PROCESSING_ERROR);
      return;
    }
  if (m.A_Device_Descriptor_Read (maskver) == -1)
    {
      c->sendreject (stop);
      return;
    }
  c->sendreject (stop, EIB_MC_CONNECTION);
  do
    {
      i = c->readmessage (stop);
      if (i != -1)
	switch (EIBTYPE (c->buf))
	  {
	  case EIB_MC_PROG_MODE:
	    if (c->size < 3)
	      {
		c->sendreject (stop);
		break;
	      }
	    EIBSETTYPE (buf, EIB_MC_PROG_MODE);
	    buf[2] = 0;
	    switch (c->buf[2])
	      {
	      case 0:
		if (m.X_Progmode_Off () == -1)
		  c->sendreject (stop);
		else
		  c->sendmessage (3, buf, stop);
		break;
	      case 1:
		if (m.X_Progmode_On () == -1)
		  c->sendreject (stop);
		else
		  c->sendmessage (3, buf, stop);
		break;
	      case 2:
		if (m.X_Progmode_Toggle () == -1)
		  c->sendreject (stop);
		else
		  c->sendmessage (3, buf, stop);
		break;
	      case 3:
		if ((i = m.X_Progmode_Status ()) == -1)
		  c->sendreject (stop);
		else
		  {
		    buf[2] = i;
		    c->sendmessage (3, buf, stop);
		  }
		break;
	      default:
		c->sendreject (stop);
	      }
	    break;
	  case EIB_MC_MASK_VERSION:
	    if (m.A_Device_Descriptor_Read (maskver) == -1)
	      c->sendreject (stop);
	    else
	      {
		EIBSETTYPE (buf, EIB_MC_MASK_VERSION);
		buf[2] = (maskver >> 8) & 0xff;
		buf[3] = (maskver) & 0xff;
		c->sendmessage (4, buf, stop);
	      }
	    break;
	  case EIB_MC_PEI_TYPE:
	    if (m.X_Get_PEIType (val) == -1)
	      c->sendreject (stop);
	    else
	      {
		EIBSETTYPE (buf, EIB_MC_PEI_TYPE);
		buf[2] = (val >> 8) & 0xff;
		buf[3] = (val) & 0xff;
		c->sendmessage (4, buf, stop);
	      }
	    break;
	  case EIB_MC_ADC_READ:
	    if (c->size < 4)
	      {
		c->sendreject (stop);
		break;
	      }
	    if (m.A_ADC_Read (c->buf[2], c->buf[3], val) == -1)
	      c->sendreject (stop);
	    else
	      {
		EIBSETTYPE (buf, EIB_MC_ADC_READ);
		buf[2] = (val >> 8) & 0xff;
		buf[3] = (val) & 0xff;
		c->sendmessage (4, buf, stop);
	      }
	    break;

	  case EIB_MC_READ:
	    if (c->size < 6)
	      {
		c->sendreject (stop);
		break;
	      }
	    {
	      memaddr_t addr = (c->buf[2] << 8) | (c->buf[3]);
	      unsigned len = (c->buf[4] << 8) | (c->buf[5]);
	      CArray data, erg;
	      if (m.X_Memory_Read_Block (addr, len, data) == -1)
		c->sendreject (stop);
	      else
		{
		  erg.resize (6);
		  EIBSETTYPE (erg, EIB_MC_READ);
		  erg.setpart (data, 2);
		  c->sendmessage (erg (), erg.array (), stop);
		}
	    }
	    break;

	  case EIB_MC_WRITE:
	    if (c->size < 6)
	      {
		c->sendreject (stop);
		break;
	      }
	    {
	      memaddr_t addr = (c->buf[2] << 8) | (c->buf[3]);
	      unsigned len = (c->buf[4] << 8) | (c->buf[5]);
	      if (c->size < len + 6)
		{
		  c->sendreject (stop);
		  break;
		}
	      i = m.X_Memory_Write_Block (addr, CArray (c->buf + 6, len));
	      if (i == -2)
		c->sendreject (stop, EIB_ERROR_VERIFY);
	      else if (i != 0)
		c->sendreject (stop, EIB_PROCESSING_ERROR);
	      else
		c->sendreject (stop, EIB_MC_WRITE);
	    }
	    break;

	  case EIB_MC_PROP_READ:
	    if (c->size < 7)
	      {
		c->sendreject (stop);
		break;
	      }
	    {
	      CArray data, erg;
	      if (m.A_Property_Read (c->buf[2], c->buf[3],
				     (c->buf[4] << 8) | c->buf[5], c->buf[6],
				     data) == -1)
		c->sendreject (stop);
	      else
		{
		  erg.resize (2);
		  EIBSETTYPE (erg, EIB_MC_PROP_READ);
		  erg.setpart (data, 2);
		  c->sendmessage (erg (), erg.array (), stop);
		}
	    }
	    break;

	  case EIB_MC_PROP_WRITE:
	    if (c->size < 7)
	      {
		c->sendreject (stop);
		break;
	      }
	    {
	      CArray data, erg;
	      if (m.A_Property_Write (c->buf[2], c->buf[3],
				      (c->buf[4] << 8) | c->buf[5], c->buf[6],
				      CArray (c->buf + 7, c->size - 7),
				      erg) == -1)
		c->sendreject (stop);
	      else
		{
		  erg.resize (2);
		  EIBSETTYPE (erg, EIB_MC_PROP_WRITE);
		  erg.setpart (data, 2);
		  c->sendmessage (erg (), erg.array (), stop);
		}
	    }
	    break;

	  case EIB_MC_AUTHORIZE:
	    if (c->size < 6)
	      {
		c->sendreject (stop);
		break;
	      }
	    EIBSETTYPE (buf, EIB_MC_AUTHORIZE);
	    key =
	      (c->buf[2] << 24) | (c->buf[3] << 16) | (c->buf[4] << 8) |
	      (c->buf[5]);
	    if (m.A_Authorize (key, buf[2]) == -1)
	      c->sendreject (stop);
	    else
	      c->sendmessage (3, buf, stop);
	    break;

	  case EIB_MC_KEY_WRITE:
	    if (c->size < 7)
	      {
		c->sendreject (stop);
		break;
	      }
	    key =
	      (c->buf[2] << 24) | (c->buf[3] << 16) | (c->buf[4] << 8) |
	      (c->buf[5]);
	    if (m.A_KeyWrite (key, *(c->buf + 6)) == -1)
	      c->sendreject (stop);
	    else
	      c->sendreject (stop, EIB_MC_KEY_WRITE);
	    break;

	  case EIB_MC_PROP_DESC:
	    if (c->size < 4)
	      {
		c->sendreject (stop);
		break;
	      }
	    if (m.A_Property_Desc (c->buf[2], c->buf[3], 0, buf[2], maskver,
				   buf[5]) == -1)
	      c->sendreject (stop);
	    else
	      {
		EIBSETTYPE (buf, EIB_MC_PROP_DESC);
		buf[3] = (maskver >> 8) & 0xff;
		buf[4] = (maskver) & 0xff;
		c->sendmessage (6, buf, stop);
	      }
	    break;

	  case EIB_MC_PROP_SCAN:
	    {
	      Array < PropertyInfo > p;
	      if (m.X_PropertyScan (p) == -1)
		c->sendreject (stop);
	      else
		{
		  CArray erg;
		  erg.resize (2 + p () * 6);
		  EIBSETTYPE (erg, EIB_MC_PROP_SCAN);
		  for (unsigned i = 0; i < p (); i++)
		    {
		      erg[i * 6 + 2] = p[i].obj;
		      erg[i * 6 + 3] = p[i].property;
		      erg[i * 6 + 4] = p[i].type;
		      erg[i * 6 + 5] = (p[i].count >> 8) & 0xff;
		      erg[i * 6 + 6] = (p[i].count) & 0xff;
		      erg[i * 6 + 7] = p[i].access;
		    }
		  c->sendmessage (erg (), erg.array (), stop);
		}
	    }
	    break;

	  case EIB_RESET_CONNECTION:
	    i = -1;
	    break;

	  case EIB_MC_RESTART:
	    m.A_Restart ();
	    c->sendreject (stop, EIB_MC_RESTART);
	    break;

	  case EIB_MC_WRITE_NOVERIFY:
	    if (c->size < 6)
	      {
		c->sendreject (stop);
		break;
	      }
	    {
	      memaddr_t addr = (c->buf[2] << 8) | (c->buf[3]);
	      unsigned len = (c->buf[4] << 8) | (c->buf[5]);
	      if (c->size < len + 6)
		{
		  c->sendreject (stop);
		  break;
		}
	      i = m.A_Memory_Write_Block (addr, CArray (c->buf + 6, len));
	      if (i != 0)
		c->sendreject (stop, EIB_PROCESSING_ERROR);
	      else
		c->sendreject (stop, EIB_MC_WRITE_NOVERIFY);
	    }
	    break;

	  default:
	    c->sendreject (stop);
	  }
    }
  while (i != -1);
}

void
ManagementIndividual (Layer3 * l3, Trace * t, ClientConnection * c,
		      pth_event_t stop)
{
  eibaddr_t dest;
  uint16_t maskver;
  int16_t val;
  uchar buf[10];
  int i;
  eibkey_type key;

  if (c->size < 4)
    {
      c->sendreject (stop);
      return;
    }

  dest = (c->buf[2] << 8) | (c->buf[3]);
  Management_Individual m (l3, t, dest);
  if (!m.init ())
    {
      c->sendreject (stop, EIB_PROCESSING_ERROR);
      return;
    }
  c->sendreject (stop, EIB_MC_INDIVIDUAL);
  do
    {
      i = c->readmessage (stop);
      if (i != -1)
	switch (EIBTYPE (c->buf))
	  {
	  case EIB_MC_PROP_READ:
	    if (c->size < 7)
	      {
		c->sendreject (stop);
		break;
	      }
	    {
	      CArray data, erg;
	      if (m.A_Property_Read (c->buf[2], c->buf[3],
				     (c->buf[4] << 8) | c->buf[5], c->buf[6],
				     data) == -1)
		c->sendreject (stop);
	      else
		{
		  erg.resize (2);
		  EIBSETTYPE (erg, EIB_MC_PROP_READ);
		  erg.setpart (data, 2);
		  c->sendmessage (erg (), erg.array (), stop);
		}
	    }
	    break;

	  case EIB_MC_PROP_WRITE:
	    if (c->size < 7)
	      {
		c->sendreject (stop);
		break;
	      }
	    {
	      CArray data, erg;
	      if (m.A_Property_Write (c->buf[2], c->buf[3],
				      (c->buf[4] << 8) | c->buf[5], c->buf[6],
				      CArray (c->buf + 7, c->size - 7),
				      erg) == -1)
		c->sendreject (stop);
	      else
		{
		  erg.resize (2);
		  EIBSETTYPE (erg, EIB_MC_PROP_WRITE);
		  erg.setpart (data, 2);
		  c->sendmessage (erg (), erg.array (), stop);
		}
	    }
	    break;

	  case EIB_RESET_CONNECTION:
	    i = -1;
	    break;

	  default:
	    c->sendreject (stop);
	  }
    }
  while (i != -1);
}

void
LoadImage (Layer3 * l3, Trace * t, ClientConnection * c, pth_event_t stop)
{
  uchar buf[200];
  CArray img (c->buf + 2, c->size - 2);
  CArray erg;
  int j;
  BCUImage *i;
  BCU_LOAD_RESULT r = PrepareLoadImage (img, i);
  if (r != IMG_IMAGE_LOADABLE)
    {
      if (i)
	delete i;
      EIBSETTYPE (buf, EIB_LOAD_IMAGE);
      buf[2] = (r >> 8) & 0xff;
      buf[3] = (r) & 0xff;
      c->sendmessage (4, buf, stop);
      return;
    }
  {
    uint16_t maskver;
    uchar c;
    r = IMG_NO_DEVICE_CONNECTION;
    Management_Connection m (l3, t, i->addr);
    if (!m.init ())
      goto out;
    r = IMG_MASK_READ_FAILED;
    if (m.A_Device_Descriptor_Read (maskver) == -1)
      goto out;
    r = IMG_WRONG_MASK_VERSION;
    if (i->BCUType == BCUImage::B_bcu1)
      {
	if (maskver != 0x0012)
	  goto out;

	/* set error flags in BCU (0x10D = 0x00) */
	r = IMG_CLEAR_ERROR;
	c = 0;
	if (m.X_Memory_Write_Block (0x010d, CArray (&c, 1)) != 0)
	  goto out;

	/*set length of the address tab to 1 */
	r = IMG_RESET_ADDR_TAB;
	c = 0x01;
	if (m.X_Memory_Write_Block (0x0116, CArray (&c, 1)) != 0)
	  goto out;

	/*load the data from 0x100 to 0x100 */
	r = IMG_LOAD_HEADER;
	c = 0xff;
	if (m.X_Memory_Write_Block (0x0100, CArray (&c, 1)) != 0)
	  goto out;

	/*load the data from 0x103 to 0x10C */
	if (m.X_Memory_Write_Block (0x0103,
				    CArray (i->code.array () + 0x03,
					    10)) != 0)
	  goto out;

	/*load the data from 0x10E to 0x115 */
	if (m.X_Memory_Write_Block (0x010E,
				    CArray (i->code.array () + 0x0E, 8)) != 0)
	  goto out;

	/*load the data from 0x119H to eeprom end */
	r = IMG_LOAD_MAIN;
	if (m.X_Memory_Write_Block (0x119,
				    CArray (i->code.array () + 0x19,
					    i->code () - 0x19)) != 0)
	  goto out;

	if (m.X_Memory_Write_Block (0x0100, CArray (i->code.array (), 1)) !=
	    0)
	  goto out;

	/*erase the user RAM (0x0CE to 0x0DF) */
	r = IMG_ZERO_RAM;
	uchar zero[18] = { 0 };
	if (m.X_Memory_Write_Block (0x00ce, CArray (zero, 18)) != 0)
	  goto out;

	/* set the length of the address table */
	r = IMG_FINALIZE_ADDR_TAB;
	if (m.X_Memory_Write_Block (0x0116,
				    CArray (i->code.array () + 0x16, 1)) != 0)
	  goto out;

	/* reset all error flags in the BCU (0x10D = 0xFF) */
	r = IMG_PREPARE_RUN;
	c = 0xff;
	if (m.X_Memory_Write_Block (0x010d, CArray (&c, 1)) != 0)
	  goto out;

	r = IMG_RESTART;
	m.A_Restart ();

	r = IMG_LOADED;
	goto out;
      }
    if (i->BCUType == BCUImage::B_bcu20 || i->BCUType == BCUImage::B_bcu21)
      {
	if (maskver != 0x0020 && i->BCUType == BCUImage::B_bcu20)
	  goto out;

	if (maskver != 0x0021 && i->BCUType == BCUImage::B_bcu21)
	  goto out;

	uchar level;
	r = IMG_AUTHORIZATION_FAILED;
	if (m.A_Authorize (i->installkey, level) == -1)
	  goto out;

	if (level)
	  goto out;

	r = IMG_KEY_WRITE;
	for (j = 0; j < 3; j++)
	  {
	    level = j;
	    if (m.A_KeyWrite (i->keys[level], level) == -1)
	      goto out;
	    if (j != level)
	      goto out;
	  }

	for (j = 0; j < i->load (); j++)
	  {
	    r = i->load[j].error;
	    if (i->load[j].obj != 0xff)
	      {
		if (m.A_Property_Write (i->load[j].obj, i->load[j].prop,
					i->load[j].start, 1, i->load[j].req,
					erg) == -1)
		  goto out;
		if (erg != i->load[j].result)
		  goto out;
	      }
	    if (i->load[j].memaddr != 0xffff)
	      if (m.X_Memory_Write_Block (i->load[j].memaddr,
					  CArray (i->code.array () +
						  i->load[j].memaddr - 0x100,
						  i->load[j].len)) != 0)
		goto out;
	  }

	r = IMG_RESTART;
	m.A_Restart ();

	r = IMG_LOADED;
	goto out;
      }
  }
out:

  if (i)
    delete i;
  EIBSETTYPE (buf, EIB_LOAD_IMAGE);
  buf[2] = (r >> 8) & 0xff;
  buf[3] = (r) & 0xff;
  c->sendmessage (4, buf, stop);
}
