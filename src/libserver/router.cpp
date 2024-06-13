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

#include "router.h"

#include <iostream>
#include <math.h>
#include <sys/socket.h>
#include <typeinfo>

#include <ev++.h>
#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

#include "cm_tp1.h"
#ifdef HAVE_GROUPCACHE
#include "groupcacheclient.h"
#endif
#include "link.h"
#include "lowlevel.h"
#include "server.h"
#include "systemdserver.h"
#include "tcptunserver.h"

/** global filter adapter, sending end */
class RouterHigh : public Driver
{
  Router* router;
public:
  RouterHigh(Router& r, const RouterLowPtr& rl);
  virtual ~RouterHigh() = default;

  virtual void recv_L_Data (LDataPtr l);
  virtual void recv_L_Busmonitor (LBusmonPtr l);

  virtual void send_L_Data (LDataPtr l)
  {
    router->send_L_Data(std::move(l));
  }

  virtual bool checkAddress (eibaddr_t addr) const override
  {
    LinkConnectPtr link = nullptr;
    return router->checkAddress(addr, link);
  }

  virtual bool checkGroupAddress (eibaddr_t addr) const override
  {
    LinkConnectPtr link = nullptr;
    return router->checkGroupAddress(addr, link);
  }

  virtual bool hasAddress (eibaddr_t addr) const override
  {
    LinkConnectPtr link = nullptr;
    return router->hasAddress(addr, link);
  }

  virtual void addAddress (eibaddr_t addr) override
  {
    if (addr != router->addr)
      ERRORPRINTF (t, E_ERROR | 80, "%s filter: Trying to add address %s", router->main, FormatEIBAddr(addr));
  }

  virtual void start()
  {
    router->start_();
  }

  virtual void stop(bool err)
  {
    router->stop_(err);
  }

  /** prevent dup calls to started() */
  bool is_started = false;
  virtual void started();
  virtual void stopped(bool err);

};

/** global filter adapter, receiving end */
class RouterLow : public LinkConnect_
{
public:
  Router* router;
  RouterLow(Router& r);
  virtual ~RouterLow() = default;

  virtual void recv_L_Data (LDataPtr l)
  {
    router->queue_L_Data (std::move(l));
  }
  virtual void recv_L_Busmonitor (LBusmonPtr l)
  {
    router->queue_L_Busmonitor (std::move(l));
  }
  virtual void send_Next ();

  virtual void start()
  {
    send->start();
  }
  virtual void stop(bool err)
  {
    send->stop(err);
  }

  virtual void started()
  {
    router->started();
  }
  virtual void stopped(bool err)
  {
    router->stopped(err);
  }
};

static Factory<Server> _servers;
static Factory<Driver> _drivers;
static Factory<Filter> _filters;

bool unseen_lister(void *user, const IniSection& section, const std::string& name, const std::string& value);

/** parses an EIB individual address */
bool
Router::readaddr (const std::string& addr, eibaddr_t& parsed)
{
  int a, b, c;
  if (sscanf (addr.c_str(), "%d.%d.%d", &a, &b, &c) != 3)
    {
      ERRORPRINTF (t, E_ERROR | 81, "'%s' is not a device address. Use X.Y.Z format.", addr);
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
      ERRORPRINTF (t, E_ERROR | 82, "An address block needs to look like X.Y.Z:N, not '%s'.", addr);
      return false;
    }
  parsed = ((a & 0x0f) << 12) | ((b & 0x0f) << 8) | (c & 0xff);
  return true;
}

Router::Router (IniData& d, std::string sn) : BaseRouter(d)
  , servers(_servers.Instance())
  , filters(_filters.Instance())
  , drivers(_drivers.Instance())
  , main(sn)
{
  IniSectionPtr s = ini[main];
  t = TracePtr(new Trace(s, s->value("name","")));
  servername = s->value("name","knxd");

  r_low = RouterLowPtr(new RouterLow(*this));
  r_high = RouterHighPtr(new RouterHigh(*this, r_low));
  r_low->set_driver(std::dynamic_pointer_cast<Driver>(r_high));

  trigger.set<Router, &Router::trigger_cb>(this);
  mtrigger.set<Router, &Router::mtrigger_cb>(this);
  state_trigger.set<Router, &Router::state_trigger_cb>(this);
  trigger.start();
  mtrigger.start();
  state_trigger.start();

  TRACEPRINTF (t, 4, "initialized");
}

