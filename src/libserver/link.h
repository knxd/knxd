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
 * Driver: an interface to the outside world, created by configuration
 *
 * LineDriver: a driver created by a Server when a client connects to knxd
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

class BaseRouter;

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
typedef std::shared_ptr<Server> ServerPtr;

class BaseRouter {
public:
  BaseRouter(IniData &i) : ini(i) {}
  virtual ~BaseRouter();

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
    fprintf(stderr,"REG %s\n",id);
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
    // fprintf(stderr,"REG1 %s\n",N);
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
  LinkBase(BaseRouter &r, IniSection& s);
  virtual ~LinkBase();

protected:
  /** config data */
  IniSection &cfg;

public:
  /** debug output */
  TracePtr t;

  /** dump info about me */
  virtual const std::string& name() { return cfg.name; }
  virtual void info(std::ostream& cs, int verbose = 0); // debugging

  /** transmit a packet */
  virtual void send_L_Data (LDataPtr l) = 0;

  virtual bool setup() { return true; }
  virtual void start() = 0;
  virtual void stop() = 0;

  virtual bool hasAddress (eibaddr_t addr) = 0;
  virtual void addAddress (eibaddr_t addr) = 0;
  virtual bool checkAddress (eibaddr_t addr) = 0;
  virtual bool checkGroupAddress (eibaddr_t addr) = 0;
  virtual bool _link(LinkRecvPtr prev) { return false; }
};

class LinkRecv : public LinkBase
{
public:
  LinkRecv(BaseRouter &r, IniSection& s) : LinkBase(r,s) {}
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
  LinkConnect(BaseRouter& r, IniSection& s);
  virtual ~LinkConnect();
  bool running = false;
  bool switching = false;
  eibaddr_t addr = 0;

  BaseRouter& router;
private:
  std::weak_ptr<Driver> driver;

public:
  void set_driver(DriverPtr d) { driver = d; }
  bool setup();
  void start();
  void stop();

  void started();
  void stopped();
  void recv_L_Data (LDataPtr l); // { l3.recv_L_Data(std::move(l), this); }
  eibaddr_t getMyAddr () { return router.addr; }
  void recv_L_Busmonitor (LBusmonPtr l); // { l3.recv_L_Busmonitor(std::move(l), this); }

  void send_L_Data (LDataPtr l) { send->send_L_Data(std::move(l)); }
  bool checkAddress (eibaddr_t addr) { return send->checkAddress(addr); }
  bool checkGroupAddress (eibaddr_t addr) { return send->checkGroupAddress(addr); }

  virtual bool hasAddress (eibaddr_t addr) { return send->hasAddress(addr); }
  virtual void addAddress (eibaddr_t addr) { send->addAddress(addr); }
};

/** connection for a single client */
class LinkConnectClient : public LinkConnect
{
public:
  LinkConnectClient(ServerPtr s);
  ~LinkConnectClient();
  ServerPtr server;

  bool hasAddress (eibaddr_t addr) { return addr == this->addr; }
  void addAddress (eibaddr_t addr) {}
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
  IniSection& client_section;

  typedef BaseRouter& first_arg;
  Server(BaseRouter& r, IniSection& s);
  LinkConnectPtr new_link();
  virtual ~Server();

  void send_L_Data (LDataPtr l) {}
  bool hasAddress (eibaddr_t addr) { return false; }
  void addAddress (eibaddr_t addr) { ERRORPRINTF(t,E_ERROR|99,"Tried to add address %s to %s", FormatEIBAddr(addr), cfg.name); }
  bool checkAddress (eibaddr_t addr) { return false; }
  bool checkGroupAddress (eibaddr_t addr) { return false; }
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

  Filter(LinkConnectPtr c, IniSection& s) : conn(c), LinkRecv(c->router, s) {}
  virtual ~Filter();
protected:
  LinkConnectPtr conn;
private:
  LinkBasePtr send;
  LinkRecvPtr recv;

public:
  virtual const std::string& name();

  bool _link(LinkRecvPtr prev)
    {
      if (prev == nullptr)
        return false;
      prev->_link_(shared_from_this());
      recv = prev; return true;
    }


