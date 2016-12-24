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
ReadIndividualAddresses (ClientConnPtr c, uint8_t *buf, size_t len)
{
  Layer7_Broadcast b (c->t);
  if (!b.init (c->l3))
    {
      c->sendreject (EIB_PROCESSING_ERROR);
      return;
    }
  CArray erg;
  Array < eibaddr_t > e = b.A_IndividualAddress_Read (c->t);
  erg.resize (2 + 2 * e.size());
  EIBSETTYPE (erg, EIB_M_INDIVIDUAL_ADDRESS_READ);
  for (unsigned i = 0; i < e.size(); i++)
    {
      erg[2 + i * 2] = (e[i] >> 8) & 0xff;
      erg[2 + i * 2 + 1] = (e[i]) & 0xff;
    }
  c->sendmessage (erg.size(), erg.data());
}

void
ChangeProgMode (ClientConnPtr c, uint8_t *buf, size_t len)
{
  eibaddr_t dest;
  uchar res[3];
  int i;
  EIBSETTYPE (res, EIB_PROG_MODE);
  res[2] = 0;
  if (c->size < 5)
    {
      c->sendreject ();
      return;
    }
  dest = (c->buf[2] << 8) | (c->buf[3]);
  Management_Connection m = Management_Connection (c->t, dest);
  if (!m.init (c->l3))
    {
      c->sendreject (EIB_PROCESSING_ERROR);
      return;
    }
  switch (c->buf[4])
    {
    case 0:
      if (m.X_Progmode_Off () == -1)
	c->sendreject ();
      else
	c->sendmessage (3, res);
      break;
    case 1:
      if (m.X_Progmode_On () == -1)
	c->sendreject ();
      else
	c->sendmessage (3, res);
      break;
    case 2:
      if (m.X_Progmode_Toggle () == -1)
	c->sendreject ();
      else
	c->sendmessage (3, res);
      break;
    case 3:
      if ((i = m.X_Progmode_Status ()) == -1)
	c->sendreject ();
      else
	{
	  res[2] = i;
	  c->sendmessage (3, res);
	}
      break;
    default:
      c->sendreject ();
    }
}

void
GetMaskVersion (ClientConnPtr c, uint8_t *buf, size_t len)
{
  eibaddr_t dest;
  uchar res[4];
  uint16_t maskver;
  EIBSETTYPE (res, EIB_MASK_VERSION);
  res[2] = 0;
  if (c->size < 4)
    {
      c->sendreject ();
      return;
    }

  dest = (c->buf[2] << 8) | (c->buf[3]);
  Management_Connection m = Management_Connection (c->t, dest);
  if (!m.init (c->l3))
    {
      c->sendreject (EIB_PROCESSING_ERROR);
      return;
    }
  if (m.A_Device_Descriptor_Read (maskver) == -1)
    c->sendreject ();
  else
    {
      res[2] = (maskver >> 8) & 0xff;
      res[3] = (maskver) & 0xff;
      c->sendmessage (4, res);
    }
}