bool
Router::setup()
{
  std::string x;
  IniSectionPtr s = ini[main];
  TRACEPRINTF (t, 4, "setting up");

  force_broadcast = s->value("force-broadcast", false);
  unknown_ok = s->value("unknown-ok", false);

  x = s->value("addr","");
  if (!x.size())
    {
      ERRORPRINTF (t, E_ERROR | 84, "There is no KNX addr= in section '%s'.", main);
      goto ex;
    }
  if (!readaddr(x,addr))
    goto ex;

  x = s->value("client-addrs","");
  if (x.size())
    {
      if (!readaddrblock(x,client_addrs_start,client_addrs_len))
        goto ex;
      client_addrs_pos = client_addrs_len-1;
      client_addrs.resize(client_addrs_len);
      ITER(i,client_addrs)
      *i = false;
    }

#ifdef HAVE_GROUPCACHE
  {
    IniSectionPtr gc = s->sub("cache",false);
    if (gc->name.size() > 0)
      {
        if (!CreateGroupCache(*this, gc))
          goto ex;
      }
  }
#endif

  if (!r_low->setup())
    return false;

#ifdef HAVE_SYSTEMD
  {
    std::string sd_name_knxd = s->value("systemd","");
    std::string sd_name_tun = s->value("systemd_tcptunsrv","");
    if (sd_name_knxd.size() > 0 || sd_name_tun.size() > 0)
      {
        // IniSectionPtr sd = ini[sd_name];
        // set the section's "referenced" bit, for error checking
        if (sd_name_knxd.size() > 0)
          (void) ini[sd_name_knxd];
        if (sd_name_tun.size() > 0)
          (void) ini[sd_name_tun];

        char** fd_names_c = nullptr;
        int num_fds = sd_listen_fds_with_names(0, &fd_names_c);
        if( num_fds < 0 )
          {
            ERRORPRINTF (t, E_ERROR | 85, "Error getting fds from systemd.");
            goto ex;
          }

        std::vector<std::string> fd_names;
        if (fd_names_c)
          {
            for (char** name = fd_names_c; *name; name++)
              {
                fd_names.push_back(*name);
                free(*name);
              }
            free(fd_names_c);
          }
        if (fd_names.size() < num_fds)
          {
            ERRORPRINTF (t, E_ERROR | 147, "Got too few fds names from systemd.");
            goto ex;
          }

        // zero FDs from systemd is not a bug
        for( int fd = SD_LISTEN_FDS_START; fd < SD_LISTEN_FDS_START+num_fds; ++fd )
          {
            std::string name = fd_names[fd - SD_LISTEN_FDS_START];
            TRACEPRINTF (t, 4, "Got fd %d with name '%s'", fd, name);
            std::string sd_name;
            bool is_tun;
            if (name == "knxnetip") {
              sd_name = sd_name_tun;
              is_tun = true;
            } else {
              sd_name = sd_name_knxd;
              is_tun = false;
            }

            IniSectionPtr sds = ini.add_auto(sd_name);

            (*sds)["use"] = sd_name;
            if( sd_is_socket(fd, AF_UNSPEC, SOCK_STREAM, 1) <= 0 )
              {
                ERRORPRINTF (t, E_ERROR | 86, "systemd socket %d is not a socket.", fd);
                goto ex;
              }

            ServerPtr sdp;
            if (is_tun) {
              sdp = ServerPtr(new TcpTunSystemdServer(*this, sds, fd));
            } else {
              sdp = ServerPtr(new SystemdServer(*this, sds, fd));
            }
            if (!sdp->setup())
              goto ex;
            registerLink(sdp);

            using_systemd = true;
          }
      }
  }

#endif

  x = s->value("connections","");
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
      ERRORPRINTF (t, E_ERROR | 87, "No connections in section '%s'.", main);
      goto ex;
    }
  if (links.size() == 1)
    {
      ERRORPRINTF (t, E_ERROR | 88, "Only one connection in section '%s'.", main);
      goto ex;
    }

  if (ini.list_unseen(&unseen_lister, (void *)this))
    goto ex;

  ITER(i,links)
  ERRORPRINTF (t, E_INFO | 129, "Connected: %s.", i->second->info(2));

  TRACEPRINTF (t, 4, "setup OK");
  return true;
