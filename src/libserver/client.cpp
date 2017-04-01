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

ClientConnection::ClientConnection (NetServerPtr s, int fd) : router(static_cast<Router&>(s->router)), sendbuf(fd),recvbuf(fd)
{
  t = TracePtr(new Trace(*(s->t)));
  t->setAuxName("CConn");
  server = s;

  TRACEPRINTF (t, 8, "ClientConnection Init");
  Router& router = static_cast<Router &>(s->router);
  this->addr = router.get_client_addr(this->t);

  this->fd = fd;

  recvbuf.on_read.set<ClientConnection,&ClientConnection::read_cb>(this);
  recvbuf.on_error.set<ClientConnection,&ClientConnection::error_cb>(this);
  sendbuf.on_error.set<ClientConnection,&ClientConnection::error_cb>(this);
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
ClientConnection::setup()
{
  return true;
}

void
ClientConnection::start()
{
  if (running)
    return;
  if (fd == -1)
    return;
  sendbuf.start();
  recvbuf.start();

  if (!addr)
    {
      sendreject (EIB_RESET_CONNECTION);
      stop();
      return;
    }
  running = true;
}

void
ClientConnection::stop()
{
  if (addr)
    {
      TRACEPRINTF (t, 8, "ClientConnection %s closing", FormatEIBAddr (addr));
      Router *router = static_cast<Router *>(&server->router);
      router->release_client_addr(addr);
      addr = 0;
    }
  exit_conn();
  server->deregister(shared_from_this());

  if (fd == -1)
    return;
  sendbuf.stop();
  recvbuf.stop();
  close (fd);
  fd = -1;
  running = false;
}

void
ClientConnection::exit_conn()
{
  if (! a_conn)
    return;
  a_conn->stop();
  if (a_conn->lc != nullptr)
    {
      router.unregisterLink(a_conn->lc);
      a_conn->lc = nullptr;
    }
  TRACEPRINTF (t, 8, "Exiting");
  delete a_conn;
  a_conn = 0;
}

#define SFT std::static_pointer_cast<ClientConnection>(shared_from_this())

size_t
ClientConnection::read_cb (uint8_t *buf, size_t len)
{
  if (len < 2)
    return 0;
  unsigned int xlen = (buf[0] << 8) | (buf[1]);
  if (len < xlen+2)
    return 0;
  buf += 2;
  t->TracePacket (0, "ReadMessage", xlen, buf);

  int msg = EIBTYPE (buf);
  if (a_conn) {
    if (msg == EIB_RESET_CONNECTION)
      {
        exit_conn();
        sendreject (EIB_RESET_CONNECTION);
      }
    else
      a_conn->recv_Data(buf,xlen);
    return xlen+2;
  }

  switch (msg)
    {
#ifdef HAVE_BUSMONITOR
    case EIB_OPEN_BUSMONITOR:
      a_conn = new A_Busmonitor (SFT, false, false);
      goto new_a_conn;

    case EIB_OPEN_BUSMONITOR_TEXT:
      a_conn = new A_Text_Busmonitor (SFT, false);
      goto new_a_conn;

    case EIB_OPEN_BUSMONITOR_TS:
      a_conn = new A_Busmonitor (SFT, false, true);
      goto new_a_conn;

    case EIB_OPEN_VBUSMONITOR:
      a_conn = new A_Busmonitor (SFT, true, false);
      goto new_a_conn;

    case EIB_OPEN_VBUSMONITOR_TEXT:
      a_conn = new A_Text_Busmonitor (SFT, true);
      goto new_a_conn;

    case EIB_OPEN_VBUSMONITOR_TS:
      a_conn = new A_Busmonitor (SFT, true, true);
      goto new_a_conn;
#endif
    case EIB_OPEN_T_BROADCAST:
      a_conn = new A_Broadcast (SFT);
      goto new_a_conn;

    case EIB_OPEN_T_GROUP:
      a_conn = new A_Group (SFT);
      goto new_a_conn;

    case EIB_OPEN_T_INDIVIDUAL:
      a_conn = new A_Individual (SFT);
      goto new_a_conn;

    case EIB_OPEN_T_TPDU:
      a_conn = new A_TPDU (SFT);
      goto new_a_conn;

    case EIB_OPEN_T_CONNECTION:
      a_conn = new A_Connection (SFT);
      goto new_a_conn;

    case EIB_OPEN_GROUPCON:
      a_conn = new A_GroupSocket (SFT);
      goto new_a_conn;

#ifdef HAVE_MANAGEMENT
    case EIB_M_INDIVIDUAL_ADDRESS_READ:
      a_conn = new ReadIndividualAddresses (SFT);
      break;

    case EIB_PROG_MODE:
      a_conn = new ChangeProgMode (SFT);
      break;

    case EIB_MASK_VERSION:
      a_conn = new GetMaskVersion (SFT);
      break;

    case EIB_M_INDIVIDUAL_ADDRESS_WRITE:
      a_conn = new WriteIndividualAddress (SFT);
      break;

    case EIB_MC_CONNECTION:
      a_conn = new ManagementConnection (SFT);
      goto new_a_conn;

    case EIB_MC_INDIVIDUAL:
      a_conn = new ManagementIndividual (SFT);
      goto new_a_conn;

    case EIB_LOAD_IMAGE:
      a_conn = new LoadImage (SFT);
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
      GroupCacheRequest (SFT, buf,xlen);
      break;
#endif

    case EIB_RESET_CONNECTION:
      sendreject (EIB_RESET_CONNECTION);
      break;

    default:
      sendreject ();
      break;

    new_a_conn:
      a_conn->on_error.set<ClientConnection,&ClientConnection::exit_conn>(this);
      if (a_conn->setup(buf,xlen))
        {
          if (a_conn->lc != nullptr && !router.registerLink(a_conn->lc, true))
            {
              exit_conn();
              sendreject();
            }
          else
            a_conn->start();
        }
      else
        {
          TRACEPRINTF (t, 8, "Error setting up conn, msg=x%x",msg);
          exit_conn();
          sendreject();
        }
      break;
    }

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
  uchar head[2];
  assert (size >= 2);
  head[0] = (size >> 8) & 0xff;
  head[1] = (size) & 0xff;

  t->TracePacket (0, "Send", size, msg);
  sendbuf.write(head,2);
  sendbuf.write(msg,size);
}
