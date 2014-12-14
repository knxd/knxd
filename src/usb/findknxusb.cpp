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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libusb.h"

void
check_device (libusb_device * dev)
{
  struct libusb_device_descriptor desc;
  struct libusb_config_descriptor *cfg;
  const struct libusb_interface *intf;
  const struct libusb_interface_descriptor *alts;
  const struct libusb_endpoint_descriptor *ep;
  libusb_device_handle *h;
  char vendor[512];
  char product[512];
  int j, k, l, m;
  int in, out;

  if (libusb_get_device_descriptor (dev, &desc))
    {
      fprintf (stderr, "libusb_get_device_descriptor failed\n");
      return;
    }
  for (j = 0; j < desc.bNumConfigurations; j++)
    {
      if (libusb_get_config_descriptor (dev, j, &cfg))
	{
	  fprintf (stderr, "libusb_get_config_descriptor failed\n");
	  continue;
	}
      for (k = 0; k < cfg->bNumInterfaces; k++)
	{
	  intf = &cfg->interface[k];
	  for (l = 0; l < intf->num_altsetting; l++)
	    {
	      alts = &intf->altsetting[l];
	      if (alts->bInterfaceClass != LIBUSB_CLASS_HID)
		continue;

	      in = 0;
	      out = 0;
	      for (m = 0; m < alts->bNumEndpoints; m++)
		{
		  ep = &alts->endpoint[m];
		  if (ep->wMaxPacketSize == 64)
		    {
		      if (ep->bEndpointAddress & LIBUSB_ENDPOINT_IN)
			{
			  if ((ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK)
			      == LIBUSB_TRANSFER_TYPE_INTERRUPT)
			    in = ep->bEndpointAddress;
			}
		      else
			{
			  if ((ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK)
			      == LIBUSB_TRANSFER_TYPE_INTERRUPT)
			    out = ep->bEndpointAddress;
			}
		    }
		}

	      if (!in || !out)
		continue;
	      if (!libusb_open (dev, &h))
		{
		  memset (vendor, 0, sizeof (vendor));
		  memset (product, 0, sizeof (product));

		  if (libusb_get_string_descriptor_ascii
		      (h, desc.iManufacturer, (unsigned char *) vendor,
		       sizeof (vendor) - 1) < 0)
		    strcpy (vendor, "<Unreadable>");
		  if (libusb_get_string_descriptor_ascii
		      (h, desc.iProduct, (unsigned char *) product,
		       sizeof (product) - 1) < 0)
		    strcpy (product, "<Unreadable>");

		  printf ("device: %d:%d:%d:%d:%d (%s:%s)\n",
			  libusb_get_bus_number (dev),
			  libusb_get_device_address (dev),
			  cfg->bConfigurationValue,
			  alts->bAlternateSetting, alts->bInterfaceNumber,
			  vendor, product);
		  libusb_close (h);
		}
	    }
	}
      libusb_free_config_descriptor (cfg);
    }
}


int
main ()
{
  libusb_context *context;
  libusb_device **devs;
  int i, count;

  if (libusb_init (&context))
    {
      fprintf (stderr, "libusb init failure\n");
      exit (1);
    }
  libusb_set_debug (context, 0);
  printf ("Possible addresses for KNX USB devices:\n");
  count = libusb_get_device_list (context, &devs);

  for (i = 0; i < count; i++)
    {
      check_device (devs[i]);
    }

  libusb_free_device_list (devs, 1);
  libusb_exit (context);
}