ex:
  TRACEPRINTF (t, 4, "setup BROKEN");
  return false;
}

bool
unseen_lister(void *user,
              const IniSection& section, const std::string& name, const std::string& value)
{
  Router *r = (Router *)user;
  ERRORPRINTF (r->t, (r->unknown_ok ? E_WARNING : E_FATAL) | 104, "Section '%s': unrecognized argument '%s = %s'",
               section.name, name, value);
  return !r->unknown_ok;
}

bool
Router::do_driver(LinkConnectPtr &link, IniSectionPtr& s, const std::string& drivername, bool quiet)
{
  DriverPtr driver = nullptr;

  link = LinkConnectPtr(new LinkConnect(*this, s, t));
  driver = DriverPtr(drivers.create(drivername, link, s));
  if (driver == nullptr)
    {
      if(!quiet)
        ERRORPRINTF (t, E_ERROR | 89, "Driver '%s' not found.", drivername);
      link = nullptr;
      return false;
    }
  link->set_driver(driver);
  if (!link->setup())
    link = nullptr;
  return true;
}


FilterPtr
Router::get_filter(const LinkConnectPtr_& link, IniSectionPtr& s, const std::string& filtername)
{
  return FilterPtr(filters.create(filtername, link, s));
}

bool
Router::do_server(ServerPtr &link, IniSectionPtr& s, const std::string& servername, bool quiet)
{
  link = ServerPtr(servers.create(servername, *this, s));
  if (link == nullptr)
    {
      if(!quiet)
        ERRORPRINTF (t, E_ERROR | 90, "Server '%s' not found.", servername);
      return false;
    }
  if (!link->setup())
    link = nullptr;
  return true;
}

LinkConnectPtr
Router::setup_link(std::string& name)
{
  IniSectionPtr s = ini[name];
  std::string servername = s->value("server","");
  std::string drivername = s->value("driver","");
  LinkConnectPtr link = nullptr;
  ServerPtr server = nullptr;

  if (servername.size() && do_server(server, s,servername))
    return server;
  if (drivername.size() && do_driver(link, s,drivername))
    return link;
  if (do_server(server, s,s->name, true))
    return server;
  if (do_driver(link, s,s->name, true))
    return link;
  ERRORPRINTF (t, E_ERROR | 91, "Section '%s' has no known server nor driver.", name);
  return nullptr;
}

void
Router::start()
{
  if (want_up)
    return;
  want_up = true;

  TRACEPRINTF (t, 4, "trigger going up");
  r_low->start();
}

void
Router::start_()
{
  some_running = true;
  bool seen = true;

  in_link_loop += 1;
  seq++;
  while (seen)
    {
      seen = false;
      links_changed = false;
      ITER(i,links)
      {
        auto ii = i->second;
        if (ii->ignore || ii->transient)
          continue;
        if (ii->seq >= seq)
          continue;
        seen = true;
        ii->seq = seq;
        TRACEPRINTF (ii->t, 3, "Start: %s", ii->info(0));
        ii->setState(L_going_up);
        if (links_changed)
          break;
      }
    }
  in_link_loop -= 1;
  TRACEPRINTF (t, 4, "going up triggered");
}