void
WriteIndividualAddress (ClientConnPtr c, uint8_t *buf, size_t len)
{
  eibaddr_t dest;
  uint16_t maskver;
  if (c->size < 4)
    {
      c->sendreject ();
      return;
    }

  dest = (c->buf[2] << 8) | (c->buf[3]);
  Layer7_Broadcast b (c->t);
  if (!b.init (c->l3))
    {
      c->sendreject (EIB_PROCESSING_ERROR);
      return;
    }
  {
    Management_Connection m = Management_Connection (c->t, dest);
    if (!m.init (c->l3))
      {
	c->sendreject (EIB_PROCESSING_ERROR);
	return;
      }
    if (m.A_Device_Descriptor_Read (maskver) != -1)
      {
	c->sendreject (EIB_ERROR_ADDR_EXISTS);
	return;
      }
  }
  Array < eibaddr_t > addr = b.A_IndividualAddress_Read (c->t);
  if (addr.size() > 1)
    {
      c->sendreject (EIB_ERROR_MORE_DEVICE);
      return;
    }
  if (addr.size() == 0)
    {
      c->sendreject (EIB_ERROR_TIMEOUT);
      return;
    }
  b.A_IndividualAddress_Write (dest);
  // wait 100ms
  pth_usleep (100000);

  Management_Connection m1 = Management_Connection (c->t, dest);
  if (!m1.init (c->l3))
    {
      c->sendreject (EIB_PROCESSING_ERROR);
      return;
    }
  if (m1.A_Device_Descriptor_Read (maskver) == -1)
    {
      c->sendreject (EIB_PROCESSING_ERROR);
      return;
    }
  if (m1.X_Progmode_Off () == -1)
    {
      c->sendreject (EIB_PROCESSING_ERROR);
      return;
    }
  c->sendreject (EIB_M_INDIVIDUAL_ADDRESS_WRITE);
}

