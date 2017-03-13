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
 *           accept packets to send.
 * 
 * LinkRecv: a LinkBase that can also accept received packets.
 *           i.e. anything that's not a driver (those get their packets
 *           from somewhere else).
 * 
 * LinkConnect: class that holds a reference to the driver stack;
 *              this is what the Router talks to
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
 *         It is a LinkConnect subclass by convenience, because
 *         that way the router only needs one loop.
 * 
 *
 * Calling sequence:
 *
 * Setup: Router calls LinkConnect(*this, ini_section)
 * 
 *        LinkConnect looks up the drivers and filters, links them,
 *        and calls setup() on each member.
 *        setup() is synchronous. Do not connect to external servers here.
 * 
 * Start: Filters call start()/stop() on the next element of the chain.
 *        The driver is responsible for calling started() or stopped().
 *        That call propagates down the stack, LinkConnect forwards to
 *        the router.
 *
 *        Starting is asynchronous but must complete eventually.
 *        Stopping may be asynchronous if necessary, but try not to.
 *
 * Receiving data: the driver calls recv_L_Data() which gets forwarded 
 *                 through the filters to LinkConnect, via the .recv
 *                 pointer.
 *
 * Sending data: LinkConnect::send_L_Data() calls the first filter, which
 *               forwards to tne driver, via the .send pointer.
 * 
 *
 * Pointers from LinkConnect towards the driver, along the .send chain,
 * are shared pointers. All pointers towards LinkConnect, along the .recv
 * chain, are weak pointers.
 */

/** Helper class so that we don't need to bunch enerything into one header */
class BaseRouter;

/** some forward declarations */
class LinkBase;
typedef std::shared_ptr<LinkBase> LinkBasePtr;

class LinkConnect_;
typedef std::shared_ptr<LinkConnect_> LinkConnectPtr_;

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
  static I* create(const I_first &c, IniSection& s)
    {
      return new T(c,s);
    }
};

template<class I>
class Factory {
public:
  typedef typename I::first_arg I_first;
  typedef I* (*Creator)(const I_first &c, IniSection& s);
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

