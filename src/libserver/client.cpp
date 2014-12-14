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

#include <errno.h>
#include <unistd.h>
#include "server.h"
#include "client.h"
#include "busmonitor.h"
#include "connection.h"
#include "managementclient.h"
#include "groupcacheclient.h"
#include "config.h"

ClientConnection::ClientConnection (Server * s, Layer3 * l3, Trace * tr,
				    int fd)
{
  TRACEPRINTF (tr, 8, this, "ClientConnection Init");
  this->fd = fd;
  this->t = tr;
  this->l3 = l3;
  this->s = s;
  buf = 0;
  buflen = 0;
}

ClientConnection::~ClientConnection ()
{
  TRACEPRINTF (t, 8, this, "ClientConnection closed");
  s->deregister (this);
  if (buf)
    delete[]buf;
  close (fd);
}

void
ClientConnection::Run (pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      if (readmessage (stop) == -1)
	break;
      int msg = EIBTYPE (buf);
      switch (msg)
	{
	case EIB_OPEN_BUSMONITOR:
	  {
	    A_Busmonitor busmon (this, l3, t, false, false);
	    busmon.Do (stop);
	  }
	  break;

	case EIB_OPEN_BUSMONITOR_TEXT:
	  {
	    A_Text_Busmonitor busmon (this, l3, t, false);
	    busmon.Do (stop);
	  }
	  break;

	case EIB_OPEN_BUSMONITOR_TS:
	  {
	    A_Busmonitor busmon (this, l3, t, false, true);
	    busmon.Do (stop);
	  }
	  break;

	case EIB_OPEN_VBUSMONITOR:
	  {
	    A_Busmonitor busmon (this, l3, t, true, false);
	    busmon.Do (stop);
	  }
	  break;

	case EIB_OPEN_VBUSMONITOR_TEXT:
	  {
	    A_Text_Busmonitor busmon (this, l3, t, true);
	    busmon.Do (stop);
	  }
	  break;

	case EIB_OPEN_VBUSMONITOR_TS:
	  {
	    A_Busmonitor busmon (this, l3, t, true, true);
	    busmon.Do (stop);
	  }
	  break;

	case EIB_OPEN_T_BROADCAST:
	  {
	    A_Broadcast cl (l3, t, this);
	    cl.Do (stop);
	  }
	  break;

	case EIB_OPEN_T_GROUP:
	  {
	    A_Group cl (l3, t, this);
	    cl.Do (stop);
	  }
	  break;

	case EIB_OPEN_T_INDIVIDUAL:
	  {
	    A_Individual cl (l3, t, this);
	    cl.Do (stop);
	  }
	  break;

	case EIB_OPEN_T_TPDU:
	  {
	    A_TPDU cl (l3, t, this);
	    cl.Do (stop);
	  }
	  break;

	case EIB_OPEN_T_CONNECTION:
	  {
	    A_Connection cl (l3, t, this);
	    cl.Do (stop);
	  }
	  break;

	case EIB_OPEN_GROUPCON:
	  {
	    A_GroupSocket cl (l3, t, this);
	    cl.Do (stop);
	  }
	  break;

	case EIB_M_INDIVIDUAL_ADDRESS_READ:
	  ReadIndividualAddresses (l3, t, this, stop);
	  break;

	case EIB_PROG_MODE:
	  ChangeProgMode (l3, t, this, stop);
	  break;

	case EIB_MASK_VERSION:
	  GetMaskVersion (l3, t, this, stop);
	  break;

	case EIB_M_INDIVIDUAL_ADDRESS_WRITE:
	  WriteIndividualAddress (l3, t, this, stop);
	  break;

	case EIB_MC_CONNECTION:
	  ManagementConnection (l3, t, this, stop);
	  break;

	case EIB_MC_INDIVIDUAL:
	  ManagementIndividual (l3, t, this, stop);
	  break;

	case EIB_LOAD_IMAGE:
	  LoadImage (l3, t, this, stop);
	  break;

	case EIB_CACHE_ENABLE:
	case EIB_CACHE_DISABLE:
	case EIB_CACHE_CLEAR:
	case EIB_CACHE_REMOVE:
	case EIB_CACHE_READ:
	case EIB_CACHE_READ_NOWAIT:
	case EIB_CACHE_LAST_UPDATES:
#ifdef HAVE_GROUPCACHE
	  GroupCacheRequest (l3, t, this, stop);
#else
	  sendreject (stop);
#endif
	  break;

	case EIB_RESET_CONNECTION:
	  sendreject (stop, EIB_RESET_CONNECTION);
	  EIBSETTYPE (buf, EIB_INVALID_REQUEST);
	  break;

	default:
	  sendreject (stop);
	}
      if (EIBTYPE (buf) == EIB_RESET_CONNECTION)
	sendreject (stop, EIB_RESET_CONNECTION);
    }
  pth_event_free (stop, PTH_FREE_THIS);
  StopDelete ();
}

int
ClientConnection::sendreject (pth_event_t stop)
{
  uchar buf[2];
  EIBSETTYPE (buf, EIB_INVALID_REQUEST);
  return sendmessage (2, buf, stop);
}

int
ClientConnection::sendreject (pth_event_t stop, int type)
{
  uchar buf[2];
  EIBSETTYPE (buf, type);
  return sendmessage (2, buf, stop);
}

int
ClientConnection::sendmessage (int size, const uchar * msg, pth_event_t stop)
{
  int i;
  int start;
  uchar head[2];
  assert (size >= 2);

  t->TracePacket (8, this, "SendMessage", size, msg);
  head[0] = (size >> 8) & 0xff;
  head[1] = (size) & 0xff;

  i = pth_write_ev (fd, head, 2, stop);
  if (i != 2)
    return -1;

  start = 0;
lp:
  i = pth_write_ev (fd, msg + start, size - start, stop);
  if (i <= 0)
    return -1;
  start += i;
  if (start < size)
    goto lp;
  return 0;
}

int
ClientConnection::readmessage (pth_event_t stop)
{
  uchar head[2];
  int i;
  unsigned start;

  i = pth_read_ev (fd, &head, 2, stop);
  if (i != 2)
    return -1;

  size = (head[0] << 8) | (head[1]);
  if (size < 2)
    return -1;

  if (size > buflen)
    {
      if (buf)
	delete[]buf;
      buf = new uchar[size];
      buflen = size;
    }

  start = 0;
lp:
  i = pth_read_ev (fd, buf + start, size - start, stop);
  if (i <= 0)
    return -1;
  start += i;
  if (start < size)
    goto lp;

  t->TracePacket (8, this, "RecvMessage", size, buf);
  return 0;
}
