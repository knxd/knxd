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

#include "eibnettunnel.h"
#include "emi.h"

#define NO_MAP
#include "nat.h"

EIBNetIPTunnel::EIBNetIPTunnel (const LinkConnectPtr_& c, IniSectionPtr& s)
  : BusDriver(c,s)
{
  t->setAuxName("ipt");
}

EIBNetIPTunnel::~EIBNetIPTunnel ()
{
  TRACEPRINTF (t, 2, "Close");
  // restart();
  is_stopped();
}

void EIBNetIPTunnel::is_stopped()
{
  timeout.stop();
  conntimeout.stop();
  trigger.stop();
  delete sock;
  sock = nullptr;
}

void EIBNetIPTunnel::stop()
{
  restart();
  is_stopped();
  BusDriver::stop();
}

bool
EIBNetIPTunnel::setup()
{
  // Force queuing so that a broken or unreachable server can't disable the whole system
  if (!assureFilter("queue", true))
    return false;

  if (!BusDriver::setup())
    return false;
  dest = cfg->value("ip-address","");
  if (!dest.size()) 
    {
      ERRORPRINTF (t, E_ERROR | 23, "The 'ipt' driver, section %s, requires an 'ip-address=' option", cfg->name);
      return false;
    }
  port = cfg->value("dest-port",3671);
  sport = cfg->value("src-port",0);
  NAT = cfg->value("nat",false);
  monitor = cfg->value("monitor",false);
  if(NAT)
    {
      srcip = cfg->value("nat-ip","");
      dataport = cfg->value("data-port",0);
    }
  heartbeat_time = cfg->value("heartbeat-timer",30);
  heartbeat_limit = cfg->value("heartbeat-retries",3);
  return true;
}

void
EIBNetIPTunnel::start()
{
  TRACEPRINTF (t, 2, "Open");

  timeout.set <EIBNetIPTunnel,&EIBNetIPTunnel::timeout_cb> (this);
  conntimeout.set <EIBNetIPTunnel,&EIBNetIPTunnel::conntimeout_cb> (this);
  trigger.set <EIBNetIPTunnel,&EIBNetIPTunnel::trigger_cb> (this);

  trigger.start();

  sock = nullptr;
  if (!GetHostIP (t, &caddr, dest))
    goto ex;
  caddr.sin_port = htons (port);
  if (!GetSourceAddress (t, &caddr, &raddr))
    goto ex;
  raddr.sin_port = htons (sport);
  NAT = false;
  sock = new EIBNetIPSocket (raddr, (sport != 0), t);
  if (!sock->init ())
    goto ex;
  sock->on_recv.set<EIBNetIPTunnel,&EIBNetIPTunnel::read_cb>(this);
  sock->on_error.set<EIBNetIPTunnel,&EIBNetIPTunnel::error_cb>(this);

  if (srcip.size())
    {
      if (!GetHostIP (t, &saddr, srcip))
                goto ex;
      saddr.sin_port = htons (sport);
      NAT = true;
    }
  else
    saddr = raddr;
  sock->sendaddr = caddr;
  sock->recvaddr = caddr;
  sock->recvall = 0;
  support_busmonitor = true;
  connect_busmonitor = false;

  {
    EIBnet_ConnectRequest creq = get_creq();
    EIBNetIPPacket p = creq.ToPacket ();
    sock->sendaddr = caddr;
    sock->Send (p);
  }
  conntimeout.start(CONNECT_REQUEST_TIMEOUT,0);

  TRACEPRINTF (t, 2, "Opened");
  out.clear();
  return;
ex:
  if (sock) 
    {
      delete sock;
      sock = nullptr;
    }
  is_stopped();
  stopped();
}

void
EIBNetIPTunnel::error_cb ()
{
  ERRORPRINTF (t, E_ERROR | 23, "Communication error: %s", strerror(errno));
  errored();
}

