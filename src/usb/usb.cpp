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

#include <stdlib.h>
#include <errno.h>
#include "usb.h"

USBLoop::USBLoop (Trace * tr)
{
  t = tr;
  if (libusb_init (&context))
    {
      ERRORPRINTF (t, E_ERROR | 40, this, "USBLoop-Create: %s", strerror(errno));
      context = 0;
      return;
    }
  TRACEPRINTF (t, 10, this, "USBLoop-Create");
  Start ();
}

USBLoop::~USBLoop ()
{
  Stop();
  if (context)
    libusb_exit (context);
}

void
USBLoop::Run (pth_sem_t * stop1)
{
  fd_set r, w, e;
  int rc, fds, i;
  struct timeval tv, tv1;
  const struct libusb_pollfd **usbfd, **usbfd_orig;
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  // The next two are dummy allocations which will be replaced later
  pth_event_t event = pth_event (PTH_EVENT_SEM, stop1);
  pth_event_t timeout = pth_event (PTH_EVENT_SEM, stop1);

  tv1.tv_sec = tv1.tv_usec = 0;
  TRACEPRINTF (t, 10, this, "LoopStart");
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      //TRACEPRINTF (t, 10, this, "LoopBegin");
      FD_ZERO (&r);
      FD_ZERO (&w);
      FD_ZERO (&e);
      fds = 0;
      rc = 0;

      usbfd = libusb_get_pollfds (context);
      usbfd_orig = usbfd;
      if (usbfd)
	while (*usbfd)
	  {
	    if ((*usbfd)->fd > fds)
	      fds = (*usbfd)->fd;
	    if ((*usbfd)->events & POLLIN)
	      FD_SET ((*usbfd)->fd, &r);
	    if ((*usbfd)->events & POLLOUT)
	      FD_SET ((*usbfd)->fd, &w);
	    usbfd++;
	  }
      free (usbfd_orig);

      i = libusb_get_next_timeout (context, &tv);
      if (i < 0)
	break;
      if (i > 0)
	{
	  pth_event (PTH_EVENT_RTIME | PTH_MODE_REUSE,
		     timeout, pth_time (tv.tv_sec, tv.tv_usec));
	  pth_event_concat (stop, timeout, NULL);
	}
      pth_event (PTH_EVENT_SELECT | PTH_MODE_REUSE, event, &rc, fds + 1, &r,
		 &w, &e);
      pth_event_concat (stop, event, NULL);
      //TRACEPRINTF (t, 10, this, "LoopWait");
      pth_wait (stop);
      //TRACEPRINTF (t, 10, this, "LoopProcess");

      pth_event_isolate (event);
      pth_event_isolate (timeout);

      if (libusb_handle_events_timeout (context, &tv1))
	break;
      //TRACEPRINTF (t, 10, this, "LoopEnd");
    }
  TRACEPRINTF (t, 10, this, "LoopStop");

  pth_event_free (timeout, PTH_FREE_THIS);
  pth_event_free (event, PTH_FREE_THIS);
  pth_event_free (stop, PTH_FREE_THIS);
}