ManagementConnection::ManagementConnection (ClientConnPtr c, uint8_t *buf, size_t len)
{
  eibaddr_t dest;
  uint16_t maskver;
  int16_t val;
  uchar buf[10];
  int i;
  eibkey_type key;

  if (c->size < 4)
    {
      c->sendreject ();
      return;
    }

  dest = (c->buf[2] << 8) | (c->buf[3]);
  Management_Connection m = Management_Connection (c->t, dest);
  if (!m.init (c->l3))
    {
      c->sendreject (EIB_PROCESSING_ERROR);
      return;
    }
  if (m.A_Device_Descriptor_Read (maskver) == -1)
    {
      c->sendreject ();
      return;
    }
  c->sendreject (EIB_MC_CONNECTION);
  do
    {
      i = c->readmessage ();
      if (i != -1)
	switch (EIBTYPE (c->buf))
	  {
	  case EIB_MC_PROG_MODE:
	    if (c->size < 3)
	      {
		c->sendreject ();
		break;
	      }
	    EIBSETTYPE (buf, EIB_MC_PROG_MODE);
	    buf[2] = 0;
	    switch (c->buf[2])
	      {
	      case 0:
		if (m.X_Progmode_Off () == -1)
		  c->sendreject ();
		else
		  c->sendmessage (3, buf);
		break;
	      case 1:
		if (m.X_Progmode_On () == -1)
		  c->sendreject ();
		else
		  c->sendmessage (3, buf);
		break;
	      case 2:
		if (m.X_Progmode_Toggle () == -1)
		  c->sendreject ();
		else
		  c->sendmessage (3, buf);
		break;
	      case 3:
		if ((i = m.X_Progmode_Status ()) == -1)
		  c->sendreject ();
		else
		  {
		    buf[2] = i;
		    c->sendmessage (3, buf);
		  }
		break;
	      default:
		c->sendreject ();
	      }
	    break;
	  case EIB_MC_MASK_VERSION:
	    if (m.A_Device_Descriptor_Read (maskver) == -1)
	      c->sendreject ();
	    else
	      {
		EIBSETTYPE (buf, EIB_MC_MASK_VERSION);
		buf[2] = (maskver >> 8) & 0xff;
		buf[3] = (maskver) & 0xff;
		c->sendmessage (4, buf);
	      }
	    break;
	  case EIB_MC_PEI_TYPE:
	    if (m.X_Get_PEIType (val) == -1)
	      c->sendreject ();
	    else
	      {
		EIBSETTYPE (buf, EIB_MC_PEI_TYPE);
		buf[2] = (val >> 8) & 0xff;
		buf[3] = (val) & 0xff;
		c->sendmessage (4, buf);
	      }
	    break;
	  case EIB_MC_ADC_READ:
	    if (c->size < 4)
	      {
		c->sendreject ();
		break;
	      }
	    if (m.A_ADC_Read (c->buf[2], c->buf[3], val) == -1)
	      c->sendreject ();
	    else
	      {
		EIBSETTYPE (buf, EIB_MC_ADC_READ);
		buf[2] = (val >> 8) & 0xff;
		buf[3] = (val) & 0xff;
		c->sendmessage (4, buf);
	      }
	    break;

	  case EIB_MC_READ:
	    if (c->size < 6)
	      {
		c->sendreject ();
		break;
	      }
	    {
	      memaddr_t addr = (c->buf[2] << 8) | (c->buf[3]);
	      unsigned len = (c->buf[4] << 8) | (c->buf[5]);
	      CArray data, erg;
	      if (m.X_Memory_Read_Block (addr, len, data) == -1)
		c->sendreject ();
	      else
		{
		  erg.resize (6);
		  EIBSETTYPE (erg, EIB_MC_READ);
		  erg.setpart (data, 2);
		  c->sendmessage (erg.size(), erg.data());
		}
	    }
	    break;

	  case EIB_MC_WRITE:
	    if (c->size < 6)
	      {
		c->sendreject ();
		break;
	      }
	    {
	      memaddr_t addr = (c->buf[2] << 8) | (c->buf[3]);
	      unsigned len = (c->buf[4] << 8) | (c->buf[5]);
	      if (c->size < len + 6)
		{
		  c->sendreject ();
		  break;
		}
	      i = m.X_Memory_Write_Block (addr, CArray (c->buf + 6, len));
	      if (i == -2)
		c->sendreject (EIB_ERROR_VERIFY);
	      else if (i != 0)
		c->sendreject (EIB_PROCESSING_ERROR);
	      else
		c->sendreject (EIB_MC_WRITE);
	    }
	    break;

	  case EIB_MC_PROP_READ:
	    if (c->size < 7)
	      {
		c->sendreject ();
		break;
	      }
	    {
	      CArray data, erg;
	      if (m.A_Property_Read (c->buf[2], c->buf[3],
				     (c->buf[4] << 8) | c->buf[5], c->buf[6],
				     data) == -1)
		c->sendreject ();
	      else
		{
		  erg.resize (2);
		  EIBSETTYPE (erg, EIB_MC_PROP_READ);
		  erg.setpart (data, 2);
		  c->sendmessage (erg.size(), erg.data());
		}
	    }
	    break;

	  case EIB_MC_PROP_WRITE:
	    if (c->size < 7)
	      {
		c->sendreject ();
		break;
	      }
	    {
	      CArray data, erg;
	      if (m.A_Property_Write (c->buf[2], c->buf[3],
				      (c->buf[4] << 8) | c->buf[5], c->buf[6],
				      CArray (c->buf + 7, c->size - 7),
				      erg) == -1)
		c->sendreject ();
	      else
		{
		  erg.resize (2);
		  EIBSETTYPE (erg, EIB_MC_PROP_WRITE);
		  erg.setpart (data, 2);
		  c->sendmessage (erg.size(), erg.data());
		}
	    }
	    break;

	  case EIB_MC_AUTHORIZE:
	    if (c->size < 6)
	      {
		c->sendreject ();
		break;
	      }
	    EIBSETTYPE (buf, EIB_MC_AUTHORIZE);
	    key =
	      (c->buf[2] << 24) | (c->buf[3] << 16) | (c->buf[4] << 8) |
	      (c->buf[5]);
	    if (m.A_Authorize (key, buf[2]) == -1)
	      c->sendreject ();
	    else
	      c->sendmessage (3, buf);
	    break;

	  case EIB_MC_KEY_WRITE:
	    if (c->size < 7)
	      {
		c->sendreject ();
		break;
	      }
	    key =
	      (c->buf[2] << 24) | (c->buf[3] << 16) | (c->buf[4] << 8) |
	      (c->buf[5]);
	    if (m.A_KeyWrite (key, *(c->buf + 6)) == -1)
	      c->sendreject ();
	    else
	      c->sendreject (EIB_MC_KEY_WRITE);
	    break;

	  case EIB_MC_PROP_DESC:
	    if (c->size < 4)
	      {
		c->sendreject ();
		break;
	      }
	    if (m.A_Property_Desc (c->buf[2], c->buf[3], 0, buf[2], maskver,
				   buf[5]) == -1)
	      c->sendreject ();
	    else
	      {
		EIBSETTYPE (buf, EIB_MC_PROP_DESC);
		buf[3] = (maskver >> 8) & 0xff;
		buf[4] = (maskver) & 0xff;
		c->sendmessage (6, buf);
	      }
	    break;

	  case EIB_MC_PROP_SCAN:
	    {
	      Array < PropertyInfo > p;
	      if (m.X_PropertyScan (p) == -1)
		c->sendreject ();
	      else
		{
		  CArray erg;
		  erg.resize (2 + p.size() * 6);
		  EIBSETTYPE (erg, EIB_MC_PROP_SCAN);
		  unsigned int ii = 0;
		  ITER( i,p)
		    {
		      erg[ii + 2] = i->obj;
		      erg[ii + 3] = i->property;
		      erg[ii + 4] = i->type;
		      erg[ii + 5] = (i->count >> 8) & 0xff;
		      erg[ii + 6] = (i->count) & 0xff;
		      erg[ii + 7] = i->access;
			  ii += 6;
		    }
		  c->sendmessage (erg.size(), erg.data());
		}
	    }
	    break;

	  case EIB_RESET_CONNECTION:
	    i = -1;
	    break;

	  case EIB_MC_RESTART:
	    m.A_Restart ();
	    c->sendreject (EIB_MC_RESTART);
	    break;

	  case EIB_MC_WRITE_NOVERIFY:
	    if (c->size < 6)
	      {
		c->sendreject ();
		break;
	      }
	    {
	      memaddr_t addr = (c->buf[2] << 8) | (c->buf[3]);
	      unsigned len = (c->buf[4] << 8) | (c->buf[5]);
	      if (c->size < len + 6)
		{
		  c->sendreject ();
		  break;
		}
	      i = m.A_Memory_Write_Block (addr, CArray (c->buf + 6, len));
	      if (i != 0)
		c->sendreject (EIB_PROCESSING_ERROR);
	      else
		c->sendreject (EIB_MC_WRITE_NOVERIFY);
	    }
	    break;

	  default:
	    c->sendreject ();
	  }
    }
  while (i != -1);
}

