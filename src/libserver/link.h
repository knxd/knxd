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

#ifndef DRIVER_BASE_H
#define DRIVER_BASE_H

#include "layer2common.h"
#include "common.h"
#include "inifile.h"
#include "lpdu.h"
#include <memory>

/*
 * This file contains class declarations for a link to the knxd router.
 * 
 * LinkBase: base class for all other modules. 
 * 
 * LinkRecv: a LinkBase that can also receive packets.
 *           i.e. anything that's not a driver.
 * 
 * LinkConnect: class that holds a reference to the driver stack;
 *              this is what Router talks to
 * 
 * Driver(LinkBase): an interface to the outside world
 *
 * LineDriver: a driver for a remote that typically has a single address,
 *             created by a Server
 *             an address gets assigned when connecting
 *             a sender address of zero gets replaced
 *
 * Filter(LinkRecv): able to modify packets
 * 
 * Server: creates new LinkConnect+Filter+Driver stacks on demand.
 * 
 * Setup: Router calls LinkConnect(*this, ini_section)
 * 
 *        LinkConnect looks up the drivers and filters, links them,
 *        calls setup(), and starts the stack. The driver is responsible
 *        for calling started() or stopped() on its receiver as it is ready
 *        (or not).
 *
 * Setup must not depend on 
 *
 */

class Router;

class LinkBase;
typedef std::shared_ptr<LinkBase> LinkBasePtr;

class LinkConnect;
typedef std::shared_ptr<LinkConnect> LinkConnectPtr;

class LinkRecv;
typedef std::shared_ptr<LinkRecv> LinkRecvPtr;

class Driver;
typedef std::shared_ptr<Driver> DriverPtr;

class LineDriver;
typedef std::shared_ptr<Driver> LineDriverPtr;

class Filter;
typedef std::shared_ptr<Filter> FilterPtr;

class Server;
typedef std::shared_ptr<Server> ServerPtr;;

class LinkBase : public std::enable_shared_from_this<LinkBase>
{
public:
  LinkBase(IniSection& s);
  virtual ~LinkBase();

protected:
  /** config data */
  IniSection &cfg;
  /** debug output */
  TracePtr t;

public:
  /** dump info about me */
  virtual void info(ostream& cs, int verbose = 0); // debugging

  /** transmit a packet */
  virtual void send_L_Data (LDataPtr l) = 0;

  virtual bool setup(bool monitor = false) { return !monitor; }
  virtual void start() = 0;
  virtual void stop() = 0;

  virtual bool hasAddress (eibaddr_t addr) = 0;
  virtual void addAddress (eibaddr_t addr) = 0;
  virtual bool checkAddress (eibaddr_t addr) = 0;
  virtual bool checkGroupAddress (eibaddr_t addr) = 0;
};

class LinkRecv : public LinkBase
{
  LinkRecv(IniSection& s) : LinkBase(s) {}
  virtual ~LinkRecv() {}
public:
  virtual void recv_L_Data (LDataPtr l) = 0;
  virtual void getMyAddr () = 0;

  virtual void started() = 0;
  virtual void stopped() = 0;
};

class LinkConnect : public LinkRecv
{
public:
  LinkConnect(Router& layer3, IniSection s, std::string& sn) : l3(layer3), section(sn), LinkRecv(s);
  virtual ~LinkConnect();
  bool setup(bool monitor = false);

private:
  Router& l3;
  std::string& section;

  LinkBasePtr send;
  DriverPtr driver;

public:
  void start();
  void stop();
  void started() { l3.link_started(this); }
  void stopped() { l3.link_stopped(this); }
  void recv_L_Data (LDataPtr l); // { l3.recv_L_Data(std::move(l), this); }
  void getMyAddr () { return l3.addr; }
  void recv_L_Busmonitor (LBusmonPtr l); // { l3.recv_L_Busmonitor(std::move(l), this); }

  bool hasAddress (eibaddr_t addr);
  void addAddress (eibaddr_t addr);
};

class Server : public LinkBase
{
  Server(Router& layer3, IniSection& s, std::string& sn) : l3(layer3), section(sn), LinkRecv(s);
  virtual ~Server() {}

private:
  Router& l3;
  std::string& section;

public:
  void send_L_Data (LDataPtr l) {}
  bool hasAddress (eibaddr_t addr) { return false; }
  void addAddress (eibaddr_t addr) { t->error("Tried to add address %s to %s", FormatEIBAddr(addr), cfg->name); }
  bool checkAddress (eibaddr_t addr) { return false; }
  bool checkGroupAddress (eibaddr_t addr) { return false; }
};

class Filter : public LinkRecv
{
  friend class LinkConnect;
public:
  Filter(LinkConnect& c, IniSection& s) : conn(c), LinkRecv(s) {}
  virtual ~Filter();
private:
  LinkConnect& conn;
  LinkBasePtr send;
  LinkRecvPtr recv;

public:
  virtual void recv_L_Data (LDataPtr l) { recv->recv_L_Data(std::move(l)); }
  virtual void recv_L_Busmonitor (LBusmonPtr l) { recv->recv_L_Busmonitor(std::move(l)); }

  virtual void send_L_Data (LDataPtr l) { send->send_L_Data(std::move(l)); }

  virtual void start() { send->start(); }
  virtual void stop() { send->stop(); }

  virtual void started() { recv->started(); }
  virtual void stopped() { recv->stopped(); }

  virtual bool checkAddress (eibaddr_t addr) { return send->addAddress(addr); }
  virtual bool checkGroupAddress (eibaddr_t addr) { return send->addGroupAddress(addr); }
};

class Driver : public LinkBase
{
  friend class LinkConnect;
public:
  Driver(LinkConnect& c, IniSection& s) : conn(c), LinkBase(s) {}
  virtual ~Driver();
private:
  LinkConnect& conn;
  LinkRecvPtr recv;

public:
  virtual void send_L_Data(LDataPtr l) = 0;
  virtual void start() { recv->started(); }
  virtual void stop() { recv->stopped(); }

  virtual bool checkAddress (eibaddr_t addr) { return true; }
  virtual bool checkGroupAddress (eibaddr_t addr) { return true; }
};

class LineDriver : public Driver
{
public:
  LineDriver(LinkConnect& c, IniSection& s) : Driver(c,s);
  virtual ~LineDriver();

  virtual bool setup(bool monitor = false); // assigns the address
protected:
  eibaddr_t addr;
};

#endif
