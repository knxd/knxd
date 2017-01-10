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
detectUSBEndpoint (libusb_context *context, USBEndpoint e)
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

USBLowLevelDriver::USBLowLevelDriver (const char *Dev, TracePtr tr) : LowLevelDriver(tr)
{
  loop = new USBLoop (tr);

  if (!loop->context)
    return;

  trigger.set<USBLowLevelDriver,&USBLowLevelDriver::trigger_cb>(this);
  trigger.start();
  TRACEPRINTF (t, 1, this, "Detect");
  USBEndpoint e = parseUSBEndpoint (Dev);
  d = detectUSBEndpoint (loop->context, e);
  state = 0;
  if (d.dev == 0)
    return;
  TRACEPRINTF (t, 1, this, "Using %d:%d:%d:%d:%d (%d:%d)",
	       libusb_get_bus_number (d.dev),
	       libusb_get_device_address (d.dev), d.config, d.altsetting,
	       d.interface, d.sendep, d.recvep);
  if (libusb_open (d.dev, &dev) < 0)
    {
      ERRORPRINTF (t, E_ERROR | 28, this, "USBLowLevelDriver: init libusb: %s", strerror(errno));
      return;
    }
  libusb_unref_device (d.dev);
  state = 1;
  TRACEPRINTF (t, 1, this, "Open");
  libusb_detach_kernel_driver (dev, d.interface);
  if (libusb_set_configuration (dev, d.config) < 0)
    {
      ERRORPRINTF (t, E_ERROR | 29, this, "USBLowLevelDriver: setup config: %s", strerror(errno));
      return;
    }
  if (libusb_claim_interface (dev, d.interface) < 0)
    {
      ERRORPRINTF (t, E_ERROR | 30, this, "USBLowLevelDriver: claim interface: %s", strerror(errno));
      return;
    }
  if (libusb_set_interface_alt_setting (dev, d.interface, d.altsetting) < 0)
    {
      ERRORPRINTF (t, E_ERROR | 31, this, "USBLowLevelDriver: altsetting: %s", strerror(errno));
      return;
    }
  TRACEPRINTF (t, 1, this, "Claimed");
  state = 2;
  connection_state = true;

  TRACEPRINTF (t, 1, this, "Opened");
}

USBLowLevelDriver::~USBLowLevelDriver ()
{
  TRACEPRINTF (t, 1, this, "Close");
  running = false;
  if (sendh)
    libusb_cancel_transfer (sendh);
  if (recvh)
    libusb_cancel_transfer (recvh);
  while (sendh || recvh)
    ev_run(EV_DEFAULT_ EVRUN_ONCE);

  TRACEPRINTF (t, 1, this, "Release");
  if (state > 0)
    {
      libusb_release_interface (dev, d.interface);
      libusb_attach_kernel_driver (dev, d.interface);
    }
  if (state > 0)
    libusb_close (dev);
  delete loop;
  TRACEPRINTF (t, 1, this, "Closed");
}

bool
USBLowLevelDriver::init ()
{
  if (state != 2)
    return false;
  // may be called a second time
  if (running)
    return true;

  recvh = libusb_alloc_transfer (0);
  if (!recvh)
    {
      ERRORPRINTF (t, E_ERROR | 34, this, "Error AllocRecv: %s", strerror(errno));
      return false;
    } 
  running = true;
  StartUsbRecvTransfer();
  return true;
}

void
USBLowLevelDriver::Send_Packet (CArray l)
{
  CArray pdu;
  t->TracePacket (1, this, "Send", l);

  send_q.put (l);
  trigger.send();
}

void
USBLowLevelDriver::SendReset ()
{
}

EMIVer USBLowLevelDriver::getEMIVer ()
{
  return vRaw;
}

void
usb_complete_send (struct libusb_transfer *transfer)
{
  USBLowLevelDriver *
    instance = (USBLowLevelDriver *) transfer->user_data;
  instance->CompleteSend(transfer);
}

