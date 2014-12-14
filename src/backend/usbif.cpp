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

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "usbif.h"
#include "usb.h"

extern libusb_context *context;

USBEndpoint
parseUSBEndpoint (const char *addr)
{
  USBEndpoint e;
  e.bus = -1;
  e.device = -1;
  e.config = -1;
  e.altsetting = -1;
  e.interface = -1;
  if (!*addr)
    return e;
  if (!isdigit (*addr))
    return e;
  e.bus = atoi (addr);
  while (isdigit (*addr))
    addr++;
  if (*addr != ':')
    return e;
  addr++;
  if (!isdigit (*addr))
    return e;
  e.device = atoi (addr);
  while (isdigit (*addr))
    addr++;
  if (*addr != ':')
    return e;
  addr++;
  if (!isdigit (*addr))
    return e;
  e.config = atoi (addr);
  while (isdigit (*addr))
    addr++;
  if (*addr != ':')
    return e;
  addr++;
  if (!isdigit (*addr))
    return e;
  e.altsetting = atoi (addr);
  if (*addr != ':')
    return e;
  addr++;
  if (!isdigit (*addr))
    return e;
  e.interface = atoi (addr);
  return e;
}

bool
check_device (libusb_device * dev, USBEndpoint e, USBDevice & e2)
{
  struct libusb_device_descriptor desc;
  struct libusb_config_descriptor *cfg;
  const struct libusb_interface *intf;
  const struct libusb_interface_descriptor *alts;
  const struct libusb_endpoint_descriptor *ep;
  libusb_device_handle *h;
  int j, k, l, m;
  int in, out;

  if (!dev)
    return false;

  if (libusb_get_bus_number (dev) != e.bus && e.bus != -1)
    return false;
  if (libusb_get_device_address (dev) != e.device && e.device != -1)
    return false;

  if (libusb_get_device_descriptor (dev, &desc))
    return false;

  for (j = 0; j < desc.bNumConfigurations; j++)
    {
      if (libusb_get_config_descriptor (dev, j, &cfg))
	continue;
      if (cfg->bConfigurationValue != e.config && e.config != -1)
	{
	  libusb_free_config_descriptor (cfg);
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
	      if (alts->bAlternateSetting != e.altsetting
		  && e.altsetting != -1)
		continue;
	      if (alts->bInterfaceNumber != e.interface && e.interface != -1)
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
		  USBDevice e1;
		  e1.dev = dev;
		  libusb_ref_device (dev);
		  e1.config = cfg->bConfigurationValue;
		  e1.interface = alts->bInterfaceNumber;
		  e1.altsetting = alts->bAlternateSetting;
		  e1.sendep = out;
		  e1.recvep = in;
		  libusb_close (h);
		  e2 = e1;
		  libusb_free_config_descriptor (cfg);
		  return true;
		}
	    }
	}
      libusb_free_config_descriptor (cfg);
    }
  return false;
}

USBDevice
detectUSBEndpoint (USBEndpoint e)
{
  libusb_device **devs;
  int i, count;
  USBDevice e2;
  e2.dev = NULL;
  count = libusb_get_device_list (context, &devs);

  for (i = 0; i < count; i++)
    if (check_device (devs[i], e, e2))
      break;

  libusb_free_device_list (devs, 1);

  return e2;
}

USBLowLevelDriver::USBLowLevelDriver (const char *Dev, Trace * tr)
{
  t = tr;
  pth_sem_init (&in_signal);
  pth_sem_init (&out_signal);
  pth_sem_init (&send_empty);
  pth_sem_set_value (&send_empty, 1);
  getwait = pth_event (PTH_EVENT_SEM, &out_signal);

  TRACEPRINTF (t, 1, this, "Detect");
  USBEndpoint e = parseUSBEndpoint (Dev);
  d = detectUSBEndpoint (e);
  state = 0;
  if (d.dev == 0)
    return;
  TRACEPRINTF (t, 1, this, "Using %d:%d:%d:%d:%d (%d:%d)",
	       libusb_get_bus_number (d.dev),
	       libusb_get_device_address (d.dev), d.config, d.altsetting,
	       d.interface, d.sendep, d.recvep);
  if (libusb_open (d.dev, &dev) < 0)
    return;
  libusb_unref_device (d.dev);
  state = 1;
  TRACEPRINTF (t, 1, this, "Open");
  libusb_detach_kernel_driver (dev, d.interface);
  if (libusb_set_configuration (dev, d.config) < 0)
    return;
  if (libusb_claim_interface (dev, d.interface) < 0)
    return;
  if (libusb_set_interface_alt_setting (dev, d.interface, d.altsetting) < 0)
    return;
  TRACEPRINTF (t, 1, this, "Claimed");
  state = 2;
  connection_state = true;

  Start ();
  TRACEPRINTF (t, 1, this, "Opened");
}