void
EIBNetIPTunnel::read_cb (EIBNetIPPacket *p1)
{
  LDataPtr c;

  switch (p1->service)
    {
    case CONNECTION_RESPONSE:
      {
        EIBnet_ConnectResponse cresp;
        if (mod)
          goto err;
        if (parseEIBnet_ConnectResponse (*p1, cresp))
          {
            TRACEPRINTF (t, 1, "Recv wrong connection response");
            break;
          }
        if (cresp.status != 0)
          {
            TRACEPRINTF (t, 1, "Connect failed with error %02X", cresp.status);
            if (cresp.status == 0x23 && support_busmonitor && monitor)
              {
                TRACEPRINTF (t, 1, "Disable busmonitor support");
                restart();
                return;
                // support_busmonitor = false;
                // connect_busmonitor = false;

                // EIBnet_ConnectRequest creq = get_creq();
                // creq.CRI[1] = 0x02;

                // EIBNetIPPacket p = creq.ToPacket ();
                // TRACEPRINTF (t, 1, "Connectretry");
                // sock->Send (p, caddr);
                // conntimeout.start(10,0);
              }
            break;
          }
        if (cresp.CRD.size() != 3)
          {
            TRACEPRINTF (t, 1, "Recv wrong connection response");
            break;
          }

        auto cn = std::dynamic_pointer_cast<LinkConnect>(conn.lock());
        if (cn != nullptr)
          cn->setAddress((cresp.CRD[1] << 8) | cresp.CRD[2]);
        auto f = findFilter("single");
        if (f != nullptr)
          std::dynamic_pointer_cast<NatL2Filter>(f)->setAddress((cresp.CRD[1] << 8) | cresp.CRD[2]);

        // TODO else reject
        daddr = cresp.daddr;
        if (!cresp.nat)
          {
            if (NAT)
              {
                daddr.sin_addr = caddr.sin_addr;
                if (dataport != 0)
                  daddr.sin_port = htons (dataport);
              }
          }
        channel = cresp.channel;
        mod = 1; trigger.send();
        sno = 0;
        rno = 0;
        sock->recvaddr2 = daddr;
        sock->recvall = 3;
        if (heartbeat_time)
          conntimeout.start(heartbeat_time,0);
        heartbeat = 0;
        BusDriver::start();
        break;
      }
    case TUNNEL_REQUEST:
      {
        EIBnet_TunnelRequest treq;
        if (mod == 0)
          {
            TRACEPRINTF (t, 1, "Not connected");
            goto err;
          }
        if (parseEIBnet_TunnelRequest (*p1, treq))
          {
            TRACEPRINTF (t, 1, "Invalid request");
            break;
          }
        if (treq.channel != channel)
          {
            TRACEPRINTF (t, 1, "Not for us (treq.chan %d != %d)", treq.channel,channel);
            break;
          }
        if (((treq.seqno + 1) & 0xff) == rno)
          {
            EIBnet_TunnelACK tresp;
            tresp.status = 0;
            tresp.channel = channel;
            tresp.seqno = treq.seqno;

            EIBNetIPPacket p = tresp.ToPacket ();
            sock->Send (p, daddr);
            sock->recvall = 0;
            break;
          }
        if (treq.seqno != rno)
          {
            TRACEPRINTF (t, 1, "Wrong sequence %d<->%d",
                          treq.seqno, rno);
            if (treq.seqno < rno)
              treq.seqno += 0x100;
            if (treq.seqno >= rno + 5)
              restart();
            break;
          }
        rno++;
        if (rno > 0xff)
          rno = 0;
        EIBnet_TunnelACK tresp;
        tresp.status = 0;
        tresp.channel = channel;
        tresp.seqno = treq.seqno;

        EIBNetIPPacket p = tresp.ToPacket ();
        sock->Send (p, daddr);

        //Confirmation
        if (treq.CEMI[0] == 0x2E)
          {
            if (mod == 3)
              {
                mod = 1; trigger.send();
              }
            break;
          }
        if (treq.CEMI[0] == 0x2B)
          {
            LBusmonPtr l2 = CEMI_to_Busmonitor (treq.CEMI, std::dynamic_pointer_cast<Driver>(shared_from_this()));
            recv_L_Busmonitor (std::move(l2));
            break;
          }
        if (treq.CEMI[0] != 0x29)
          {
            TRACEPRINTF (t, 1, "Unexpected CEMI Type %02X",
                          treq.CEMI[0]);
            break;
          }
        c = CEMI_to_L_Data (treq.CEMI, t);
        if (c)
          {
            if (!monitor)
              recv_L_Data (std::move(c));
            else
              {
                LBusmonPtr p1 = LBusmonPtr(new L_Busmonitor_PDU ());
                p1->pdu = c->ToPacket ();
                recv_L_Busmonitor (std::move(p1));
              }
            break;
          }
        TRACEPRINTF (t, 1, "Unknown CEMI");
        break;
      }
    case TUNNEL_RESPONSE:
      {
        EIBnet_TunnelACK tresp;
        if (mod == 0)
          {
            TRACEPRINTF (t, 1, "Not connected");
            goto err;
          }
        if (parseEIBnet_TunnelACK (*p1, tresp))
          {
            TRACEPRINTF (t, 1, "Invalid response");
            break;
          }
        if (tresp.channel != channel)
          {
            TRACEPRINTF (t, 1, "Not for us (tresp.chan %d != %d)", tresp.channel,channel);
            break;
          }
        if (tresp.seqno != sno)
          {
            TRACEPRINTF (t, 1, "Wrong sequence %d<->%d",
                          tresp.seqno, sno);
            break;
          }
        if (tresp.status)
          {
            TRACEPRINTF (t, 1, "Error in ACK %d", tresp.status);
            break;
          }
        if (mod == 2)
          {
            sno++;
            if (sno > 0xff)
              sno = 0;
            out.clear();
            send_Next();
            mod = 1; trigger.send();
            retry = 0;
          }
        else
          TRACEPRINTF (t, 1, "Unexpected ACK mod=%d",mod);
        break;
      }
    case CONNECTIONSTATE_RESPONSE:
      {
        EIBnet_ConnectionStateResponse csresp;
        if (parseEIBnet_ConnectionStateResponse (*p1, csresp))
          {
            TRACEPRINTF (t, 1, "Invalid response");
            break;
          }
        if (csresp.channel != channel)
          {
            TRACEPRINTF (t, 1, "Not for us (csresp.chan %d != %d)", csresp.channel,channel);
            break;
          }
        if (csresp.status == 0)
          {
            if (heartbeat > 0)
              {
                heartbeat = 0;
                TRACEPRINTF (t, 1, "got Connection State Response");
              }
            else
              TRACEPRINTF (t, 1, "Duplicate Connection State Response");
          }
        else if (csresp.status == 0x21)
          {
            TRACEPRINTF (t, 1, "Connection State Response: not connected");
            restart();
          }
        else
          {
            TRACEPRINTF (t, 1, "Connection State Response Error %02x", csresp.status);
            errored();
          }
        break;
      }
    case DISCONNECT_REQUEST:
      {
        EIBnet_DisconnectRequest dreq;
        if (mod == 0)
          {
            TRACEPRINTF (t, 1, "Not connected");
            goto err;
          }
        if (parseEIBnet_DisconnectRequest (*p1, dreq))
          {
            TRACEPRINTF (t, 1, "Invalid request");
            break;
          }
        if (dreq.channel != channel)
          {
            TRACEPRINTF (t, 1, "Not for us (dreq.chan %d != %d)", dreq.channel,channel);
            break;
          }

        EIBnet_DisconnectResponse dresp;
        dresp.channel = channel;
        dresp.status = 0;

        EIBNetIPPacket p = dresp.ToPacket ();
        t->TracePacket (1, "SendDis", p.data);
        sock->Send (p, caddr);
        sock->recvall = 0;
        mod = 0;
        conntimeout.start(0.1,0);
        break;
      }
    case DISCONNECT_RESPONSE:
      {
        EIBnet_DisconnectResponse dresp;
        if (mod == 0)
          {
            TRACEPRINTF (t, 1, "Not connected");
            break;
          }
        if (parseEIBnet_DisconnectResponse (*p1, dresp))
          {
            TRACEPRINTF (t, 1, "Invalid request");
            break;
          }
        if (dresp.channel != channel)
          {
            TRACEPRINTF (t, 1, "Not for us (dresp.chan %d != %d)", dresp.channel,channel);
            break;
          }
        mod = 0;
        sock->recvall = 0;
        TRACEPRINTF (t, 1, "Disconnected");
        restart();
        conntimeout.start(0.1,0);
        break;
      }
    default:
    err:
      TRACEPRINTF (t, 1, "Recv unexpected service %04X", p1->service);
    }
  delete p1;
}