void
Router::send_Next()
{
  if (high_sending)
    {
      TRACEPRINTF (t, 6, "sending set");
      return;
    }
  if (high_send_more)
    {
      TRACEPRINTF (t, 6, "send_more set");
      return;
    }
  ITER(i,links)
  {
    auto ii = i->second;
    if (ii->state != L_up)
      {
        TRACEPRINTF (ii->t, 6, "not up");
        continue;
      }
    if (!ii->send_more)
      {
        TRACEPRINTF (ii->t, 6, "still waiting");
        return;
      }
    TRACEPRINTF (ii->t, 6, "is OK");
  }
  TRACEPRINTF (t, 6, "OK");
  high_send_more = true;
  r_high->send_Next();
}

void
Router::linkStateChanged(const LinkConnectPtr& link)
{
  TRACEPRINTF (link->t, 4, "link state changed: %s", link->stateName());
  linkChanges.push(link);
  state_trigger.send();
}

void
Router::state_trigger_cb (ev::async &, int)
{
  bool oarn = all_running;
  bool osrn = some_running;

  int n_up = 0;
  int n_going = 0;
  int n_down = 0;
  int n_error = 0;

  TRACEPRINTF (t, 4, "check start");

  while (!linkChanges.empty())
    {
      LinkConnectPtr l = linkChanges.get();
      if (!want_up && l->state >= L_up)
        l->setState(L_going_down);
    }

  ITER(i,links)
  {
    auto ii = i->second;
    LConnState lcs = ii->state;

    if (ii->transient)
      {
        if (!want_up && lcs >= L_up)
          {
            ii->setState(L_going_down);
            n_going ++;
          }
        continue;
      }

    lcs = ii->state;
    switch(lcs)
      {
      case L_down:
        TRACEPRINTF (ii->t, 4, "is %s", ii->stateName());
        if (!ii->ignore)
          n_down++;
        break;
      case L_error:
        if (!ii->ignore)
          {
            n_down++;
            n_error++;
          }
        break;
      case L_up:
        n_up++;
        break;
      default:
        TRACEPRINTF (ii->t, 4, "state is %s", ii->stateName());
        n_going++;
      }
  }

  if (!n_going && !n_down && n_up)
    {
      all_running = true;
      some_running = true;
    }
  else if (!n_going && !n_up)
    {
      some_running = false;
      all_running = false;
    }
  else
    {
      some_running = true;
      all_running = false;
    }

  TRACEPRINTF (t, 4, "check end: want_up %d some %d>%d all %d>%d, going %d up %d down %d", want_up, osrn,some_running, oarn,all_running, n_going,n_up,n_down);
  if (want_up && n_error)
    {
      // Hard report errors
      ITER(i,links)
      {
        auto ii = i->second;
        LConnState lcs = ii->state;
        switch(lcs)
          {
          case L_error:
            if (!ii->ignore)
              ERRORPRINTF (ii->t, E_FATAL | 105, "Link down, terminating");
          }
      }
      exitcode = 1;
      stop(true);
    }

  if (osrn && !some_running)
    r_high->stopped(n_error > 0);
  else if (!oarn && all_running)
    r_high->started();
}

void
Router::stop(bool err)
{
  if (!want_up)
    return;
  want_up = false;

  TRACEPRINTF (t, 4, "trigger Going down");
  r_low->stop(err);
}

void
Router::stop_(bool err)
{
  all_running = false;
  bool seen = true;

  if (want_up)
    {
      // we get here when there's a failure to start the global filter chain
      stop(err);
      return;
    }

  in_link_loop += 1;
  seq++;
  while(seen)
    {
      links_changed = false;
      seen = false;
      ITER(i,links)
      {
        auto ii = i->second;
        if (ii->seq >= seq)
          continue; // already told it
        TRACEPRINTF (ii->t, 4, "R Stopping");
        seen = true;
        ii->seq = seq;
        ii->setState(L_going_down);
        if (links_changed)
          break;
      }
    }
  in_link_loop -= 1;
}


void
Router::started()
{
  low_send_more = true;
  high_send_more = true;
  trigger.send();
  mtrigger.send();

  if (all_running && !running_signal)
    {
      running_signal = true;
      TRACEPRINTF (t, 4, "all drivers up");
#ifdef HAVE_SYSTEMD
      sd_notify(0,"READY=1");
#endif
    }

  TRACEPRINTF (t, 4, "up");
}