void
ManagementIndividual (ClientConnPtr c, uint8_t *buf, size_t len)
{
  eibaddr_t dest;
  int i;

  if (c->size < 4)
    {
      c->sendreject ();
      return;
    }

  dest = (c->buf[2] << 8) | (c->buf[3]);
  Management_Individual m (c->t, dest);
  if (!m.init (c->l3))
    {
      c->sendreject (EIB_PROCESSING_ERROR);
      return;
    }
  c->sendreject (EIB_MC_INDIVIDUAL);
  do
    {
      i = c->readmessage ();
      if (i != -1)
	switch (EIBTYPE (c->buf))
	  {
	  case EIB_MC_PROP_READ:
	    if (c->size < 7)
	      {
		c->sendreject ();
		break;
	      }
	    {
	      CArray data, erg;
	      if (m.A_Property_Read (c->buf[2], c->buf[3],
				     (c->buf[4] << 8) | c->buf[5], c->buf[6],
				     data) == -1)
		c->sendreject ();
	      else
		{
		  erg.resize (2);
		  EIBSETTYPE (erg, EIB_MC_PROP_READ);
		  erg.setpart (data, 2);
		  c->sendmessage (erg.size(), erg.data());
		}
	    }
	    break;

	  case EIB_MC_PROP_WRITE:
	    if (c->size < 7)
	      {
		c->sendreject ();
		break;
	      }
	    {
	      CArray data, erg;
	      if (m.A_Property_Write (c->buf[2], c->buf[3],
				      (c->buf[4] << 8) | c->buf[5], c->buf[6],
				      CArray (c->buf + 7, c->size - 7),
				      erg) == -1)
		c->sendreject ();
	      else
		{
		  erg.resize (2);
		  EIBSETTYPE (erg, EIB_MC_PROP_WRITE);
		  erg.setpart (data, 2);
		  c->sendmessage (erg.size(), erg.data());
		}
	    }
	    break;

	  case EIB_RESET_CONNECTION:
	    i = -1;
	    break;

	  default:
	    c->sendreject ();
	  }
    }
  while (i != -1);
}