void
USBLowLevelDriver::CompleteSend(struct libusb_transfer *transfer)
{
  assert(transfer == sendh);
  if (sendh->status != LIBUSB_TRANSFER_COMPLETED)
    ERRORPRINTF (t, E_WARNING | 35, this, "SendError %d", sendh->status);
  else
    {
      TRACEPRINTF (t, 0, this, "SendComplete %d",
                    sendh->actual_length);
      send_q.get ();
    }
  libusb_free_transfer (sendh);
  sendh = nullptr;
  trigger.send();
}

void
usb_complete_recv (struct libusb_transfer *transfer)
{
  USBLowLevelDriver *
    instance = (USBLowLevelDriver *) transfer->user_data;
  instance->CompleteReceive(transfer);
}

void 
USBLowLevelDriver::CompleteReceive(struct libusb_transfer *transfer)
{
  assert (transfer == recvh);
  FinishUsbRecvTransfer();
  if (running)
    StartUsbRecvTransfer();
  else if (recvh)
    {
      libusb_free_transfer (recvh);
      recvh = nullptr;
    }
}


void 
USBLowLevelDriver::StartUsbRecvTransfer()
{
  libusb_fill_interrupt_transfer (recvh, dev, d.recvep, recvbuf,
                                  sizeof (recvbuf), usb_complete_recv,
                                  this, 30000);
  if (libusb_submit_transfer (recvh))
    {
      ERRORPRINTF (t, E_ERROR | 32, this, "Error StartRecv: %s", strerror(errno));
          startUsbRecvTransferFailed = true;
      return;
    }
  TRACEPRINTF (t, 0, this, "StartRecv");
}

void
USBLowLevelDriver::FinishUsbRecvTransfer()
{
  if (recvh->status != LIBUSB_TRANSFER_COMPLETED)
    ERRORPRINTF (t, E_WARNING | 33, this, "RecvError %d", recvh->status);
  else
    {
      TRACEPRINTF (t, 0, this, "RecvComplete %d",
		   recvh->actual_length);
      ReceiveUsb();
    }
}

inline bool is_connection_state(uint8_t *recvbuf)
{
  uint8_t wanted[] = { 0x01,0x13,0x0A,0x00,0x08,0x00,0x02,0x0F,0x04,0x00,0x00,0x03 };
  return !memcmp(recvbuf, wanted, sizeof(wanted));
}

bool get_connection_state(uint8_t *recvbuf)
{
  return recvbuf[12] & 0x1;
}

void 
USBLowLevelDriver::ReceiveUsb()
{
  CArray res;
  res.set (recvbuf, sizeof (recvbuf));
  t->TracePacket (0, this, "RecvUSB", res);
  on_recv (new CArray (res));
  if (is_connection_state(recvbuf))
    connection_state = get_connection_state(recvbuf);
}

void
USBLowLevelDriver::trigger_cb(ev::async &w, int revents)
{
  if (!connection_state || send_q.isempty())
    return;

  const CArray & c = send_q.top ();
  t->TracePacket (0, this, "Send", c);
  memset (sendbuf, 0, sizeof (sendbuf));
  memcpy (sendbuf, c.data(),
          (c.size() > sizeof (sendbuf) ? sizeof (sendbuf) : c.size()));
  sendh = libusb_alloc_transfer (0);
  if (!sendh)
    {
      ERRORPRINTF (t, E_ERROR | 36, this, "Error AllocSend");
      return;
    }
  libusb_fill_interrupt_transfer (sendh, dev, d.sendep, sendbuf,
                                  sizeof (sendbuf), usb_complete_send,
                                  this, 1000);
  if (libusb_submit_transfer (sendh))
    {
      ERRORPRINTF (t, E_ERROR | 37, this, "Error StartSend");
      return;
    }
  TRACEPRINTF (t, 0, this, "StartSend");
}

