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

LinkBase::~LinkBase() { }
LinkRecv::~LinkRecv() { }
LinkConnect::~LinkConnect() { }
Driver::~Driver() { }
BusDriver::~BusDriver() { }
SubDriver::~SubDriver() { }
LineDriver::~LineDriver() { }
Filter::~Filter() { }
Server::~Server() { }
BaseRouter::~BaseRouter() { }

LinkBase::LinkBase(BaseRouter& r, IniSection& s) : cfg(s)
{
  Router& rt = dynamic_cast<Router&>(r);
  t = TracePtr(new Trace(s, rt.servername));
}

std::string
LinkBase::info(int level)
{
  std::string res = "LinkBase: cfg:";
  res += cfg.name;
  return res;
}

LinkConnect::LinkConnect(BaseRouter& r, IniSection& c)
   : router(r), LinkRecv(r,c)
{
  //Router& rt = dynamic_cast<Router&>(r);
}

LinkConnectSingle::~LinkConnectSingle()
{
  if (addr)
    static_cast<Router &>(router).release_client_addr(addr);
}

bool
LinkConnectSingle::setup()
{
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
  running = false;
  switching = true;
  send->start();
}

void
LinkConnect::stop()
{
  if (running && switching)
    return;
  if (!running && !switching)
    return;
  switching = true;
  send->stop();
}

const std::string&
Filter::name()
{ 
  return cfg.value("filter",cfg.name);
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
  auto c = conn.lock();
  if (c == nullptr)
    return false;

  addr = static_cast<Router &>(c->router).get_client_addr(t);
    // .c->set_driver(std::static_pointer_cast<Driver>(shared_from_this()));
  return true;
}

bool
LinkConnect::setup()
{
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
            link = static_cast<Router&>(router).get_filter(std::dynamic_pointer_cast<LinkConnect>(shared_from_this()),
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
  
  return true;
}

void
LinkConnect::started()
{
  running = true;
  switching = false;
  static_cast<Router&>(router).link_started(std::dynamic_pointer_cast<LinkConnect>(shared_from_this()));
}

void
LinkConnect::stopped()
{
  running = false;
  switching = false;
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

LinkConnectClient::LinkConnectClient(ServerPtr s, IniSection& c)
  : LinkConnect(s->router, c)
{}

LinkConnectClient::~LinkConnectClient()
{}

SubDriver::SubDriver(LinkConnectClientPtr c)
      : BusDriver(c->server, c->cfg)
{
  server = c->server;
}

LineDriver::LineDriver(LinkConnectClientPtr c)
      : Driver(c->server, c->cfg)
{
  server = c->server;
}

void
Driver::recv_L_Data (LDataPtr l)
{
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Data(std::move(l));
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

  if (!filter->setup())
    {
      filter->unlink();
      return false;
    }
  return true;
}

