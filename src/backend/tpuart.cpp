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

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "tpuart.h"

#define TPUART_MAGIC           'E'
#define TPUART_SET_PH_ADDR  _IOW  (TPUART_MAGIC, 2, unsigned short)
#define TPUART_UNSET_PH_ADDR _IOW (TPUART_MAGIC, 18, unsigned short)
#define TPUART_SET_GR_ADDR  _IOW  (TPUART_MAGIC, 4, unsigned short)
#define TPUART_UNSET_GR_ADDR _IOW (TPUART_MAGIC, 5, unsigned short)
#define TPUART_RESET         _IO (TPUART_MAGIC, 8)
#define TPUART_BUSMON_ON     _IO (TPUART_MAGIC, 9)
#define TPUART_BUSMON_OFF    _IO (TPUART_MAGIC, 10)

struct message
{
  struct timespec timestamp;
  unsigned short length;
  unsigned char data[64];
};


TPUARTLayer2Driver::TPUARTLayer2Driver (int version, const char *device,
					eibaddr_t a, Trace * tr)
{
  t = tr;
  TRACEPRINTF (t, 2, this, "Open");
  addr = a;
  ver = version;

  pth_sem_init (&in_signal);
  pth_sem_init (&out_signal);
  getwait = pth_event (PTH_EVENT_SEM, &out_signal);

  fd = open (device, O_RDWR);
  if (fd == -1)
    return;

  addAddress (a);
  Start ();
  mode = 0;
  vmode = 0;
  TRACEPRINTF (t, 2, this, "Opened");
}

TPUARTLayer2Driver::~TPUARTLayer2Driver ()
{
  TRACEPRINTF (t, 2, this, "Close");
  Stop ();
  pth_event_free (getwait, PTH_FREE_THIS);
  while (!inqueue.isempty ())
    delete inqueue.get ();
  while (!outqueue.isempty ())
    delete outqueue.get ();

  if (fd != -1)
    {
      removeAddress (addr);
      close (fd);
    }
}

bool
TPUARTLayer2Driver::init ()
{
  return fd != -1;
}

bool
TPUARTLayer2Driver::openVBusmonitor ()
{
  vmode = 1;
  return 1;
}

bool
TPUARTLayer2Driver::closeVBusmonitor ()
{
  vmode = 0;
  return 1;
}

bool TPUARTLayer2Driver::addAddress (eibaddr_t addr)
{
  return ioctl (fd, TPUART_SET_PH_ADDR, &addr) != -1;
}

bool TPUARTLayer2Driver::addGroupAddress (eibaddr_t addr)
{
  return ioctl (fd, TPUART_UNSET_PH_ADDR, &addr) != -1;
}

bool TPUARTLayer2Driver::removeAddress (eibaddr_t addr)
{
  return ioctl (fd, TPUART_SET_GR_ADDR, &addr) != -1;
}

bool TPUARTLayer2Driver::removeGroupAddress (eibaddr_t addr)
{
  return ioctl (fd, TPUART_UNSET_GR_ADDR, &addr) != -1;
}

bool TPUARTLayer2Driver::enterBusmonitor ()
{
  mode = 1;
  return ioctl (fd, TPUART_BUSMON_ON) != -1;
}

bool TPUARTLayer2Driver::leaveBusmonitor ()
{
  mode = 0;
  return ioctl (fd, TPUART_BUSMON_OFF) != -1;
}

bool TPUARTLayer2Driver::Open ()
{
  return ioctl (fd, TPUART_RESET) != -1;
}

bool TPUARTLayer2Driver::Close ()
{
  return ioctl (fd, TPUART_RESET) != -1;
}

eibaddr_t TPUARTLayer2Driver::getDefaultAddr ()
{
  return addr;
}

bool TPUARTLayer2Driver::Connection_Lost ()
{
  return 0;
}

bool TPUARTLayer2Driver::Send_Queue_Empty ()
{
  return inqueue.isempty ();
}

void
TPUARTLayer2Driver::Send_L_Data (LPDU * l)
{
  TRACEPRINTF (t, 2, this, "Send %s", l->Decode ()());
  inqueue.put (l);
  pth_sem_inc (&in_signal, 1);
}

LPDU *
TPUARTLayer2Driver::Get_L_Data (pth_event_t stop)
{
  if (stop != NULL)
    pth_event_concat (getwait, stop, NULL);

  pth_wait (getwait);

  if (stop)
    pth_event_isolate (getwait);

  if (pth_event_status (getwait) == PTH_STATUS_OCCURRED)
    {
      pth_sem_dec (&out_signal);
      LPDU *l = outqueue.get ();
      TRACEPRINTF (t, 2, this, "Recv %s", l->Decode ()());
      return l;
    }
  else
    return 0;
}

void
TPUARTLayer2Driver::Run (pth_sem_t * stop1)
{
  struct message m;
  int l;
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  pth_event_t input = pth_event (PTH_EVENT_SEM, &in_signal);
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      pth_event_concat (stop, input, NULL);
      l = pth_read_ev (fd, &m, sizeof (m), stop);
      if (l >= 0)
	{
	  LPDU *l1;
	  if (m.length > sizeof (m.data))
	    m.length = sizeof (m.data);
	  t->TracePacket (0, this, "Recv", m.length, m.data);
	  if (vmode && mode == 0)
	    {
	      L_Busmonitor_PDU *l2 = new L_Busmonitor_PDU;
	      l2->pdu.set (m.data, m.length);
	      outqueue.put (l2);
	      pth_sem_inc (&out_signal, 1);
	    }
	  if (mode == 0)
	    l1 = LPDU::fromPacket (CArray (m.data, m.length));
	  else
	    {
	      l1 = new L_Busmonitor_PDU;
	      ((L_Busmonitor_PDU *) l1)->pdu.set (m.data, m.length);
	    }
	  outqueue.put (l1);
	  pth_sem_inc (&out_signal, 1);
	}
      pth_event_isolate (stop);
      if (!inqueue.isempty ())
	{
	  LPDU *l1 = inqueue.top ();
	  CArray c = l1->ToPacket ();
	  unsigned len = c ();
	  if (len > sizeof (m.data))
	    len = sizeof (m.data);
	  memcpy (m.data, c.array (), len);
	  m.length = len;
	  if (ver)
	    m.length--;
	  t->TracePacket (0, this, "Send", m.length, m.data);
	  l = pth_write_ev (fd, &m, sizeof (m), stop);
	  if (l >= 0)
	    {
	      if (vmode)
		{
		  L_Busmonitor_PDU *l2 = new L_Busmonitor_PDU;
		  l2->pdu.set (c);
		  outqueue.put (l2);
		  pth_sem_inc (&out_signal, 1);
		}
	      pth_sem_dec (&in_signal);
	      delete inqueue.get ();
	    }
	}
    }
  pth_event_free (stop, PTH_FREE_THIS);
  pth_event_free (input, PTH_FREE_THIS);
}
