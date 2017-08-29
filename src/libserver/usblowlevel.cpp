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

#include "usblowlevel.h"
#include "usb.h"

USBEndpoint
parseUSBEndpoint (IniSectionPtr s)
{
  USBEndpoint e;
  e.bus = s->value("bus", -1);
  e.device = s->value("device", -1);
  e.config = s->value("config", -1);
  e.altsetting = s->value("setting", -1);
  e.interface = s->value("interface", -1);
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
  e2.dev = nullptr;
  count = libusb_get_device_list (context, &devs);

  for (i = 0; i < count; i++)
    if (check_device (devs[i], e, e2))
      break;

  libusb_free_device_list (devs, 1);

  return e2;
}

USBLowLevelDriver::USBLowLevelDriver (LowLevelIface* p, IniSectionPtr& s) : LowLevelDriver(p,s)
{
  t->setAuxName("usbL");
  read_trigger.set<USBLowLevelDriver,&USBLowLevelDriver::read_trigger_cb>(this);
  write_trigger.set<USBLowLevelDriver,&USBLowLevelDriver::write_trigger_cb>(this);
  read_trigger.start();
  write_trigger.start();
  reset();
}

void
USBLowLevelDriver::reset()
{
  state = sNone;
}

void
USBLowLevelDriver::start()
{
  int res;
  stopping = false;

  if (state >= sStarted)
    return;
  if (loop == nullptr)
    {
      ERRORPRINTF (t, E_FATAL | 28, "USBLowLevelDriver: setup not called");
      goto ex;
    }

  if ((res = libusb_open (d.dev, &dev))< 0)
    {
      ERRORPRINTF (t, E_ERROR | 28, "USBLowLevelDriver: init libusb: %s", libusb_error_name(res));
      goto ex;
    }
  libusb_unref_device (d.dev);
  state = sStarted;
  TRACEPRINTF (t, 1, "Open");
  libusb_detach_kernel_driver (dev, d.interface);
  if ((res = libusb_set_configuration (dev, d.config)) < 0)
    {
      ERRORPRINTF (t, E_ERROR | 29, "USBLowLevelDriver: setup config: %s", libusb_error_name(res));
      goto ex;
    }
  if ((res = libusb_claim_interface (dev, d.interface)) < 0)
    {
      ERRORPRINTF (t, E_ERROR | 30, "USBLowLevelDriver: claim interface: %s", libusb_error_name(res));
      goto ex;
    }
  if ((res = libusb_set_interface_alt_setting (dev, d.interface, d.altsetting)) < 0)
    {
      ERRORPRINTF (t, E_ERROR | 31, "USBLowLevelDriver: altsetting: %s", libusb_error_name(res));
      goto ex;
    }
  TRACEPRINTF (t, 1, "Claimed");
  state = sClaimed;

  TRACEPRINTF (t, 1, "Opened");

  recvh = libusb_alloc_transfer (0);
  if (!recvh)
    {
      ERRORPRINTF (t, E_ERROR | 34, "Error AllocRecv: %s", strerror(errno));
      goto ex;
    } 
  StartUsbRecvTransfer();
  state = sRunning;
  started();
  return;
ex:
  stop();
}

void
USBLowLevelDriver::abort_send()
{
  int res;
  if (sendh == nullptr)
    return;

  if ((res = libusb_cancel_transfer (sendh)) < 0)
    {
      ERRORPRINTF (t, E_ERROR | 31, "cancel %lx: %s", (unsigned long) sendh, libusb_error_name(res));
      sendh = nullptr; // XXX does this make sense?
      out.clear();
      return;
    }
  while (sendh)
    ev_run(EV_DEFAULT_ EVRUN_ONCE);
}

void
USBLowLevelDriver::stop_()
{
  TRACEPRINTF (t, 1, "Close");
  stopping = true;

  if (state > sClaimed)
    state = sClaimed;
  if (sendh)
    libusb_cancel_transfer (sendh);
  if (recvh)
    libusb_cancel_transfer (recvh);
  if (state > sClaimed)
    state = sClaimed;
  while (sendh || recvh)
    ev_run(EV_DEFAULT_ EVRUN_ONCE);

  TRACEPRINTF (t, 1, "Release");
  if (state > sStarted)
    {
      libusb_release_interface (dev, d.interface);
      libusb_attach_kernel_driver (dev, d.interface);
    }
  if (state > sNone)
    libusb_close (dev);
  delete loop;
  loop = nullptr;
  reset();
}

void
USBLowLevelDriver::stop()
{
  stop_();
  LowLevelDriver::stop();
}

USBLowLevelDriver::~USBLowLevelDriver ()
{
  stop();
  read_trigger.stop();
  write_trigger.stop();
}

void
USBLowLevelDriver::send_Data (CArray& l)
{
  if (sendh != nullptr)
    {
      ERRORPRINTF (t, E_FATAL | 35, "Send while buffer not empty");
      errored(); // XXX signal async
      return;
    }
  out = l;
  do_send();
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
  if (transfer != sendh)
    ERRORPRINTF (t, E_WARNING | 35, "SendComp %lx %lx",(unsigned long)transfer,(unsigned long)sendh);
  assert(transfer == sendh);

  TRACEPRINTF (t, 10, "SendCB %lx", (unsigned long) sendh);
  write_trigger.send();
}

