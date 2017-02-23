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

#include "link.h"
#include "server.h"
#include "sys/socket.h"
#include "systemdserver.h"
#include "lowlevel.h"
#include "groupcacheclient.h"
#include <typeinfo>
#include <iostream>

#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

static Factory<Server> _servers;
static Factory<Driver> _drivers;
static Factory<Filter> _filters;
static Factory<LowLevelDriver> _lowlevels;

bool unseen_lister(void *user, const IniSection& section, const std::string& name, const std::string& value);

/** parses an EIB individual address */
bool
Router::readaddr (const std::string& addr, eibaddr_t& parsed)
{
  int a, b, c;
  if (sscanf (addr.c_str(), "%d.%d.%d", &a, &b, &c) != 3)
    {
      ERRORPRINTF (t, E_ERROR | 55, "'%s' is not a device address. Use X.Y.Z format.", addr);
      return false;
    }
  parsed = ((a & 0x0f) << 12) | ((b & 0x0f) << 8) | (c & 0xff);
  return true;
}

bool
Router::readaddrblock (const std::string& addr, eibaddr_t& parsed, int &len)
{
  int a, b, c;
  if (sscanf (addr.c_str(), "%d.%d.%d:%d", &a, &b, &c, &len) != 4)
    {
      ERRORPRINTF (t, E_ERROR | 55, "An address block needs to look like X.Y.Z:N, not '%s'.", addr);
      return false;
    }
  parsed = ((a & 0x0f) << 12) | ((b & 0x0f) << 8) | (c & 0xff);
  return true;
}

Router::Router (IniData& d, std::string sn) : BaseRouter(d),main(sn)
                , servers(_servers.Instance())
                , filters(_filters.Instance())
                , drivers(_drivers.Instance())
                , lowlevels(_lowlevels.Instance())
{
  IniSection &s = ini[main];
  t = TracePtr(new Trace(s, s.value("name","")));
  servername = s.value("name","knxd");

  trigger.set<Router, &Router::trigger_cb>(this);
  mtrigger.set<Router, &Router::mtrigger_cb>(this);
  trigger.start();
  mtrigger.start();

  TRACEPRINTF (t, 4, "R state: initialized");
}

bool
Router::setup()
{
  std::string x;
  IniSection &s = ini[main];
  TRACEPRINTF (t, 4, "R state: setting up");

  force_broadcast = s.value("force-broadcast", false);
  unknown_ok = s.value("unknown-ok", false);

  x = s.value("addr","");
  if (!x.size())
    {
      ERRORPRINTF (t, E_ERROR | 55, "There is no KNX addr= in section '%s'.", main);
      goto ex;
    }
  if (!readaddr(x,addr))
    goto ex;

  x = s.value("client-addrs","");
  if (x.size())
    {
      if (!readaddrblock(x,client_addrs_start,client_addrs_len))
        goto ex;
      client_addrs_pos = client_addrs_len-1;
      client_addrs.resize(client_addrs_len);
      ITER(i,client_addrs)
        *i = false;
    }

    {
      IniSection& gc = s.sub("cache",false);
      if (gc.name.size() > 0)
        {
          if (!CreateGroupCache(*this, gc))
            goto ex;
        }
    }
#ifdef HAVE_SYSTEMD
  {
    std::string sd_name = s.value("systemd","");
    if (sd_name.size() > 0)
      {
        IniSection& sd = ini[sd_name];
        int num_fds = sd_listen_fds(0);
        if( num_fds < 0 )
          {
            ERRORPRINTF (t, E_ERROR | 55, "Error getting fds from systemd.");
            goto ex;
          }

        // zero FDs from systemd is not a bug
        for( int fd = SD_LISTEN_FDS_START; fd < SD_LISTEN_FDS_START+num_fds; ++fd )
          {
            if( sd_is_socket(fd, AF_UNSPEC, SOCK_STREAM, 1) <= 0 )
              {
                ERRORPRINTF (t, E_ERROR | 55, "systemd socket %d is not a socket.", fd);
                goto ex;
              }

            ServerPtr sdp = ServerPtr(new SystemdServer(*this, sd, fd));
            if (!sdp->setup())
              goto ex;
            registerLink(sdp);
            using_systemd = true;
          }
      }
  }

#endif

  x = s.value("connections","");
  {
    size_t pos = 0;
    size_t comma = 0;
    while(true)
      {
        comma = x.find(',',pos);
        std::string name = x.substr(pos,comma-pos);
        if (name.size())
          {
            LinkConnectPtr link = setup_link(name);
            if (link == nullptr)
              goto ex;
            registerLink(link);
          }
        if (comma == std::string::npos)
          break;
        pos = comma+1;
      }
  }

  if (links.size() == 0)
    {
      ERRORPRINTF (t, E_ERROR | 55, "No connections in section '%s'.", main);
      goto ex;
    }
  if (links.size() == 1)
    {
      ERRORPRINTF (t, E_ERROR | 55, "Only one connection in section '%s'.", main);
      goto ex;
    }

  if (ini.list_unseen(&unseen_lister, (void *)this))
    goto ex;

  ITER(i,links)
    ERRORPRINTF (t, E_INFO | 55, "Connected: %s.", i->second->info(2));

  TRACEPRINTF (t, 4, "R state: setup OK");
  return true;
ex:
  TRACEPRINTF (t, 4, "R state: setup BROKEN");
  return false;
}

