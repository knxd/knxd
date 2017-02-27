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
#include "router.h"
#include <stdio.h>

LinkBase::~LinkBase() { }
LinkRecv::~LinkRecv() { }
Driver::~Driver() { }
BusDriver::~BusDriver() { }
SubDriver::~SubDriver() { }
LineDriver::~LineDriver() { }
LinkConnect_::~LinkConnect_() { }
Filter::~Filter() { }
Server::~Server() { }
BaseRouter::~BaseRouter() { }
LinkConnectClient::~LinkConnectClient() { }
LinkConnectSingle::~LinkConnectSingle() { }

LinkConnect::~LinkConnect()
{
  retry_timer.stop();

  if (addr && addr_local)
    static_cast<Router &>(router).release_client_addr(addr);
}

LinkBase::LinkBase(BaseRouter& r, IniSection& s, TracePtr tr) : cfg(s)
{
  Router& rt = dynamic_cast<Router&>(r);
  t = TracePtr(new Trace(*tr, s));
}

std::string
LinkBase::info(int level)
{
  std::string res = "LinkBase: cfg:";
  res += cfg.name;
  return res;
}

LinkConnect_::LinkConnect_(BaseRouter& r, IniSection& c, TracePtr tr)
   : router(r), LinkRecv(r,c,tr)
{
  //Router& rt = dynamic_cast<Router&>(r);
}

LinkConnect::LinkConnect(BaseRouter& r, IniSection& c, TracePtr tr)
   : LinkConnect_(r,c,tr)
{
  //Router& rt = dynamic_cast<Router&>(r);
  retry_timer.set <LinkConnect,&LinkConnect::retry_timer_cb> (this);
}

void
LinkConnect::retry_timer_cb (ev::timer &w, int revents)
{
  retries++;
  start();
}

void
LinkConnect::setAddress(eibaddr_t addr)
{
  this->addr = addr;
  this->addr_local = false;
}
bool
LinkConnectSingle::setup()
{
  if (!LinkConnectClient::setup())
    return false;
  if (addr == 0)
    addr = static_cast<Router &>(router).get_client_addr(t);
  if (addr == 0)
    return false;
  return true;
}

void
LinkConnect::start()
{
  if (running || switching)
    return;
  TRACEPRINTF(t, 5, "Starting");
  running = false;
  switching = true;
  need_send_more = false;
  send_more = true;
  changed = time(NULL);
  LinkConnect_::start();
}

void
LinkConnect_::start()
{
  LinkRecv::start();
  send->start();
}

void
LinkConnect::stop()
{
  retry_timer.stop();
  if (running && !switching)
    retries = 0;

  if (!running || switching)
    return;
  TRACEPRINTF(t, 5, "Stopping");
  switching = true;
  changed = time(NULL);
  LinkConnect_::stop();
}

void
LinkConnect_::stop()
{
  send->stop();
  LinkRecv::stop();
}

const std::string&
Filter::name()
{
  return cfg.value("filter",cfg.name);
}

const std::string&
Driver::name()
{
  return cfg.value("driver",cfg.name);
}

FilterPtr
Filter::findFilter(std::string name)
{
  auto r = recv.lock();
  if (r == nullptr)
    return nullptr;
  if (this->name() == name)
    return std::static_pointer_cast<Filter>(shared_from_this());
  return r->findFilter(name);
}

FilterPtr
Driver::findFilter(std::string name)
{
  auto r = recv.lock();
  if (r == nullptr)
    return nullptr;
  return r->findFilter(name);
}

bool
LineDriver::setup()
{
  if(!Driver::setup())
    return false;

  auto c = std::dynamic_pointer_cast<LinkConnect>(conn.lock());
  if (c == nullptr)
    return false;

  _addr = c->addr;
  return true;
}

bool
LinkConnect::setup()
{
  if (!LinkConnect_::setup())
    return false;

  ignore = cfg.value("ignore",false);
  may_fail = cfg.value("may-fail",false);
  retry_delay = cfg.value("retry",0);
  return true;
}