  void unlink() { send->_link(recv); send = nullptr; recv = nullptr; }
  virtual void recv_L_Data (LDataPtr l) { recv->recv_L_Data(std::move(l)); }
  virtual void recv_L_Busmonitor (LBusmonPtr l) { recv->recv_L_Busmonitor(std::move(l)); }

  virtual void send_L_Data (LDataPtr l) { send->send_L_Data(std::move(l)); }

  virtual void start() { send->start(); }
  virtual void stop() { send->stop(); }

  virtual void started() { recv->started(); }
  virtual void stopped() { recv->stopped(); }

  virtual eibaddr_t getMyAddr () { return recv->getMyAddr(); }
  virtual bool hasAddress (eibaddr_t addr) { return send->hasAddress(addr); }
  virtual void addAddress (eibaddr_t addr) { return send->addAddress(addr); }
  virtual bool checkAddress (eibaddr_t addr) { return send->checkAddress(addr); }
  virtual bool checkGroupAddress (eibaddr_t addr) { return send->checkGroupAddress(addr); }

  virtual FilterPtr findFilter(std::string name);
};

#ifdef NO_MAP
#define DRIVER(_cls,_name) \
class _cls : public Driver
#define DRIVER_(_cls,_base,_name) \
class _cls : public _base

#else
#define DRIVER(_cls,_name) \
static constexpr const char _cls##_name[] = #_name; \
class _cls; \
static AutoRegister<_cls,Driver,_cls##_name> _auto_D_##_name; \
class _cls : public Driver

#define DRIVER_(_cls,_base,_name) \
static constexpr const char _cls##_name[] = #_name; \
class _cls; \
static AutoRegister<_cls,Driver,_cls##_name> _auto_D_##_name; \
class _cls : public _base
#endif

class Driver : public LinkBase
{
  friend class LinkConnect;
  std::vector<bool> addrs;
public:
  typedef LinkConnectPtr first_arg;

  Driver(LinkConnectPtr c, IniSection& s) : conn(c), LinkBase(c->router, s)
  { addrs.resize(65536); }
  virtual ~Driver();
  LinkConnectPtr conn;
protected:
  LinkRecvPtr recv;

public:
  virtual void recv_L_Data (LDataPtr l) { recv->recv_L_Data(std::move(l)); }
  virtual void recv_L_Busmonitor (LBusmonPtr l) { recv->recv_L_Busmonitor(std::move(l)); }

  virtual void send_L_Data(LDataPtr l) = 0;
  virtual void start() { started(); }
  virtual void stop() { stopped(); }

  void started() { recv->started(); }
  void stopped() { recv->stopped(); }
  bool _link(LinkRecvPtr prev)
    {
      if (prev == nullptr)
        return false;
      prev->_link_(shared_from_this());
      recv = prev;
      return true;
    }
  bool link(LinkBasePtr next) { return false; }
  void _link_(LinkBasePtr next) { assert(false); }
  bool push_filter(FilterPtr filter)
    {
      if (!recv->link(filter))
        return false;
      if (!filter->link(shared_from_this()))
        {
          recv->link(shared_from_this());
          return false;
        }

      if (!filter->setup())
        {
          filter->unlink();
          return false;
        }
      return true;
    }

  virtual bool hasAddress(eibaddr_t addr) { return addrs[addr]; }
  virtual void addAddress(eibaddr_t addr) { addrs[addr] = true; }
  virtual bool checkAddress (eibaddr_t addr) { return true; }
  virtual bool checkGroupAddress (eibaddr_t addr) { return true; }
};

class LineDriver : public Driver
{
public:
  ServerPtr server;
  LineDriver(ServerPtr s)
      : Driver(s->new_link(), s->client_section)
    { server = s; }
  virtual ~LineDriver();

  virtual bool setup(); // assigns the address
  eibaddr_t addr;

protected:
  virtual bool hasAddress(eibaddr_t addr) { return addr == this->addr; }
  virtual void addAddress(eibaddr_t addr) { }
};

#endif
