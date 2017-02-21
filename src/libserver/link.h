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

#include "common.h"
#include "inifile.h"
#include "lpdu.h"

#include <memory>
#include <string>
#include <ostream>
#include <unordered_map>
#include <vector>

/*
 * This file contains class declarations for a link to the knxd router.
 * 
 * LinkBase: base class for all other modules. All LinkBase objects can
             (pretend to) sendn packets.
 * 
 * LinkRecv: a LinkBase that can also receive packets.
 *           i.e. anything that's not a driver.
 * 
 * LinkConnect: class that holds a reference to the driver stack;
 *              this is what Router talks to
 * 
 * LinkConnectClient: a LinkConnect that was created by a server's new connection
 * 
 * LinkConnectSingle: a LinkConnectClient guaranteed to only have one address
 * 
 * Driver: an interface to the outside world, created by configuration
 *
 * LineDriver: base class for the driver that's linked to a LinkConnectClient
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
 * Setup code must not depend on ->send being filled, much less already set up.
 *
 * Start code in filters should complete before calling send->start() if at all possible.
 *
 */

/** Helper class so that we don't need to bunch enerything into one header */
class BaseRouter;

/** some forward declarations */
class LinkBase;
typedef std::shared_ptr<LinkBase> LinkBasePtr;

class LinkConnect;
typedef std::shared_ptr<LinkConnect> LinkConnectPtr;

class LinkConnectClient;
typedef std::shared_ptr<LinkConnectClient> LinkConnectClientPtr;

class LinkConnectSingle;
typedef std::shared_ptr<LinkConnectSingle> LinkConnectSinglePtr;

class LinkRecv;
typedef std::shared_ptr<LinkRecv> LinkRecvPtr;

class LineDriver;
typedef std::shared_ptr<LineDriver> LineDriverPtr;

class Driver;
typedef std::shared_ptr<Driver> DriverPtr;

class BusDriver;
typedef std::shared_ptr<BusDriver> BusDriverPtr;

class Filter;
typedef std::shared_ptr<Filter> FilterPtr;

class Server;
typedef std::shared_ptr<Server> ServerPtr;

class BaseRouter {
protected: // can't instantiate this class directly
  BaseRouter(IniData &i) : ini(i) {}
public:
  virtual ~BaseRouter();
  /** debug output */
  TracePtr t;

  eibaddr_t addr = 0;

  /** our configuration data */
  IniData &ini;
};


template<class T, class I>
struct Maker {
  typedef typename I::first_arg I_first;
  static std::shared_ptr<I> create(I_first c, IniSection& s) { return std::shared_ptr<T>(new T(c,s)); }
};

template<class I>
class Factory {
public:
  typedef typename I::first_arg I_first;
  typedef std::shared_ptr<I> (*Creator)(I_first c, IniSection& s);
  typedef std::unordered_map<std::string, Creator> M;
  static M& map() { static M m; return m;}

  static Factory& Instance() {
      static Factory<I> f;
      return f;
  };

  template<class T>
  void
  reg(struct Maker<T,I> &m, const char* id)
  {
    map()[id] = &m.create;
  }

  std::shared_ptr<I>
  create(const std::string& id, I_first& c, IniSection& s) {
    typename M::iterator i = map().find(id);
    if (i == map().end())
      return nullptr;
    return i->second(c,s);
  }
};

template<class T, class I, const char * N>
struct RegisterClass
{
  typedef typename I::first_arg I_first;
  typedef std::shared_ptr<I> (*Creator)(I_first c, IniSection& s);

  RegisterClass()
  {
    Factory<I> f;

    static struct Maker<T,I> m;
    f.reg(m,N);
  }
};

template <class T, class I, const char * N>
struct AutoRegister
{
  AutoRegister() { (volatile void *) &ourRegisterer; } 
private:
  static RegisterClass<T, I, N> ourRegisterer;
};

template <class T, class I, const char * N>
RegisterClass<T,I,N> AutoRegister<T,I,N>::ourRegisterer;


class LinkBase : public std::enable_shared_from_this<LinkBase>
{
public:
  LinkBase(BaseRouter &r, IniSection& s, TracePtr tr);
  virtual ~LinkBase();
private:
  bool setup_called;
public:
  /** config data */
  IniSection &cfg;

  /** debug output */
  TracePtr t;

  /** dump info about me */
  virtual const std::string& name() { return cfg.name; }
  virtual std::string info(int verbose = 0); // debugging

  /** transmit a packet */
  virtual void send_L_Data (LDataPtr l) = 0;

  virtual bool setup() { setup_called = true; return true; }
  virtual void start() { assert(setup_called); }
  virtual void stop() {}

  virtual bool hasAddress (eibaddr_t addr) = 0;
  virtual void addAddress (eibaddr_t addr) = 0;
  virtual bool checkAddress (eibaddr_t addr) = 0;
  virtual bool checkGroupAddress (eibaddr_t addr) = 0;
  virtual bool _link(LinkRecvPtr prev) { return false; }
};

