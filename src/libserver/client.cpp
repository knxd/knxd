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
#ifdef HAVE_BUSMONITOR
#include "busmonitor.h"
#endif
#include "connection.h"
#include "config.h"
#ifdef HAVE_MANAGEMENT
#include "managementclient.h"
#endif
#ifdef HAVE_GROUPCACHE
#include "groupcacheclient.h"
#endif

ClientConnection::ClientConnection (Server *s, int fd) : sendbuf(fd),recvbuf(fd)
{
  TRACEPRINTF (s->t, 8, this, "ClientConnection Init");
  this->t = TracePtr(new Trace(*s->t, s->t->name));
  this->l3 = s->l3;
  this->addr = l3->get_client_addr(this->t);

  this->fd = fd;
  this->s = s;

  recvbuf.on_recv_cb.set<ClientConnection,&ClientConnection::read_cb>(this);
  recvbuf.on_error_cb.set<ClientConnection,&ClientConnection::error_cb>(this);
  sendbuf.on_error_cb.set<ClientConnection,&ClientConnection::error_cb>(this);
}

ClientConnection::~ClientConnection ()
{
  /* make sure that stop() has been called */
  assert(addr == 0);
  assert(fd == -1);
}

void
ClientConnection::error_cb ()
{
  stop();
}

bool
ClientConnection::start()
{
  sendbuf.start();
  recvbuf.start();

  if (!addr)
    {
      sendreject (EIB_RESET_CONNECTION);
      stop();
      return false;
    }
  return true;
}

void
ClientConnection::stop(bool no_server)
{
  if (addr)
    {
      TRACEPRINTF (t, 8, this, "ClientConnection %s closing", FormatEIBAddr (addr).c_str());
      l3->release_client_addr(addr);
      addr = 0;
    }

  if (no_server)
    s = nullptr;
  if (a_conn)
    {
      delete a_conn;
      a_conn = nullptr;
    }
  if (fd == -1)
    return;
  if (! no_server)
    s->deregister (shared_from_this());
  sendbuf.stop();
  recvbuf.stop();
  close (fd);
  fd = -1;
}

void
ClientConnection::exit_conn()
{
  if (! a_conn)
    return;
  delete a_conn;
  a_conn = 0;
}

size_t
ClientConnection::read_cb (uint8_t *buf, size_t len)
{
  if (len < 2)
    return 0;
  int xlen = (buf[0] << 8) | (buf[1]);
  if (len < xlen+2)
    return 0;
  buf += 2;
  t->TracePacket (0, this, "ReadMessage", len, buf);

  int msg = EIBTYPE (buf);
  if (a_conn) {
    if (msg == EIB_RESET_CONNECTION)
      {
        exit_conn();
        sendreject (EIB_RESET_CONNECTION);
      }
    else
      a_conn->recv(buf,xlen);
    return xlen+2;
  }

  switch (msg)
    {
#ifdef HAVE_BUSMONITOR
    case EIB_OPEN_BUSMONITOR:
      a_conn = new A_Busmonitor (shared_from_this(), false, false, buf,xlen);
      goto new_a_conn;

    case EIB_OPEN_BUSMONITOR_TEXT:
      a_conn = new A_Text_Busmonitor (shared_from_this(), false, buf,xlen);
      goto new_a_conn;

    case EIB_OPEN_BUSMONITOR_TS:
      a_conn = new A_Busmonitor (shared_from_this(), false, true, buf,xlen);
      goto new_a_conn;

    case EIB_OPEN_VBUSMONITOR:
      a_conn = new A_Busmonitor (shared_from_this(), true, false, buf,xlen);
      goto new_a_conn;

    case EIB_OPEN_VBUSMONITOR_TEXT:
      a_conn = new A_Text_Busmonitor (shared_from_this(), true, buf,xlen);
      goto new_a_conn;

    case EIB_OPEN_VBUSMONITOR_TS:
      a_conn = new A_Busmonitor (shared_from_this(), true, true, buf,xlen);
      goto new_a_conn;
#endif
    case EIB_OPEN_T_BROADCAST:
      a_conn = new A_Broadcast (shared_from_this(), buf,xlen);
      goto new_a_conn;

    case EIB_OPEN_T_GROUP:
      a_conn = new A_Group (shared_from_this(), buf,xlen);
      goto new_a_conn;

    case EIB_OPEN_T_INDIVIDUAL:
      a_conn = new A_Individual (shared_from_this(), buf,xlen);
      goto new_a_conn;

    case EIB_OPEN_T_TPDU:
      a_conn = new A_TPDU (shared_from_this(), buf,xlen);
      goto new_a_conn;

    case EIB_OPEN_T_CONNECTION:
      a_conn = new A_Connection (shared_from_this(), buf,xlen);
      goto new_a_conn;

    case EIB_OPEN_GROUPCON:
      a_conn = new A_GroupSocket (shared_from_this(), buf,xlen);
      goto new_a_conn;

#ifdef HAVE_MANAGEMENT
    case EIB_M_INDIVIDUAL_ADDRESS_READ:
      ReadIndividualAddresses (shared_from_this(), buf,xlen);
      break;

    case EIB_PROG_MODE:
      ChangeProgMode (shared_from_this(), buf,xlen);
      break;

    case EIB_MASK_VERSION:
      GetMaskVersion (shared_from_this(), buf,xlen);
      break;

    case EIB_M_INDIVIDUAL_ADDRESS_WRITE:
      WriteIndividualAddress (shared_from_this(), buf,xlen);
      break;

    case EIB_MC_CONNECTION:
      a_conn = new ManagementConnection (shared_from_this(), buf,xlen);
      goto new_a_conn;

    case EIB_MC_INDIVIDUAL:
      a_conn = new ManagementIndividual (shared_from_this());
      goto new_a_conn;

    case EIB_LOAD_IMAGE:
      LoadImage (shared_from_this(), buf,xlen);
      break;
#endif

#ifdef HAVE_GROUPCACHE
    case EIB_CACHE_ENABLE:
    case EIB_CACHE_DISABLE:
    case EIB_CACHE_CLEAR:
    case EIB_CACHE_REMOVE:
    case EIB_CACHE_READ:
    case EIB_CACHE_READ_NOWAIT:
    case EIB_CACHE_LAST_UPDATES:
    case EIB_CACHE_LAST_UPDATES_2:
      GroupCacheRequest (shared_from_this(), buf,xlen);
      break;
#endif

    case EIB_RESET_CONNECTION:
      sendreject (EIB_RESET_CONNECTION);
      break;

    default:
      sendreject ();
      break;

    new_a_conn:
      a_conn->on_error_cb.set<ClientConnection,&ClientConnection::exit_conn>(this);
      break;
    }

  if (a_conn && !a_conn->is_connected()) // had an error
    exit_conn();

  return xlen+2;
}

void
ClientConnection::sendreject ()
{
  uchar buf[2];
  EIBSETTYPE (buf, EIB_INVALID_REQUEST);
  sendmessage (2, buf);
}

void
ClientConnection::sendreject (int type)
{
  uchar buf[2];
  EIBSETTYPE (buf, type);
  sendmessage (2, buf);
}

void
ClientConnection::sendmessage (int size, const uchar * msg)
{
  int i;
  int start;
  uchar head[2];
  assert (size >= 2);
  head[0] = (size >> 8) & 0xff;
  head[1] = (size) & 0xff;

  t->TracePacket (0, this, "SendMessage", size, msg);
  sendbuf.write(head,2);
  sendbuf.write(msg,size);
}