bool
unseen_lister(void *user, 
    const IniSection& section, const std::string& name, const std::string& value)
{
  Router *r = (Router *)user;
  ERRORPRINTF (r->t, r->unknown_ok ? E_WARNING : E_FATAL, "Section '%s': unrecognized argument '%s = %s'",
      section.name, name, value);
  return !r->unknown_ok;
}

bool
Router::do_driver(LinkConnectPtr &link, IniSection& s, const std::string& drivername, bool quiet)
{
  DriverPtr driver = nullptr;

  link = LinkConnectPtr(new LinkConnect(*this, s, t));
  driver = DriverPtr(drivers.create(drivername, link, s));
  if (driver == nullptr)
    {
      if(!quiet)
        ERRORPRINTF (t, E_ERROR | 55, "Driver '%s' not found.", drivername);
      link = nullptr;
      return false;
    }
  link->set_driver(driver);
  if (!link->setup())
    link = nullptr;
  return true;
}


FilterPtr
Router::get_filter(const LinkConnectPtr& link, IniSection& s, const std::string& filtername)
{
  return FilterPtr(filters.create(filtername, link, s));
}

LowLevelDriver *
Router::get_lowlevel(const DriverPtr& parent, IniSection& s, const std::string& lowlevelname)
{
  return lowlevels.create(lowlevelname, parent, s);
}

bool
Router::do_server(ServerPtr &link, IniSection& s, const std::string& servername, bool quiet)
{
  link = ServerPtr(servers.create(servername, *this, s));
  if (link == nullptr)
    {
      if(!quiet)
        ERRORPRINTF (t, E_ERROR | 55, "Server '%s' not found.", servername);
      return false;
    }
  if (!link->setup())
    link = nullptr;
  return true;
}

LinkConnectPtr
Router::setup_link(std::string& name)
{
  IniSection& s = ini[name];
  std::string servername = s.value("server","");
  std::string drivername = s.value("driver","");
  LinkConnectPtr link = nullptr;
  ServerPtr server = nullptr;
  bool found;

  if (servername.size() && do_server(server, s,servername))
    return server;
  if (drivername.size() && do_driver(link, s,drivername))
    return link;
  if (do_server(server, s,s.name, true))
    return link;
  if (do_driver(link, s,s.name, true))
    return link;
  ERRORPRINTF (t, E_ERROR | 55, "Section '%s' has no known server nor driver.", name);
  return nullptr;
}