USBLowLevelDriver::~USBLowLevelDriver ()
{
  TRACEPRINTF (t, 1, this, "Close");
  Stop ();
  pth_event_free (getwait, PTH_FREE_THIS);

  TRACEPRINTF (t, 1, this, "Release");
  if (state > 0)
    {
      libusb_release_interface (dev, d.interface);
      libusb_attach_kernel_driver (dev, d.interface);
    }
  TRACEPRINTF (t, 1, this, "Close");
  if (state > 0)
    libusb_close (dev);
}

bool
USBLowLevelDriver::init ()
{
  return state == 2;
}

bool
USBLowLevelDriver::Connection_Lost ()
{
  return 0;
}

void
USBLowLevelDriver::Send_Packet (CArray l)
{
  CArray pdu;
  t->TracePacket (1, this, "Send", l);

  inqueue.put (l);
  pth_sem_set_value (&send_empty, 0);
  pth_sem_inc (&in_signal, TRUE);
}

void
USBLowLevelDriver::SendReset ()
{
}

bool
USBLowLevelDriver::Send_Queue_Empty ()
{
  return inqueue.isempty ();
}

pth_sem_t *
USBLowLevelDriver::Send_Queue_Empty_Cond ()
{
  return &send_empty;
}

CArray *
USBLowLevelDriver::Get_Packet (pth_event_t stop)
{
  if (stop != NULL)
    pth_event_concat (getwait, stop, NULL);

  pth_wait (getwait);

  if (stop)
    pth_event_isolate (getwait);

  if (pth_event_status (getwait) == PTH_STATUS_OCCURRED)
    {
      pth_sem_dec (&out_signal);
      CArray *c = outqueue.get ();
      t->TracePacket (1, this, "Recv", *c);
      return c;
    }
  else
    return 0;
}

LowLevelDriverInterface::EMIVer USBLowLevelDriver::getEMIVer ()
{
  return vRaw;
}

struct usb_complete
{
  pth_sem_t
    signal;
};

void
usb_complete (struct libusb_transfer *transfer)
{
  struct usb_complete *
    complete = (struct usb_complete *) transfer->user_data;
  pth_sem_inc (&complete->signal, 0);
}