void
Router::stopped(bool err)
{
  TRACEPRINTF (t, 4, "down");
  if (want_up)
    stop(err);
  else
    ev_break (EV_A_ EVBREAK_ALL);
}

Router::~Router()
{
  TRACEPRINTF (t, 4, "deleting");
  cache = nullptr;
  trigger.stop();
  mtrigger.stop();
  state_trigger.stop();

  R_ITER(i,vbusmonitor)
  ERRORPRINTF (t, E_WARNING | 55, "VBusmonitor '%s' didn't de-register!", i->cb->name);
  vbusmonitor.clear();

  R_ITER(i,busmonitor)
  ERRORPRINTF (t, E_WARNING | 56, "Busmonitor '%s' didn't de-register!", i->cb->name);
  busmonitor.clear();

//  ITER(i,links)
//    delete i->second;
  links.clear();

  TRACEPRINTF (t, 4, "deleted.");
}

void
Router::recv_L_Data (LDataPtr l, LinkConnect& link)
{
  LinkConnectPtr l2x = nullptr;

  if (l->address_type == IndividualAddress && l->destination_address == 0)
    {
      // Common problem with things that are not true gateways
      ERRORPRINTF (link.t, E_WARNING | 57, "Message without destination. Use the single-node filter ('-B single')?");
      return;
    }

  // Unassigned source: set to link's, or our, address
  if (l->source_address == 0)
    {
      l->source_address = link.addr;
      if (l->source_address == 0)
        l->source_address = addr;
    }

  if (l->source_address == addr)
    {
      // locally generated?
      if (!link.is_local)
        {
          // Nope. Reject.
          TRACEPRINTF (link.t, 3, "Packet not from us");
          return;
        }
    }
  else if (hasAddress (l->source_address, l2x))
    {
      // check if from the correct interface
      if (&*l2x != &link)
        {
          TRACEPRINTF (link.t, 3, "Packet not from %d:%s: %s", l2x->t->seq, l2x->t->name, l->Decode (t));
          return;
        }
    }
  else if (client_addrs_len && l->source_address >= client_addrs_start && l->source_address < client_addrs_start+client_addrs_len)
    {
      TRACEPRINTF (link.t, 3, "Packet originally from closed local interface");
      return;
    }
  else if (l->source_address != 0xFFFF)   // don't assign the "unprogrammed" address
    {
      link.addAddress (l->source_address);
    }

  l->source = &link;
  r_high->recv_L_Data(std::move(l));
}

void
Router::recv_L_Busmonitor (LBusmonPtr l)
{
  r_high->recv_L_Busmonitor(std::move(l));
}

void
Router::queue_L_Data (LDataPtr l)
{
  if (some_running || want_up)
    {
      buf.emplace (std::move(l));
      if (running_signal)
        trigger.send();
    }
  else
    TRACEPRINTF (t, 9, "Queue: discard (not running) %s", l->Decode (t));
}