class LinkRecv : public LinkBase
{
public:
  LinkRecv(BaseRouter &r, IniSection& c, TracePtr tr) : LinkBase(r,c,tr) {}
  virtual ~LinkRecv();
  virtual void recv_L_Data (LDataPtr l) = 0;
  virtual void recv_L_Busmonitor (LBusmonPtr l) = 0;
  virtual eibaddr_t getMyAddr () = 0;

  virtual void started() = 0;
  virtual void stopped() = 0;

  virtual FilterPtr findFilter(std::string name) { return nullptr; }

protected:
  LinkBasePtr send = nullptr;
public:
  bool link(LinkBasePtr next)
    {
      assert(next);
      if(!next->_link(std::dynamic_pointer_cast<LinkRecv>(shared_from_this())))
        return false;
      assert(send == next); // _link_ was called
      return true;
    }
  void _link_(LinkBasePtr next) { send = next; }
  void unlink();
};

class LinkConnect : public LinkRecv
{
public:
  LinkConnect(BaseRouter& r, IniSection& s, TracePtr tr);
  virtual ~LinkConnect();
  bool running = false;
  bool switching = false;
  eibaddr_t addr = 0;

  BaseRouter& router;
private:
  DriverPtr driver;
  bool addr_local = true;

public:
  void set_driver(DriverPtr d)
    {
      driver = d;
      link(std::dynamic_pointer_cast<LinkBase>(d));
    }
  virtual bool setup();
  virtual void start();
  virtual void stop();

  /** Call before setup() to set a remotely-assigned address */
  virtual void setAddress(eibaddr_t addr);

  virtual void started();
  virtual void stopped();
  virtual void recv_L_Data (LDataPtr l); // { l3.recv_L_Data(std::move(l), this); }
  virtual eibaddr_t getMyAddr () { return router.addr; }
  virtual void recv_L_Busmonitor (LBusmonPtr l); // { l3.recv_L_Busmonitor(std::move(l), this); }

  virtual void send_L_Data (LDataPtr l) { send->send_L_Data(std::move(l)); }
  virtual bool checkAddress (eibaddr_t addr) { return send->checkAddress(addr); }
  virtual bool checkGroupAddress (eibaddr_t addr) { return send->checkGroupAddress(addr); }

  virtual bool hasAddress (eibaddr_t addr) { return send->hasAddress(addr); }
  virtual void addAddress (eibaddr_t addr) { send->addAddress(addr); }
};

/** connection for a server client */
class LinkConnectClient : public LinkConnect
{
  std::string linkname;

public:
  ServerPtr server;

  LinkConnectClient(ServerPtr s, IniSection& c, TracePtr tr);
  virtual ~LinkConnectClient();

  virtual const std::string& name() { return linkname; }
};

/** connection for a server client with a single address */
class LinkConnectSingle : public LinkConnectClient
{
public:
  LinkConnectSingle(ServerPtr s, IniSection& c, TracePtr tr) : LinkConnectClient(s,c,tr) {}
  virtual ~LinkConnectSingle();

  virtual bool setup();
  virtual bool hasAddress (eibaddr_t addr) { return addr == this->addr; }
  virtual void addAddress (eibaddr_t addr) { assert (addr == 0 || addr == this->addr); }
};

#ifdef NO_MAP
#define SERVER(_cls,_name) \
class _cls : public Server
#define SERVER_(_cls,_base,_name) \
class _cls : public _base

#else
#define SERVER(_cls,_name) \
static constexpr const char _cls##_name[] = #_name; \
class _cls; \
static AutoRegister<_cls,Server,_cls##_name> _auto_S##_name; \
class _cls : public Server

#define SERVER_(_cls,_base,_name) \
static constexpr const char _cls##_name[] = #_name; \
class _cls; \
static AutoRegister<_cls,Server,_cls##_name> _auto_S##_name; \
class _cls : public _base

#endif

class Server : public LinkConnect
{
public:
  typedef BaseRouter& first_arg;
  Server(BaseRouter& r, IniSection& c) : LinkConnect(r,c,r.t) {}
  virtual ~Server();

  /** does NOT call LinkConnect::setup because there is no driver here.
      TODO: move to router code, separate servers and links */
  virtual bool setup();
  virtual void start() { started(); }
  virtual void stop() { stopped(); }

  virtual void send_L_Data (LDataPtr l) {}
  virtual bool hasAddress (eibaddr_t addr) { return false; }
  virtual void addAddress (eibaddr_t addr) { ERRORPRINTF(t,E_ERROR|99,"Tried to add address %s to %s", FormatEIBAddr(addr), cfg.name); }
  virtual bool checkAddress (eibaddr_t addr) { return false; }
  virtual bool checkGroupAddress (eibaddr_t addr) { return false; }
};

#ifdef NO_MAP
#define FILTER(_cls,_name) \
class _cls : public Filter
#else
#define FILTER(_cls,_name) \
static constexpr const char _cls##_name[] = #_name; \
class _cls; \
static AutoRegister<_cls,Filter,_cls##_name> _auto_F##_name; \
class _cls : public Filter
#endif