bool
LinkConnect_::setup()
{
  if (!LinkRecv::setup())
    return false;
  DriverPtr dr = driver; // .lock();
  if(dr == nullptr)
    {
      ERRORPRINTF (t, E_ERROR | 55, "No driver in %s. Refusing.", cfg.name);
      return false;
    }

  std::string x = cfg.value("filters","");
  {
    size_t pos = 0;
    size_t comma = 0;
    while(true)
      {
        comma = x.find(',',pos);
        std::string name = x.substr(pos,comma-pos);
        if (name.size())
          {
            FilterPtr link;
            IniSection& s = static_cast<Router&>(router).ini[name];
            name = s.value("filter",name);
            link = static_cast<Router&>(router).get_filter(std::dynamic_pointer_cast<LinkConnect_>(shared_from_this()),
                  s, name);
            if (link == nullptr)
              {
                ERRORPRINTF (t, E_ERROR | 32, "filter '%s' not found.", name);
                return false;
              }
            if(!dr->push_filter(link))
              {
                ERRORPRINTF (t, E_ERROR | 32, "Linking filter '%s' failed.", name);
                return false;
              }
          }
        if (comma == std::string::npos)
          break;
        pos = comma+1;
      }
  }
  
  LinkBasePtr s = send;
  while (s != nullptr)
    {
      if (!s->setup())
        {
          ERRORPRINTF (t, E_ERROR | 32, "%s: setup %s: failed", cfg.name, s->cfg.name);
          return false;
        }
      if (s == dr)
        break;
      auto ps = std::dynamic_pointer_cast<Filter>(s);
      if (ps == nullptr)
        {
          ERRORPRINTF (t, E_FATAL | 32, "%s: setup %s: no driver", cfg.name, s->cfg.name);
          return false;
        }
      s = ps->send;
    }
  if (s == nullptr)
    {
      ERRORPRINTF (t, E_FATAL | 33, "%s: setup: no driver", cfg.name);
      return false;
    }
  return true;
}

void
LinkConnect::started()
{
  running = true;
  switching = false;
  changed = time(NULL);
  TRACEPRINTF(t, 5, "Started");
  static_cast<Router&>(router).link_started(std::dynamic_pointer_cast<LinkConnect>(shared_from_this()));
}

void
LinkConnect::send_Next()
{
  need_send_more = true;
  send_more = true;
  static_cast<Router&>(router).send_Next();
}

void
LinkConnect::stopped()
{
  if (running != switching && retry_delay)
    retry_timer.start(retry_delay,0);
  running = false;
  switching = false;
  changed = time(NULL);
  TRACEPRINTF(t, 5, "Stopped");
  static_cast<Router&>(router).link_stopped(std::dynamic_pointer_cast<LinkConnect>(shared_from_this()));
}

void
LinkConnect::recv_L_Data (LDataPtr l)
{
  static_cast<Router&>(router).recv_L_Data(std::move(l), *this);
}

void
LinkConnect::recv_L_Busmonitor (LBusmonPtr l)
{
  static_cast<Router&>(router).recv_L_Busmonitor(std::move(l));
}

bool
Server::setup()
{
  return true;
}

LinkConnectClient::LinkConnectClient(ServerPtr s, IniSection& c, TracePtr tr)
  : server(s), LinkConnect(s->router, c, tr)
{
  char n[10];
  sprintf(n,"%d",t->seq);
  linkname = t->name + '_' + n;
}

SubDriver::SubDriver(const LinkConnectClientPtr& c)
      : BusDriver(static_cast<const LinkConnectPtr&>(c), c->cfg)
{
  server = c->server;
}

LineDriver::LineDriver(const LinkConnectClientPtr& c)
      : Driver(c, c->cfg)
{
  server = c->server;
}

void
Driver::send_Next()
{
  auto r = recv.lock();
  if (r != nullptr)
    r->send_Next();
}

void
Driver::recv_L_Data (LDataPtr l)
{
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Data(std::move(l));
}

void
Filter::send_Next()
{
  auto r = recv.lock();
  if (r != nullptr)
    r->send_Next();
}

void
Filter::recv_L_Data (LDataPtr l)
{
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Data(std::move(l));
}

void
Driver::recv_L_Busmonitor (LBusmonPtr l)
{
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Busmonitor(std::move(l)); 
}

void
Filter::recv_L_Busmonitor (LBusmonPtr l)
{
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Busmonitor(std::move(l)); 
}

void
Driver::started()
{
  auto r = recv.lock();
  if (r != nullptr)
    r->started();
}

void
Filter::started()
{
  auto r = recv.lock();
  if (r != nullptr)
    r->started();
}

void
Driver::stopped()
{
  auto r = recv.lock();
  if (r != nullptr)
    r->stopped();
}

void
Filter::stopped()
{
  auto r = recv.lock();
  if (r != nullptr)
    r->stopped();
}

bool
Driver::push_filter(FilterPtr filter)
{
  auto r = recv.lock();
  if (r == nullptr)
    return false;

  if (!r->link(filter))
    return false;
  if (!filter->link(shared_from_this()))
    {
      r->link(shared_from_this());
      return false;
    }

#if 0 // this is done by LinkConnect::setup() once the stack is complete
  if (!filter->setup())
    {
      filter->unlink();
      return false;
    }
#endif
  return true;
}

Filter::Filter(const LinkConnectPtr_& c, IniSection& s)
    : LinkRecv(c->router, s, c->t)
{
  conn = c;
  t->setName(name() + '@' + c->t->name);
}

