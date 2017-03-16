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
#include <sys/poll.h>
#include "usb.h"
#include "types.h"

static void pollfd_added_cb (int fd UNUSED, short events UNUSED, void *user_data)
{
  USBLoop *loop = static_cast<USBLoop *>(user_data);
  loop->setup();
}

static void pollfd_removed_cb (int fd UNUSED, void *user_data)
{
  USBLoop *loop = static_cast<USBLoop *>(user_data);
  loop->setup();
}

USBLoop::USBLoop (TracePtr tr)
{
  t = tr;
  if (libusb_init (&context))
    {
      ERRORPRINTF (t, E_ERROR | 40, "USBLoop-Create: %s", strerror(errno));
      context = 0;
      return;
    }

  if(t->ShowPrint(10))
    libusb_set_debug(context,LIBUSB_LOG_LEVEL_DEBUG);
  else
    libusb_set_debug(context,LIBUSB_LOG_LEVEL_ERROR);

  tm.set<USBLoop, &USBLoop::timer_cb>(this);
  libusb_set_pollfd_notifiers (context, pollfd_added_cb,pollfd_removed_cb, this);
  setup();
  TRACEPRINTF (t, 10, "USBLoop-Create");
}

void USBLoop::timer()
{
  struct timeval tv;
  if (libusb_get_next_timeout (context, &tv) > 0)
    tm.start(tv.tv_sec+tv.tv_usec/1000000., 0);
}

void USBLoop::setup()
{
  ITER(i,fds)
    {
      (*i)->stop();
      delete *i;
    }
  fds.clear();
  const struct libusb_pollfd **usb_fds = libusb_get_pollfds(context);
  const struct libusb_pollfd **orig_usb_fds = usb_fds;
  for(const struct libusb_pollfd *it = *usb_fds; it != NULL; it = *++usb_fds)
    {
      if (it->events & POLLIN)
        {
          ev::io *io = new ev::io();
          io->set<USBLoop, &USBLoop::io_cb>(this);
          io->start(it->fd,ev::READ);
          fds.push_back(io);
          TRACEPRINTF (t, 10, "USBLoop read %d",it->fd);
        }
      if (it->events & POLLOUT)
        {
          ev::io *io = new ev::io();
          io->set<USBLoop, &USBLoop::io_cb>(this);
          io->start(it->fd,ev::WRITE);
          fds.push_back(io);
          TRACEPRINTF (t, 10, "USBLoop write %d",it->fd);
        }
    }
  free(orig_usb_fds);
}

USBLoop::~USBLoop ()
{
  if (context)
    libusb_exit (context);
  tm.stop();
  ITER(i,fds)
    {
      (*i)->stop();
      delete *i;
    }
  fds.clear();
}

void
USBLoop::timer_cb (ev::timer &w UNUSED, int revents UNUSED)
{
  struct timeval tv1 = {0,0};
  libusb_handle_events_timeout (context, &tv1);
  timer();
}

void
USBLoop::io_cb (ev::io &w UNUSED, int revents UNUSED)
{
  // TRACEPRINTF (t, 10, "USBLoop hit %d", w.fd);
  struct timeval tv1 = {0,0};
  libusb_handle_events_timeout (context, &tv1);
  timer();
}