  I *
  create(const std::string& id, const I_first& c, IniSection& s) {
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


/** The start of the object hierarchy is something that can send
 * packets, which implies checking whether a packet _can_ be sent.
 */
class LinkBase : public std::enable_shared_from_this<LinkBase>
{
public:
  LinkBase(BaseRouter &r, IniSection& s, TracePtr tr);
  virtual ~LinkBase();
private:
  /* DEBUG: Flag to make sure that the call sequence is observed */
  bool setup_called = false;
  //volatile char *setup_foo; // see setup()
public:
  /** config data */
  IniSection &cfg;

  /** debug output */
  TracePtr t;

  /** This thing's name; drivers/filters override this with their "real" name */
  virtual const std::string& name() { return cfg.name; }
  /** dump info about me */
  virtual std::string info(int verbose = 0); // debugging

  /** transmit a packet */
  virtual void send_L_Data (LDataPtr l) = 0;

  /** Parse configuration; return False if anything's wrong */
  virtual bool setup()
    {
#if 0
      // Use this code if you want to (ab)use valgrind for tracking which
      // code path called setup the first time.
      if (setup_called)
        { // make sure that this doesn't break anything
          char x = *setup_foo;
          *setup_foo = 1;
          *setup_foo = x;
        }
      else
        {
          setup_foo = (char *)malloc(1);
          free((void *)setup_foo);
        }
#endif
      assert (!setup_called);
      setup_called = true;
      return true;
    }
  /** Start up. Ultimately calls started() or stopped() */
  virtual void start() { assert(setup_called); }
  /** Shut down. Ultimately calls stopped() */
  virtual void stop() {}

  /** Note that this link has started */
  virtual void started() = 0;
  /** Note that this link has stopped */
  virtual void stopped() = 0;

  /** Check whether this physical address has been seen on this link */
  virtual bool hasAddress (eibaddr_t addr) = 0;
  /** Remember that this physical address has been seen on this link */
  virtual void addAddress (eibaddr_t addr) = 0;
  /** Check whether this physical address may appear on this link */
  virtual bool checkAddress (eibaddr_t addr) = 0;
  /** Check whether this group address may appear on this link */
  virtual bool checkGroupAddress (eibaddr_t addr) = 0;

  /* link() calls _link() which calls _link_(). See there. */
  virtual bool _link(LinkRecvPtr prev) { return false; }
};


/** The next level is something that can receive packets, i.e. everything
 * that's not a driver.
 */
class LinkRecv : public LinkBase
{
public:
  LinkRecv(BaseRouter &r, IniSection& c, TracePtr tr) : LinkBase(r,c,tr)
    {
      t->setAuxName("Recv");
    }
  virtual ~LinkRecv();
  /** Arriving data packet */
  virtual void recv_L_Data (LDataPtr l) = 0;
  /** Arriving monitor packet */
  virtual void recv_L_Busmonitor (LBusmonPtr l) = 0;
  /** packet buffer is empty */
  virtual void send_Next () = 0;

  /** Call for drivers to find a filter, if it exists */
  virtual FilterPtr findFilter(std::string name, bool skip_me = false) { return nullptr; }

  /** The thing to send data to. */
  LinkBasePtr send = nullptr;

  /** Attach the next (i.e. sending) link to me */
  bool link(LinkBasePtr next)
    {
      assert(next);
      if(!next->_link(std::dynamic_pointer_cast<LinkRecv>(shared_from_this())))
        return false;
      assert(send == next); // _link_ was called
      return true;
    }
  void _link_(LinkBasePtr next) { send = next; }
  /** remove this object from the chain */
  virtual void unlink() = 0;
};


/** This is the base class for LinkConnect, the bottom node of a filter stack.
 * This class separates the parts that are used in the global filter chain.
 */
class LinkConnect_ : public LinkRecv
{
public:
  LinkConnect_(BaseRouter& r, IniSection& s, TracePtr tr);
  virtual ~LinkConnect_();

  BaseRouter& router;

private:
  DriverPtr driver;
public:
  virtual bool setup();
  virtual void start();
  virtual void stop();

  /** Link up a driver. Can't be in ctor because of the shared pointer. */
  void set_driver(DriverPtr d)
    {
      driver = d;
      link(std::dynamic_pointer_cast<LinkBase>(d));
    }

  /** You can't unlink the root of the chain … */
  virtual void unlink() { assert(false); }

  virtual void send_L_Data (LDataPtr l) { send->send_L_Data(std::move(l)); }
  virtual bool checkAddress (eibaddr_t addr) { return send->checkAddress(addr); }
  virtual bool checkGroupAddress (eibaddr_t addr) { return send->checkGroupAddress(addr); }
  virtual bool hasAddress (eibaddr_t addr) { return send->hasAddress(addr); }
  virtual void addAddress (eibaddr_t addr) { send->addAddress(addr); }
};

/** A LinkConnect is something which the router knows about.
 * For non-servers, it holds a pointer to the driver and to the bottom of
 * the filter stack.
 * This contains the parts useable on a per-link filter chain.
 */
class LinkConnect : public LinkConnect_
{
public:
  LinkConnect(BaseRouter& r, IniSection& s, TracePtr tr);
  virtual ~LinkConnect();
  /** Don't auto-start */
  bool ignore = false;
  /** Ignore startup failures */
  bool may_fail = false;
  /** address assigned to this link */
  eibaddr_t addr = 0;

  /** Driver/Server is up */
  bool running = false;
  /** Driver/Server intends to be what @running says  */
  bool switching = false;

  /** loop counter for the router */
  int seq = 0;
  /** last state change */
  time_t changed = 0;
  /** retry timer */
  int retry_delay = 0;
  /** how often …? */
  int retries = 0;

private:
  ev::timer retry_timer;
  void retry_timer_cb(ev::timer &w, int revents);

  bool addr_local = true;

  /* Flow control. need_send_more is true if the driver ever called send_more(). */
  /* send_more will only be cleared when need_send_more is true. */
  bool need_send_more = false;
public:
  bool send_more = true;
  virtual void send_L_Data (LDataPtr l)
    {
      LinkConnect_::send_L_Data(std::move(l));
    }

  /** This is responsible for setting up the filters. Don't call it twice!
   * Precondition: set_driver() has been called. */
  virtual bool setup();
  virtual void start();
  virtual void stop();

  /** set this link's remotely-assigned address */
  virtual void setAddress(eibaddr_t addr);

  virtual void started();
  virtual void stopped();
  virtual void recv_L_Data (LDataPtr l); // { l3.recv_L_Data(std::move(l), this); }
  virtual void recv_L_Busmonitor (LBusmonPtr l); // { l3.recv_L_Busmonitor(std::move(l), this); }
  virtual void send_Next ();
};

/** connection for a server's client */
class LinkConnectClient : public LinkConnect
{
  /** Some unique identifier */
  std::string linkname;

public:
  ServerPtr server;

  LinkConnectClient(ServerPtr s, IniSection& c, TracePtr tr);
  virtual ~LinkConnectClient();

  virtual const std::string& name() { return linkname; }
};

/** connection for a server's client with a single address */
class LinkConnectSingle : public LinkConnectClient
{
public:
  LinkConnectSingle(ServerPtr s, IniSection& c, TracePtr tr) : LinkConnectClient(s,c,tr)
    {
      t->setAuxName("ConnS");
    }
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


/** A server is a LinkConnect which doesn't itself send packets.
 * Instead it creates other LinkConnect objects on demand, when a client
 * connects to it.
 */
class Server : public LinkConnect
{
public:
  typedef BaseRouter& first_arg;
  Server(BaseRouter& r, IniSection& c) : LinkConnect(r,c,r.t)
    {
      t->setAuxName("Server");
    }
  virtual ~Server();

  /** Server::setup() does NOT call LinkConnect::setup() because there is no driver here. */
  virtual bool setup();
  virtual void start() { started(); }
  virtual void stop() { stopped(); }

  /** Servers don't accept data */
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


/** filters are inserted between a link's LinkConnect object and the actual
 * driver. They may modify, log, … the data flowing through them. */
class Filter : public LinkRecv
{
  friend class LinkConnect;
public:
  typedef LinkConnectPtr_ first_arg;

  Filter(const LinkConnectPtr_& c, IniSection& s);
  virtual ~Filter();

protected:
  /** Link to the receiver */
  std::weak_ptr<LinkRecv> recv;
  /** Link to the LinkConnect object holding the stack this filter is in */
  std::weak_ptr<LinkConnect_> conn;
public:
  virtual void send_L_Data (LDataPtr l) { send->send_L_Data(std::move(l)); }
  virtual void recv_L_Data (LDataPtr l); // recv->recv_L_Data(std::move(l));
  virtual void recv_L_Busmonitor (LBusmonPtr l); // recv->recv_L_Busmonitor(std::move(l));
  virtual void send_Next ();
  virtual void started(); // recv->started()
  virtual void stopped(); // recv->stopped()

  /** Returns the filter's name, i.e. the config's filter= value */
  virtual const std::string& name();

  bool _link(LinkRecvPtr prev)
    {
      if (prev == nullptr)
        return false;
      prev->_link_(shared_from_this());
      recv = prev; return true;
    }

  /** Remove this filter from the link chain */
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

  virtual bool hasAddress (eibaddr_t addr) { return send->hasAddress(addr); }
  virtual void addAddress (eibaddr_t addr) { return send->addAddress(addr); }
  virtual bool checkAddress (eibaddr_t addr) { return send->checkAddress(addr); }
  virtual bool checkGroupAddress (eibaddr_t addr) { return send->checkGroupAddress(addr); }

  virtual FilterPtr findFilter(std::string name, bool skip_me = false);
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
  typedef LinkConnectPtr_ first_arg;
  /** Returns the driver's name, i.e. the config's driver= value */
  virtual const std::string& name();

  Driver(const LinkConnectPtr_& c, IniSection& s) : LinkBase(c->router, s, c->t)
    {
      conn = c;
    }
  virtual ~Driver();
  std::weak_ptr<LinkConnect_> conn;

protected:
  std::weak_ptr<LinkRecv> recv;
public:
  virtual void recv_L_Data (LDataPtr l);
  virtual void recv_L_Busmonitor (LBusmonPtr l);
  virtual void send_Next ();
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
  /** This is the end of the link, so nothing to link to! */
  virtual bool link(LinkBasePtr next) { return false; }
  virtual void _link_(LinkBasePtr next) { assert(false); }

  /** Add a filter just below this driver.
   * The caller is responsible for calling .setup()!
   * Don't call after being started! */
  virtual bool push_filter(FilterPtr filter);

  /** Find a filter below me.
   * This checks the filter= value, not the section. */
  virtual FilterPtr findFilter(std::string name, bool skip_me = false);
};

class BusDriver : public Driver
{
  std::vector<bool> addrs;

public:
  BusDriver(const LinkConnectPtr_& c, IniSection& s) : Driver(c,s)
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
  SubDriver(const LinkConnectClientPtr& c);
  virtual ~SubDriver();
};

/** Base class for server-linked drivers with a single client */
class LineDriver : public Driver
{
public:
  ServerPtr server;
  LineDriver(const LinkConnectClientPtr& c);
  virtual ~LineDriver();

  virtual bool setup(); // assigns the address
private:
  eibaddr_t _addr; // cached copy of conn->addr
public:
  eibaddr_t getAddress() { return _addr; }

protected:
  virtual bool hasAddress(eibaddr_t addr) { return addr == this->_addr; }
  virtual void addAddress(eibaddr_t addr)
    {
      if (addr != this->_addr)
        ERRORPRINTF(t,E_WARNING,"%s: Addr mismatch: %s vs. %s", this->name(), FormatEIBAddr (addr), FormatEIBAddr (this->_addr));
    }
  virtual bool checkAddress(eibaddr_t addr) { return addr == this->_addr; }
  virtual bool checkGroupAddress (eibaddr_t addr) { return true; }
};

#endif