void
Router::start()
{
  if (want_up)
    return;
  want_up = true;

  TRACEPRINTF (t, 4, "R state: trigger going up");
  some_running = true;
  bool seen = true;

  while (seen)
    {
      seen = false;
      links_changed = false;
      ITER(i,links)
        {
          if (i->second->running || i->second->switching)
            continue;
          if (i->second->ignore)
            continue;
          seen = true;
          TRACEPRINTF (i->second->t, 3, "Start: %s", i->second->info(0));
          i->second->start();
          if (links_changed)
            break;
        }
    }
  TRACEPRINTF (t, 4, "R state: going up triggered");
}

void
Router::link_started(const LinkConnectPtr& link)
{
  bool orn = all_running;
  some_running = true;
  all_running = true;

  TRACEPRINTF (link->t, 4, "R state: started");

  ITER(i,links)
    if (!i->second->running || i->second->switching) // not up
      {
        //TRACEPRINTF (i->second->t, 4, "R state: still %s%s",
        //    i->second->switching?">":"",
        //    i->second->running?"up":"down");

        /* Ignore drivers marked as may_fail, but only if they're not still
         * transitioning */
        if (!i->second->may_fail && !i->second->switching)
          all_running = false;
      }

  if (orn != all_running)
    TRACEPRINTF (t, 4, "R state: %s", all_running?"up":"transition");

  if (all_running && !running_signal)
    {
      running_signal = true;
      TRACEPRINTF (t, 4, "R state: all drivers up");
      trigger.send();
      mtrigger.send();
#ifdef HAVE_SYSTEMD
      sd_notify(0,"READY=1");
#endif
    }
}

void
Router::link_stopped(const LinkConnectPtr& link)
{
  bool orn = some_running;

  some_running = false;
  all_running = false;

  ITER(i,links)
    if (i->second->running || i->second->switching)
      {
        some_running = true;
        //TRACEPRINTF (i->second->t, 4, "R state: still %s%s",
        //    i->second->switching?">":"",
        //    i->second->running?"up":"down");
      }

  if (orn != some_running)
    TRACEPRINTF (t, 4, "R state: %s", some_running?"transition":"down");
}

void
Router::stop()
{
  if (!want_up)
    return;
  want_up = false;

  TRACEPRINTF (t, 4, "R state: trigger Going down");
  all_running = false;
  bool seen = true;

  while(seen)
    {
      links_changed = false;
      seen = false;
      ITER(i,links)
        {
          if (i->second->running == i->second->switching)
            continue;
          TRACEPRINTF (i->second->t, 3, "Stop: %s", i->second->info(0));
          seen = true;
          i->second->stop();
          if (links_changed)
            break;
        }
    }
  TRACEPRINTF (t, 4, "R state: Going down triggered");
}

Router::~Router()
{
  TRACEPRINTF (t, 4, "R state: deleting");
  cache = nullptr;
  trigger.stop();
  mtrigger.stop();

  R_ITER(i,vbusmonitor)
    ERRORPRINTF (t, E_WARNING | 55, "VBusmonitor '%s' didn't de-register!", i->cb->name);
  vbusmonitor.clear();

  R_ITER(i,busmonitor)
    ERRORPRINTF (t, E_WARNING | 56, "Busmonitor '%s' didn't de-register!", i->cb->name);
  busmonitor.clear();

//  ITER(i,links)
//    delete i->second;
  links.clear();

  TRACEPRINTF (t, 4, "R state: deleted.");
}

void
Router::recv_L_Data (LDataPtr l, LinkConnect& link)
{
  if (some_running)
    {
      TRACEPRINTF (link.t, 9, "Queue %s", l->Decode (t));
      buf.emplace (std::move(l),std::dynamic_pointer_cast<LinkConnect>(link.shared_from_this()));
      if (running_signal)
        trigger.send();
    }
  else
    TRACEPRINTF (link.t, 9, "Queue: discard (not running) %s", l->Decode (t));
}