void
LoadImage (ClientConnPtr c, uint8_t *buf, size_t len)
{
  uchar buf[200];
  CArray img (c->buf + 2, c->size - 2);
  CArray erg;
  unsigned int j;
  BCUImage *i;
  BCU_LOAD_RESULT r = PrepareLoadImage (img, i);
  if (r != IMG_IMAGE_LOADABLE)
    {
      if (i)
	delete i;
      EIBSETTYPE (buf, EIB_LOAD_IMAGE);
      buf[2] = (r >> 8) & 0xff;
      buf[3] = (r) & 0xff;
      c->sendmessage (4, buf);
      return;
    }
  {
    uint16_t maskver;
    uchar ch;
    r = IMG_NO_DEVICE_CONNECTION;
    Management_Connection m = Management_Connection (c->t, i->addr);
    if (!m.init (c->l3))
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
	ch = 0;
	if (m.X_Memory_Write_Block (0x010d, CArray (&ch, 1)) != 0)
	  goto out;

	/*set length of the address tab to 1 */
	r = IMG_RESET_ADDR_TAB;
	ch = 0x01;
	if (m.X_Memory_Write_Block (0x0116, CArray (&ch, 1)) != 0)
	  goto out;

	/*load the data from 0x100 to 0x100 */
	r = IMG_LOAD_HEADER;
	ch = 0xff;
	if (m.X_Memory_Write_Block (0x0100, CArray (&ch, 1)) != 0)
	  goto out;

	/*load the data from 0x103 to 0x10C */
	if (m.X_Memory_Write_Block (0x0103,
				    CArray (i->code.data() + 0x03,
					    10)) != 0)
	  goto out;

	/*load the data from 0x10E to 0x115 */
	if (m.X_Memory_Write_Block (0x010E,
				    CArray (i->code.data() + 0x0E, 8)) != 0)
	  goto out;

	/*load the data from 0x119H to eeprom end */
	r = IMG_LOAD_MAIN;
	if (m.X_Memory_Write_Block (0x119,
				    CArray (i->code.data() + 0x19,
					    i->code.size() - 0x19)) != 0)
	  goto out;

	if (m.X_Memory_Write_Block (0x0100, CArray (i->code.data(), 1)) !=
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
				    CArray (i->code.data() + 0x16, 1)) != 0)
	  goto out;

	/* reset all error flags in the BCU (0x10D = 0xFF) */
	r = IMG_PREPARE_RUN;
	ch = 0xff;
	if (m.X_Memory_Write_Block (0x010d, CArray (&ch, 1)) != 0)
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

	ITER (j, i->load)
	  {
	    r = j->error;
	    if (j->obj != 0xff)
	      {
		if (m.A_Property_Write (j->obj, j->prop,
					j->start, 1, j->req,
					erg) == -1)
		  goto out;
		if (erg != j->result)
		  goto out;
	      }
	    if (j->memaddr != 0xffff)
	      if (m.X_Memory_Write_Block (j->memaddr,
					  CArray (i->code.data() +
						  j->memaddr - 0x100,
						  j->len)) != 0)
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
  c->sendmessage (4, buf);
}