class Filter : public LinkRecv
{
  friend class LinkConnect;
public:
  typedef LinkConnectPtr first_arg;

  Filter(LinkConnectPtr c, IniSection& s) : LinkRecv(c->router, s, c->t) { conn = c; }
  virtual ~Filter();

protected:
  std::weak_ptr<LinkRecv> recv;
  std::weak_ptr<LinkConnect> conn;
public:
  virtual void send_L_Data (LDataPtr l) { send->send_L_Data(std::move(l)); }
  virtual void recv_L_Data (LDataPtr l);
  virtual void recv_L_Busmonitor (LBusmonPtr l);
  virtual void started();
  virtual void stopped();

  virtual const std::string& name();

  bool _link(LinkRecvPtr prev)
    {
      if (prev == nullptr)
        return false;
      prev->_link_(shared_from_this());
      recv = prev; return true;
    }
  void unlink()
    {
      auto r = recv.lock();
      if (r != nullptr)
        {
          if (send != nullptr)
            send->_link(r);
          r->_link_(send);
        }
      send.reset();
      recv.reset();
    }

  virtual void start() { if (send == nullptr) stopped(); else send->start(); }
  virtual void stop() { if (send == nullptr) stopped(); else send->stop(); }

  virtual eibaddr_t getMyAddr ()
    {
      auto r = recv.lock();
      if (r == nullptr)
        return 0;
      return r->getMyAddr();
    }
  virtual bool hasAddress (eibaddr_t addr) { return send->hasAddress(addr); }
  virtual void addAddress (eibaddr_t addr) { return send->addAddress(addr); }
  virtual bool checkAddress (eibaddr_t addr) { return send->checkAddress(addr); }
  virtual bool checkGroupAddress (eibaddr_t addr) { return send->checkGroupAddress(addr); }

  virtual FilterPtr findFilter(std::string name);
};

#ifdef NO_MAP
#define DRIVER(_cls,_name) \
class _cls : public BusDriver
#define DRIVER_(_cls,_base,_name) \
class _cls : public _base

#else
#define DRIVER(_cls,_name) \
static constexpr const char _cls##_name[] = #_name; \
class _cls; \
static AutoRegister<_cls,Driver,_cls##_name> _auto_D_##_name; \
class _cls : public BusDriver

#define DRIVER_(_cls,_base,_name) \
static constexpr const char _cls##_name[] = #_name; \
class _cls; \
static AutoRegister<_cls,Driver,_cls##_name> _auto_D_##_name; \
class _cls : public _base
#endif

class Driver : public LinkBase
{
  friend class LinkConnect;
public:
  typedef LinkConnectPtr first_arg;

  Driver(LinkConnectPtr c, IniSection& s) : LinkBase(c->router, s, c->t) { conn = c; }
  virtual ~Driver();
  std::weak_ptr<LinkConnect> conn;

protected:
  std::weak_ptr<LinkRecv> recv;
public:
  virtual void recv_L_Data (LDataPtr l);
  virtual void recv_L_Busmonitor (LBusmonPtr l);
  virtual void started();
  virtual void stopped();

public:
  virtual void send_L_Data(LDataPtr l) = 0;
  virtual void start() { started(); }
  virtual void stop() { stopped(); }

  virtual bool _link(LinkRecvPtr prev)
    {
      if (prev == nullptr)
        return false;
      prev->_link_(shared_from_this());
      recv = prev;
      return true;
    }
  virtual bool link(LinkBasePtr next) { return false; }
  virtual void _link_(LinkBasePtr next) { assert(false); }
  virtual bool push_filter(FilterPtr filter);
  virtual FilterPtr findFilter(std::string name);
};

class BusDriver : public Driver
{
  std::vector<bool> addrs;

public:
  BusDriver(LinkConnectPtr c, IniSection& s) : Driver(c,s)
    {
      addrs.resize(65536);
    }
  virtual ~BusDriver();

  virtual bool hasAddress(eibaddr_t addr) { return addrs[addr]; }
  virtual void addAddress(eibaddr_t addr) { addrs[addr] = true; }
  virtual bool checkAddress (eibaddr_t addr) { return true; }
  virtual bool checkGroupAddress (eibaddr_t addr) { return true; }
};

/** Base class for server-linked drivers with busses behind them */
class SubDriver : public BusDriver
{
public:
  ServerPtr server;
  SubDriver(LinkConnectClientPtr c);
  virtual ~SubDriver();
};

/** Base class for server-linked drivers with a single client */
class LineDriver : public Driver
{
public:
  ServerPtr server;
  LineDriver(LinkConnectClientPtr c);
  virtual ~LineDriver();

  virtual bool setup(); // assigns the address
  eibaddr_t addr;

protected:
  virtual bool hasAddress(eibaddr_t addr) { return addr == this->addr; }
  virtual void addAddress(eibaddr_t addr) { this->addr = addr; }
  virtual bool checkAddress(eibaddr_t addr) { return addr == this->addr; }
  virtual bool checkGroupAddress (eibaddr_t addr) { return true; }
};

#endif