void
Router::recv_L_Busmonitor (LBusmonPtr l)
{
  if (some_running)
    {
      TRACEPRINTF (t, 9, "MonQueue %s", l->Decode (t));
      mbuf.push (std::move(l));
      if (running_signal)
        mtrigger.send();
    }
  else
    TRACEPRINTF (t, 9, "MonQueue: discard (not running) %s", l->Decode (t));
}

bool
Router::deregisterBusmonitor (L_Busmonitor_CallBack * c)
{
  ITER(i, busmonitor)
    if (i->cb == c)
      {
        busmonitor.erase(i);
	TRACEPRINTF (t, 3, "deregisterBusmonitor");
        return true;
      }
  TRACEPRINTF (t, 3, "deregisterBusmonitor failed");
  return false;
}

bool
Router::deregisterVBusmonitor (L_Busmonitor_CallBack * c)
{
  ITER(i,vbusmonitor)
    if (i->cb == c)
      {
	TRACEPRINTF (t, 3, "deregisterVBusmonitor");
	vbusmonitor.erase(i);
	return true;
      }
  TRACEPRINTF (t, 3, "deregisterVBusmonitor failed");
  return false;
}

bool
Router::registerBusmonitor (L_Busmonitor_CallBack * c)
{
  busmonitor.push_back((Busmonitor_Info){.cb=c});
  TRACEPRINTF (t, 3, "registerBusmonitor");
  return true;
}

bool
Router::registerVBusmonitor (L_Busmonitor_CallBack * c)
{
  vbusmonitor.push_back((Busmonitor_Info){.cb=c});
  TRACEPRINTF (t, 3, "registerVBusmonitor");
  return true;
}

bool
Router::registerLink(const LinkConnectPtr& link)
{
  const std::string& n = link->name();
  auto res = links.emplace(std::piecewise_construct,
                std::forward_as_tuple(n),
                std::forward_as_tuple(link));
  if (! res.second)
    {
      TRACEPRINTF (link->t, 3, "registerLink: %s: already present", n);
      return false;
    }
  TRACEPRINTF (link->t, 3, "registerLink: %s", n);
  links_changed = true;
  if (want_up)
    {
      all_running = false;
      TRACEPRINTF (link->t, 3, "Start: %s", link->info(0));
      link->start();
    }
  return true;
}

bool
Router::unregisterLink(const LinkConnectPtr& link)
{
  const std::string& n = link->name();
  auto res = links.find(n);
  if (res == links.end())
    {
      TRACEPRINTF (link->t, 3, "unregisterLink: %s: not present", n);
      return false;
    }
  links.erase(res);
  TRACEPRINTF (link->t, 3, "unregisterLink: %s", n);
  links_changed = true;
  return true;
}

bool
Router::hasAddress (eibaddr_t addr, LinkConnectPtr& link, bool quiet)
{
  if (addr == this->addr)
    {
      if (!quiet)
        TRACEPRINTF (t, 8, "default addr %s", FormatEIBAddr (addr));
      return true;
    }

  if (link && link->hasAddress(addr))
    {
    on_this_interface:
      if (!quiet)
        TRACEPRINTF (t, 8, "own addr %s", FormatEIBAddr (addr));
      return false;
    }

  ITER(i,links)
    {
      if (i->second == link)
        continue;
      if (i->second->hasAddress (addr))
        {
          if (i->second == link)
            {
              if (!quiet)
                TRACEPRINTF (link->t, 8, "local addr %s", FormatEIBAddr (addr));
              return false;
            }
          if (!quiet)
            TRACEPRINTF (i->second->t, 8, "found addr %s", FormatEIBAddr (addr));
          link = i->second;
          return true;
        }
    }

  if (!quiet)
    TRACEPRINTF (t, 8, "unknown addr %s", FormatEIBAddr (addr));
  return false;
}