void
Router::queue_L_Busmonitor (LBusmonPtr l)
{
  if (some_running || want_up)
    {
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
  busmonitor.push_back((Busmonitor_Info)
  {
    .cb=c
  });
  TRACEPRINTF (t, 3, "registerBusmonitor");
  return true;
}

bool
Router::registerVBusmonitor (L_Busmonitor_CallBack * c)
{
  vbusmonitor.push_back((Busmonitor_Info)
  {
    .cb=c
  });
  TRACEPRINTF (t, 3, "registerVBusmonitor");
  return true;
}

bool
Router::registerLink(const LinkConnectPtr& link, bool transient)
{
  const std::string& n = link->name();
#if 1 // TODO tracing
  link->pos = link->t->seq;
#else
  static int pos = 0;
  if (link->pos == 0)
    link->pos = ++pos;
#endif
  auto res = links.emplace(std::piecewise_construct,
                           std::forward_as_tuple(link->pos),
                           std::forward_as_tuple(link));
  if (! res.second)
    {
      ERRORPRINTF (link->t, E_ERROR | 93, "registerLink: %d:%s: already present", link->pos,n);
      return false;
    }
  TRACEPRINTF (link->t, 3, "registerLink: %d:%s", link->pos,n);
  links_changed = true;
  if (transient)
    link->transient = true;
  if (want_up)
    {
      all_running = false;
      TRACEPRINTF (link->t, 3, "Start: %s", link->info(0));
      link->seq = seq;
      link->setState(L_going_up);
    }
  return true;
}

bool
Router::unregisterLink(const LinkConnectPtr& link)
{
  const std::string& n = link->name();
  auto res = links.find(link->pos);
  if (res == links.end())
    {
      ERRORPRINTF (link->t, E_ERROR | 94, "unregisterLink: %d:%s: not present", link->pos,n);
      return false;
    }
  links.erase(res);
  TRACEPRINTF (link->t, 3, "unregisterLink: %s", n);
  links_changed = true;
  if (!in_link_loop)
    state_trigger.send();
  return true;
}

bool
Router::hasAddress (eibaddr_t addr, LinkConnectPtr& link, bool quiet) const
{
  if (addr == this->addr)
    {
      if (!quiet)
        TRACEPRINTF (t, 8, "default addr %s", FormatEIBAddr (addr));
      return true;
    }

  if (link && link->hasAddress(addr))
    {
      if (!quiet)
        TRACEPRINTF (t, 8, "own addr %s", FormatEIBAddr (addr));
      return false;
    }

  C_ITER(i, links)
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
Router::checkAddress (eibaddr_t addr, LinkConnectPtr link) const
{
  if (addr == 0) // always accept broadcast
    return true;

  C_ITER(i, links)
  {
    if (i->second == link)
      continue;
    if (i->second->checkAddress (addr))
      return true;
  }

  return false;
}

bool
Router::checkGroupAddress (eibaddr_t addr, LinkConnectPtr link) const
{
  if (addr == 0) // always accept broadcast
    return true;

  C_ITER(i, links)
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
  int pos = addr - client_addrs_start;
  if (pos >= client_addrs_len)
    {
      ERRORPRINTF (t, E_ERROR | 95, "Release BAD2 %s", FormatEIBAddr (addr));
      return;
    }
  if (!client_addrs[pos])
    {
      ERRORPRINTF (t, E_ERROR | 96, "Release free addr %s", FormatEIBAddr (addr));
      return;
    }

  TRACEPRINTF (t, 3, "Release %s", FormatEIBAddr (addr));
  client_addrs[pos] = false;
}

void
Router::trigger_cb (ev::async &, int)
{
  while (!buf.empty() && low_send_more)
    {
      LDataPtr l1 = buf.get ();

      if (vbusmonitor.size())
        {
          LBusmonPtr l2 = LBusmonPtr(new L_Busmon_PDU ());
          l2->lpdu.set (L_Data_to_CM_TP1 (l1));

          ITER(i,vbusmonitor)
          i->cb->send_L_Busmonitor (LBusmonPtr(new L_Busmon_PDU (*l2)));
        }
      if (!l1->hop_count)
        {
          TRACEPRINTF (t, 3, "Hopcount zero: %s", l1->Decode (t));
          goto next;
        }
      if (l1->hop_count < 7 || !force_broadcast)
        l1->hop_count--;

      if (l1->repeated)
        {
          CArray d1 = L_Data_to_CM_TP1 (l1);
          ITER (i,ignore)
          if (d1 == i->data)
            {
              TRACEPRINTF (t, 9, "Drop: %s", l1->Decode (t));
              goto next;
            }
        }
      l1->repeated = 1;
      ignore.push_back((IgnoreInfo)
      {
        .data = L_Data_to_CM_TP1 (l1), .end = getTime () + 1000000
      });
      l1->repeated = 0;

      if (l1->address_type == IndividualAddress
          && l1->destination_address == this->addr)
        l1->destination_address = 0;

      low_send_more = false;
      r_low->send_L_Data(std::move(l1));
next:
      ;
    }

  if (!low_send_more)
    TRACEPRINTF (t, 6, "wait L");

  // Timestamps are ordered, so we scan for the first
  timestamp_t tm = getTime ();
  ITER (i, ignore)
  if (i->end >= tm)
    {
      ignore.erase (ignore.begin(), i);
      break;
    }
}

bool
Router::has_send_more(LinkConnectPtr i)
{
  if (i->send_more)
    return true;
  ERRORPRINTF (i->t, E_FATAL | 106, "internal error: send_more is not set");
  return false;
}

void
Router::send_L_Data(LDataPtr l1)
{
  assert(high_send_more);
  high_sending = true;
  high_send_more = false;

  auto source = l1->source;
  l1->source = nullptr;

  if (l1->address_type == GroupAddress)
    {
      // This is easy: send to all other L2 which subscribe to the
      // group.
      ITER(i, links)
      {
        auto ii = i->second;
        if (ii->state != L_up)
          continue;
        if ((l1->source_address == 0xFFFF) // programming
             ? &*ii == source
             : ii->hasAddress(l1->source_address))
          continue; // don't return to same interface
        if(!has_send_more(ii))
          continue; // internal error if not
        if (ii->checkGroupAddress(l1->destination_address))
          ii->send_L_Data (LDataPtr(new L_Data_PDU (*l1)));
      }
    }
  else if (l1->address_type == IndividualAddress)
    {
      // we want to send to the interface on which the destination address
      // has appeared. If it hasn't been seen yet, we send to all
      // interfaces.
      // Address ~0 is special; it's used for programming
      // so can be on different interfaces. Always broadcast these.
      bool found = (l1->destination_address == this->addr);
      if (l1->destination_address != 0xFFFF)
        ITER(i, links)
        {
          auto ii = i->second;
          if (ii->hasAddress (l1->source_address))
            continue;
          if (ii->hasAddress (l1->destination_address))
            {
              found = true;
              break;
            }
        }
      ITER (i, links)
      {
        auto ii = i->second;
        if (ii->state != L_up)
          continue;
        if (ii->hasAddress (l1->source_address))
          continue; // don't return to same interface
        if(!has_send_more(ii))
          continue; // internal error if not
        if (l1->hop_count == 7 || found ? ii->hasAddress (l1->destination_address) : ii->checkAddress (l1->destination_address))
          i->second->send_L_Data (LDataPtr(new L_Data_PDU (*l1)));
      }
    }
  high_sending = false;
  send_Next(); // check readiness
}

void
Router::mtrigger_cb (ev::async &, int)
{
  while (!mbuf.empty())
    {
      LBusmonPtr l1 = mbuf.get ();

      TRACEPRINTF (t, 3, "RecvMon %s", l1->Decode (t));
      ITER (i, busmonitor)
      i->cb->send_L_Busmonitor (LBusmonPtr(new L_Busmon_PDU (*l1)));
    }
}

bool
Router::checkStack(IniSectionPtr& cfg)
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
Router::hasClientAddrs(bool complain) const
{
  if (client_addrs_len > 0)
    return true;
  if (complain)
    ERRORPRINTF (t, E_ERROR | 51, "You need a client-addrs=X.Y.Z:N option in your %s section.", main);
  return false;

}

void
RouterHigh::started()
{
  if (is_started)
    return;
  is_started = true;
  Driver::started();
}

void
RouterHigh::stopped(bool err)
{
  is_started = false;
  Driver::stopped(err);
}

void
RouterHigh::recv_L_Data (LDataPtr l)
{
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Data(std::move(l));
}

void
RouterHigh::recv_L_Busmonitor (LBusmonPtr l)
{
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Busmonitor(std::move(l));
}

RouterHigh::RouterHigh(Router& r, const RouterLowPtr& rl)
  : Driver(std::dynamic_pointer_cast<LinkConnect_>(rl), r.ini[r.main]), router(&r)
{
  t->setAuxName("H");
}

RouterLow::RouterLow(Router& r)
  : LinkConnect_(r, r.ini[r.main], r.t), router(&r)
{
  t->setAuxName("L");
}

void
RouterLow::send_Next()
{
  router->low_send_more = true;
  TRACEPRINTF (t, 6, "OK L");
  router->trigger.send();
}