void
USBLowLevelDriver::Run (pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  pth_event_t input = pth_event (PTH_EVENT_SEM, &in_signal);
  uchar recvbuf[64];
  uchar sendbuf[64];
  struct libusb_transfer *sendh = 0;
  struct libusb_transfer *recvh = 0;
  struct usb_complete sendc, recvc;
  pth_event_t sende = pth_event (PTH_EVENT_SEM, &in_signal);
  pth_event_t recve = pth_event (PTH_EVENT_SEM, &in_signal);

  pth_sem_init (&sendc.signal);
  pth_sem_init (&recvc.signal);

  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      if (!recvh)
	{
	  recvh = libusb_alloc_transfer (0);
	  if (!recvh)
	    {
	      TRACEPRINTF (t, 0, this, "Error AllocRecv");
	      break;
	    }
	  libusb_fill_interrupt_transfer (recvh, dev, d.recvep, recvbuf,
					  sizeof (recvbuf), usb_complete,
					  &recvc, 30000);
	  if (libusb_submit_transfer (recvh))
	    {
	      TRACEPRINTF (t, 0, this, "Error StartRecv");
	      break;
	    }

	  TRACEPRINTF (t, 0, this, "StartRecv");
	  pth_event (PTH_EVENT_SEM | PTH_MODE_REUSE | PTH_UNTIL_DECREMENT,
		     recve, &recvc.signal);
	}
      if (recvh && pth_event_status (recve) == PTH_STATUS_OCCURRED)
	{
	  if (recvh->status != LIBUSB_TRANSFER_COMPLETED)
	    TRACEPRINTF (t, 0, this, "RecvError %d", recvh->status);
	  else
	    {
	      TRACEPRINTF (t, 0, this, "RecvComplete %d",
			   recvh->actual_length);
	      CArray res;
	      res.set (recvbuf, sizeof (recvbuf));
	      t->TracePacket (0, this, "RecvUSB", res);
	      outqueue.put (new CArray (res));
	      pth_sem_inc (&out_signal, 1);
	      if (recvbuf[0] == 0x01 &&
		  recvbuf[1] == 0x13 &&
		  recvbuf[2] == 0x0A &&
		  recvbuf[3] == 0x00 &&
		  recvbuf[4] == 0x08 &&
		  recvbuf[5] == 0x00 &&
		  recvbuf[6] == 0x02 &&
		  recvbuf[7] == 0x0F &&
		  recvbuf[8] == 0x04 &&
		  recvbuf[9] == 0x00 &&
		  recvbuf[10] == 0x00 && recvbuf[11] == 0x03)
		{
		  if (recvbuf[12] & 0x1)
		    connection_state = true;
		  else
		    connection_state = false;
		}
	    }
	  libusb_free_transfer (recvh);
	  recvh = 0;
	  continue;
	}
      if (sendh && pth_event_status (sende) == PTH_STATUS_OCCURRED)
	{
	  if (sendh->status != LIBUSB_TRANSFER_COMPLETED)
	    TRACEPRINTF (t, 0, this, "SendError %d", sendh->status);
	  else
	    {
	      TRACEPRINTF (t, 0, this, "SendComplete %d",
			   sendh->actual_length);
	      pth_sem_dec (&in_signal);
	      inqueue.get ();
	      if (inqueue.isempty ())
		pth_sem_set_value (&send_empty, 1);
	    }
	  libusb_free_transfer (sendh);
	  sendh = 0;
	  continue;
	}
      if (!sendh && !inqueue.isempty () && connection_state)
	{
	  const CArray & c = inqueue.top ();
	  t->TracePacket (0, this, "Send", c);
	  memset (sendbuf, 0, sizeof (sendbuf));
	  memcpy (sendbuf, c.array (),
		  (c () > sizeof (sendbuf) ? sizeof (sendbuf) : c ()));
	  sendh = libusb_alloc_transfer (0);
	  if (!sendh)
	    {
	      TRACEPRINTF (t, 0, this, "Error AllocSend");
	      break;
	    }
	  libusb_fill_interrupt_transfer (sendh, dev, d.sendep, sendbuf,
					  sizeof (sendbuf), usb_complete,
					  &sendc, 1000);
	  if (libusb_submit_transfer (sendh))
	    {
	      TRACEPRINTF (t, 0, this, "Error StartSend");
	      break;
	    }
	  TRACEPRINTF (t, 0, this, "StartSend");
	  pth_event (PTH_EVENT_SEM | PTH_MODE_REUSE | PTH_UNTIL_DECREMENT,
		     sende, &sendc.signal);
	  continue;
	}

      if (recvh)
	pth_event_concat (stop, recve, NULL);
      if (sendh)
	pth_event_concat (stop, sende, NULL);
      else
	pth_event_concat (stop, input, NULL);

      pth_wait (stop);

      pth_event_isolate (sende);
      pth_event_isolate (recve);
      pth_event_isolate (input);
    }

  if (sendh)
    {
      libusb_cancel_transfer (sendh);
      libusb_free_transfer (sendh);
    }
  if (recvh)
    {
      libusb_cancel_transfer (recvh);
      libusb_free_transfer (recvh);
    }
  pth_event_free (stop, PTH_FREE_THIS);
  pth_event_free (input, PTH_FREE_THIS);
  pth_event_free (sende, PTH_FREE_THIS);
  pth_event_free (recve, PTH_FREE_THIS);
}