bool
Router::checkAddress (eibaddr_t addr, LinkConnectPtr link)
{
  if (addr == 0) // always accept broadcast
    return true;

  ITER(i, links)
    {
      if (i->second == link)
        continue;
      if (i->second->checkAddress (addr))
        return true;
    }

  return false;
}

bool
Router::checkGroupAddress (eibaddr_t addr, LinkConnectPtr link)
{
  if (addr == 0) // always accept broadcast
    return true;

  ITER(i, links)
    {
      if (i->second == link)
        continue;
      if (i->second->checkGroupAddress (addr))
        return true;
    }

  return false;
}

eibaddr_t
Router::get_client_addr (TracePtr t)
{
  /*
   * Start allocating after the last-assigned address.
   * This leaves a buffer for delayed replies so that they don't get sent
   * to a new client.
   *
   * client_addrs_pos is set to len-1 in set_client_block() so that allocation
   * still starts at the first free address when starting up.
   */
  for (int i = 1; i <= client_addrs_len; i++)
    {
      unsigned int pos = (client_addrs_pos + i) % client_addrs_len;
      if (client_addrs[pos])
        continue;
      eibaddr_t a = client_addrs_start + pos;
      LinkConnectPtr link = nullptr;
      if (a != addr && !hasAddress (a, link, true))
        {
          TRACEPRINTF (t, 3, "Allocate %s", FormatEIBAddr (a));
          /* remember for next pass */
          client_addrs_pos = pos;
          client_addrs[pos] = true;
          return a;
        }
    }

  /* no more … */
  ERRORPRINTF (t, E_WARNING | 59, "Allocate: no more free addresses!");
  return 0;
}

void
Router::release_client_addr(eibaddr_t addr)
{
  if (addr < client_addrs_start)
    {
      ERRORPRINTF (t, E_ERROR | 3, "Release BAD1 %s", FormatEIBAddr (addr));
      return;
    }
  unsigned int pos = addr - client_addrs_start;
  if (pos >= client_addrs_len)
    {
      ERRORPRINTF (t, E_ERROR | 3, "Release BAD2 %s", FormatEIBAddr (addr));
      return;
    }
  if (!client_addrs[pos])
    {
      ERRORPRINTF (t, E_ERROR | 3, "Release free addr %s", FormatEIBAddr (addr));
      return;
    }

  TRACEPRINTF (t, 3, "Release %s", FormatEIBAddr (addr));
  client_addrs[pos] = false;
}