void
USBLowLevelDriver::write_trigger_cb(ev::async &w UNUSED, int revents UNUSED)
{
  TRACEPRINTF (t, 10, "SendComplete %lx %d", (unsigned long)sendh, sendh->actual_length);
  if (sendh == nullptr)
    return;
  if (sendh->status == LIBUSB_TRANSFER_COMPLETED)
    {
      libusb_free_transfer (sendh);
      send_retry = 0;
      sendh = nullptr;
      send_Next();
      return;
    }
  if (sendh->status == LIBUSB_TRANSFER_TIMED_OUT && ++send_retry < 3)
    {
      ERRORPRINTF (t, E_WARNING | 35, "SendError %lx timeout, retrying", (unsigned long)sendh);
      libusb_free_transfer (sendh);
      sendh = nullptr;
      do_send();
      return;
    }
  ERRORPRINTF (t, E_ERROR | 35, "SendError %lx status %d", (unsigned long)sendh, sendh->status);
  sendh = nullptr;
  errored(); // TODO probably needs to be an async error
  return;
}

void
USBLowLevelDriver::do_send_Next()
{
  out.clear();
  LowLevelDriver::do_send_Next();
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
  TRACEPRINTF (t, 10, "RecvCB %lx", (unsigned long) recvh);
  read_trigger.send();
}

void
USBLowLevelDriver::read_trigger_cb(ev::async &w UNUSED, int revents UNUSED)
{
  TRACEPRINTF (t, 10, "RecvComplete %lx %d", (unsigned long) recvh, recvh->actual_length);
  if (recvh == nullptr)
    return;

  if (recvh->status != LIBUSB_TRANSFER_COMPLETED)
    {
      ERRORPRINTF (t, E_WARNING | 33, "RecvError %d", recvh->status);
      recvh = nullptr;
      errored();
      return;
    }
  else
    HandleReceiveUsb();

  if (state > sNone && !stopping)
    StartUsbRecvTransfer();
  else
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
                                  this, 0);
  int res = libusb_submit_transfer (recvh);
  if (res)
    {
      ERRORPRINTF (t, E_ERROR | 32, "Error StartRecv: %s", libusb_error_name(res));
      errored();
      return;
    }
  TRACEPRINTF (t, 10, "StartRecv");
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
USBLowLevelDriver::HandleReceiveUsb()
{
  CArray res;
  res.set (recvbuf, sizeof (recvbuf));
  t->TracePacket (0, "RecvUSB", res);
  master->recv_Data (res);

  if (stopping)
    return;
  if (!is_connection_state(recvbuf))
    return;
  if (get_connection_state(recvbuf))
    {
      if (state == sRunning)
        {
          ERRORPRINTF(t, E_INFO, "Connected");
          state = sConnected;
          LowLevelDriver::start();
        }
      else if (state < sRunning)
        ERRORPRINTF(t, E_WARNING, "Connected in state %d", state);
    }
  else
    {
      if (state == sConnected)
        {
          state = sRunning;
          ERRORPRINTF(t, E_ERROR, "No connection");
          stop(); // XXX TODO signal async error instead
        }
    }
}

void
USBLowLevelDriver::do_send()
{
  if (sendh || state < sClaimed || !out.size())
    return;

  t->TracePacket (0, "SendUSB", out);
  memset (sendbuf, 0, sizeof (sendbuf));
  memcpy (sendbuf, out.data(),
          (out.size() > sizeof (sendbuf) ? sizeof (sendbuf) : out.size()));
  sendh = libusb_alloc_transfer (0);
  if (!sendh)
    {
      ERRORPRINTF (t, E_ERROR | 36, "Error AllocSend: %s", strerror(errno));
      return;
    }
  libusb_fill_interrupt_transfer (sendh, dev, d.sendep, sendbuf,
                                  sizeof (sendbuf), usb_complete_send,
                                  this, 1000);
  int res = libusb_submit_transfer (sendh);
  if (res)
    {
      ERRORPRINTF (t, E_ERROR | 37, "Error StartSend: %s", libusb_error_name(res));
      return;
    }
  TRACEPRINTF (t, 0, "StartSend %lx", (unsigned long)sendh);
}

bool
USBLowLevelDriver::setup()
{
  loop = new USBLoop (t);

  if (!loop->context)
    {
      ERRORPRINTF (t, E_ERROR | 36, "setting up USB failed");
      goto ex;
    }

  TRACEPRINTF (t, 1, "Detect");
  {
    USBEndpoint e = parseUSBEndpoint (cfg);
    d = detectUSBEndpoint (loop->context, e);
  }
  if (d.dev == nullptr)
    {
      TRACEPRINTF (t, 1, "No matching endpoint found.");
      goto ex;
    }

  TRACEPRINTF (t, 1, "Using %d:%d:%d:%d:%d (%d:%d)",
	       libusb_get_bus_number (d.dev),
	       libusb_get_device_address (d.dev), d.config, d.altsetting,
	       d.interface, d.sendep, d.recvep);

  return true;
ex:
  stop_();
  return false;
}