void
EIBNetIPTunnel::send_L_Data (LDataPtr l)
{
  assert(out.size() == 0);
  out = L_Data_ToCEMI (0x11, l);
  trigger.send();
}

void EIBNetIPTunnel::trigger_cb(ev::async &w UNUSED, int revents UNUSED)
{
  if (mod != 1 || out.size() == 0)
    return;

  EIBnet_TunnelRequest treq;
  treq.channel = channel;
  treq.seqno = sno;
  treq.CEMI = out;

  EIBNetIPPacket p = treq.ToPacket ();
  t->TracePacket (1, "SendTunnel", p.data);
  sock->Send (p, daddr);
  mod = 2; timeout.start(1,0);
}

void EIBNetIPTunnel::conntimeout_cb(ev::timer &w UNUSED, int revents UNUSED)
{
  if (mod)
    {
      if (heartbeat < heartbeat_limit)
        {
          EIBnet_ConnectionStateRequest csreq;
          csreq.nat = saddr.sin_addr.s_addr == 0;
          csreq.caddr = saddr;
          csreq.channel = channel;

          EIBNetIPPacket p = csreq.ToPacket ();
          TRACEPRINTF (t, 1, "Heartbeat");
          sock->Send (p, caddr);
          heartbeat++;
          if (heartbeat_time)
            conntimeout.start(heartbeat_time,0);
        }
      else
        {
          ERRORPRINTF (t, E_ERROR, "Heartbeat messages unanswered");
          restart();
        }
    }
  else
    {
      TRACEPRINTF (t, 1, "Connect timed out");
      is_stopped();
      errored();
      // EIBnet_ConnectRequest creq = get_creq();
      // creq.CRI[1] =
        // ((connect_busmonitor && support_busmonitor) ? 0x80 : 0x02);

      // TRACEPRINTF (t, 1, "Connectretry");
      // EIBNetIPPacket p = creq.ToPacket ();
      // sock->Send (p, caddr);
      // conntimeout.start(10,0);
    }
}

void
EIBNetIPTunnel::restart()
{
  if (mod == 0)
    return;
  TRACEPRINTF (t, 1, "Disconnecting");
  EIBnet_DisconnectRequest dreq;
  dreq.caddr = saddr;
  dreq.channel = channel;

  if (channel != -1)
    {
      EIBNetIPPacket p = dreq.ToPacket ();
      sock->Send (p, caddr);
    }
  sock->recvall = 0;
  mod = 0;
  conntimeout.start(0.1,0);
}

void
EIBNetIPTunnel::timeout_cb(ev::timer &w UNUSED, int revents UNUSED)
{
  if (mod != 2)
    return;
  if (retry++ > 3)
    {
      out.clear();
      TRACEPRINTF (t, 1, "Too many retransmits, disconnecting");
      restart();
    }
  else
    TRACEPRINTF (t, 1, "Retry");
  mod = 1; trigger.send();
}