void
Router::trigger_cb (ev::async &w, int revents)
{
  unsigned i;

  while (!buf.isempty())
    {
      LPtr l = buf.get ();

      LDataPtr l1 = std::move(l.first);
      LinkConnectPtr l2 = l.second;
      LinkConnectPtr l2x = nullptr;

      if (l1->source == 0)
        l1->source = l2->addr;
      if (l1->source == 0)
        l1->source = addr;
      if (l1->source == addr) { /* locally generated, do nothing */ }
      // Cases:
      // * address is unknown: associate with input IF not from local range and not programming addr
      // * address is known to input: OK
      // * address is on another 
      else if (hasAddress (l1->source, l2x))
        {
          if (l2x != l2)
            {
              TRACEPRINTF (l2->t, 3, "Packet not from %d:%s: %s", l2->t->seq, l2->t->name, l1->Decode (t));
              goto next;
            }
        }
      else if (client_addrs_start && (l1->source >= client_addrs_start) &&
          (l1->source < client_addrs_start+client_addrs_len))
        { // late arrival to an already-freed client
          TRACEPRINTF (l2->t, 3, "Packet from client: %s", l1->Decode (t));
        }
      else if (l1->source != 0xFFFF)
        l2->addAddress (l1->source);

      if (vbusmonitor.size())
        {
          LBusmonPtr l2 = LBusmonPtr(new L_Busmonitor_PDU ());
          l2->pdu.set (l1->ToPacket ());

          ITER(i,vbusmonitor)
            i->cb->send_L_Busmonitor (LBusmonPtr(new L_Busmonitor_PDU (*l2)));
        }
      if (!l1->hopcount)
        {
          TRACEPRINTF (t, 3, "Hopcount zero: %s", l1->Decode (t));
          goto next;
        }
      if (l1->hopcount < 7 || !force_broadcast)
        l1->hopcount--;

      if (l1->repeated)
        {
          CArray d1 = l1->ToPacket ();
          ITER (i,ignore)
            if (d1 == i->data)
              {
                TRACEPRINTF (t, 9, "Repeated, discarded");
                goto next;
              }
        }
      l1->repeated = 1;
      ignore.push_back((IgnoreInfo){.data = l1->ToPacket (), .end = getTime () + 1000000});
      l1->repeated = 0;

      if (l1->AddrType == IndividualAddress
          && l1->dest == this->addr)
        l1->dest = 0;
      TRACEPRINTF (t, 3, "Route %s", l1->Decode (t));

      if (l1->AddrType == GroupAddress)
        {
          // This is easy: send to all other L2 which subscribe to the
          // group.
          ITER(i, links)
            {
              if (i->second == l2)
                continue;
              if (l1->hopcount == 7 || i->second->checkGroupAddress(l1->dest))
                i->second->send_L_Data (LDataPtr(new L_Data_PDU (*l1)));
            }
        }
      if (l1->AddrType == IndividualAddress)
        {
	  if (!l1->dest)
	    {
	      // Common problem with things that are not true gateways
	      ERRORPRINTF (l2->t, E_WARNING | 57, "Message without destination. Use the single-node filter ('-B single')?");
	      goto next;
	    }

          // we want to send to the interface on which the address
          // has appeared. If it hasn't been seen yet, we send to all
          // interfaces.
          // Address ~0 is special; it's used for programming
          // so can be on different interfaces. Always broadcast these.
          // Packets to knxd itself aren't forwarded.
          bool found = (l1->dest == this->addr);
          if (l1->dest != 0xFFFF)
            ITER(i, links)
              {
                if (i->second == l2)
                  continue;
                if (i->second->hasAddress (l1->dest))
                  {
                    found = true;
                    break;
                  }
              }
          ITER (i, links)
            {
              if (i->second == l2)
                continue;
              if (l1->hopcount == 7 || found ? i->second->hasAddress (l1->dest) : i->second->checkAddress (l1->dest))
                i->second->send_L_Data (LDataPtr(new L_Data_PDU (*l1)));
            }
        }
    next:;
    }

  // Timestamps are ordered, so we scan for the first 
  timestamp_t tm = getTime ();
  ITER (i, ignore)
    if (i->end >= tm)
      {
        ignore.erase (ignore.begin(), i);
        break;
      }
}

void
Router::mtrigger_cb (ev::async &w, int revents)
{
  unsigned i;

  while (!mbuf.isempty())
    {
      LBusmonPtr l1 = mbuf.get ();

      TRACEPRINTF (t, 3, "RecvMon %s", l1->Decode (t));
      ITER (i, busmonitor)
        i->cb->send_L_Busmonitor (LBusmonPtr(new L_Busmonitor_PDU (*l1)));
    }
}

bool
Router::checkStack(IniSection& cfg)
{
  LinkConnectPtr conn = LinkConnectPtr(new LinkConnect(*this, cfg, t));
  DriverPtr dummy = DriverPtr(drivers.create("dummy",conn,cfg));
  if (dummy == nullptr)
    return false;
  conn->set_driver(dummy);
  if (!conn->setup())
    return false;
  return true;
}

bool
Router::hasClientAddrs(bool complain)
{
  if (client_addrs_len > 0)
    return true;
  if (complain)
    ERRORPRINTF (t, E_ERROR | 51, "You need a client-addrs=X.Y.Z:N option in your %s section.", main);
  return false;

}


