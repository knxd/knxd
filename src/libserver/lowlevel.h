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

/**
 * @file
 * @addtogroup Driver
 * @{
 */

#ifndef LOWLEVEL_H
#define LOWLEVEL_H

#include "common.h"
#include "emi.h"
#include "iobuf.h"
#include "link.h"

/** Low level interface
 *
 * This code implements the basis for the interface between a driver and
 * the actual hardware. In particular, we need rudimentary protocol
 * layering (cEMI => ft12 => serial, or => EMI1 => ft12 => TCP => socat).
 *
 * This interface deals with encapsulating a KNX packet in an opaque data
 * or packet stream. In contrast, the Filter/Driver interface is about
 * manipulating and filtering structured KNX packets.
 *
 *
 * Classes:
 *
 *
 */
typedef void (*packet_cb_t)(void *data, CArray *p);

#define NO_MAP_LL

class LowLevelDriver;
#if defined(NO_MAP) || defined(NO_MAP_LL)
#define LOWLEVEL(_cls,_name) \
class _cls : public LowLevelDriver
#define LOWLEVEL_(_cls,_base,_name) \
class _cls : public _base

#else
#define LOWLEVEL(_cls,_name) \
static constexpr const char _cls##_name[] = #_name; \
class _cls; \
static AutoRegister<_cls,LowLevelDriver,_cls##_name> _auto_L##_name; \
class _cls : public LowLevelDriver

#define LOWLEVEL_(_cls,_base,_name) \
static constexpr const char _cls##_name[] = #_name; \
class _cls; \
static AutoRegister<_cls,LowLevelDriver,_cls##_name> _auto_L##_name; \
class _cls : public _base

#endif

/** common code for drivers / interfaces / adapters */
class LowLevelIface
{
public:
  LowLevelIface();
  virtual ~LowLevelIface();

  virtual TracePtr tr() const = 0;
  virtual void started() = 0;
  virtual void stopped(bool err) = 0;
  virtual void recv_Data(CArray& c) = 0;
  virtual void send_Data(CArray& c) = 0;

  virtual void send_L_Data(LDataPtr l) = 0;
  virtual void recv_L_Data(LDataPtr l) = 0;
  virtual void recv_L_Busmonitor(LBusmonPtr l) = 0;

  /** Callback for send_Local */
  StateCallback sendLocal_done;
  void sendLocal_done_cb(bool); // default: ignore success, abort on error
  void send_Next();

  /** like send_Data but calls the sendLocal_done CB upon success */
  void send_Local (CArray& l, int raw = 0);
  virtual void do_send_Local (CArray& l, int raw = 0)
  {
    assert(!raw);
    send_Data(l);
  };

  /* adapters for non-lvalue calls et al. */
  inline void do_send_Local (CArray&& l, int raw = 0)
  {
    do_send_Local(l, raw);
  };
  inline void send_Local (CArray&& l, int raw = 0)
  {
    CArray lx = l;
    send_Local(lx, raw);
  }
  inline void send_Data (CArray&& l)
  {
    CArray lx = l;
    send_Data(lx);
  }
  inline void recv_Data (CArray&& l)
  {
    CArray lx = l;
    recv_Data(lx);
  }
  inline void send_Data (unsigned char c)
  {
    CArray ca(&c,1);
    send_Data(ca);
  }
  inline void recv_Data (unsigned char c)
  {
    CArray ca(&c,1);
    recv_Data(ca);
  }
  inline void send_Data (unsigned char *c, size_t len)
  {
    CArray ca(c,len);
    send_Data(ca);
  }
  inline void recv_Data (unsigned char *c, size_t len)
  {
    CArray ca(c,len);
    send_Data(ca);
  }

  virtual FilterPtr findFilter(std::string name) = 0;
  virtual bool checkAddress(eibaddr_t addr) const = 0;
  virtual bool checkGroupAddress(eibaddr_t addr) const = 0;
  virtual bool checkSysAddress(eibaddr_t addr) = 0;
  virtual bool checkSysGroupAddress(eibaddr_t addr) = 0;

protected:
  bool is_local = false;

private:
  ev::timer local_timeout;
  void local_timeout_cb(ev::timer &w, int revents);

  virtual void do_send_Next() = 0;
};

/** Code that passes incoming packets to actual hardware */
class LowLevelDriver : public LowLevelIface
{
public:
  typedef LowLevelIface* first_arg;

  TracePtr tr() const
  {
    return t;
  }

  LowLevelDriver (LowLevelIface* parent, IniSectionPtr& s) : cfg(s)
  {
    t = TracePtr(new Trace(*parent->tr(),s));
    t->setAuxName("LowD");
    master = parent;
  }

  void resetMaster(LowLevelIface* parent)
  {
    master = parent;
  }

  virtual ~LowLevelDriver () = default;

  virtual bool setup ()
  {
    return true;
  }

  virtual void start ()
  {
    started();
  }

  virtual void stop (bool err)
  {
    stopped(err);
  }

  virtual FilterPtr findFilter(std::string name)
  {
    return master->findFilter(name);
  }

  virtual bool checkAddress(eibaddr_t addr) const
  {
    return master->checkAddress(addr);
  }

  virtual bool checkGroupAddress(eibaddr_t addr) const
  {
    return master->checkGroupAddress(addr);
  }

  virtual bool checkSysAddress(eibaddr_t addr)
  {
    return master->checkSysAddress(addr);
  }

  virtual bool checkSysGroupAddress(eibaddr_t addr)
  {
    return master->checkSysGroupAddress(addr);
  }

  void started()
  {
    master->started();
  }
  void stopped(bool err)
  {
    master->stopped(err);
  }
  void do_send_Next()
  {
    master->send_Next();
  }
  void recv_L_Data(LDataPtr l)
  {
    master->recv_L_Data(std::move(l));
  }
  void recv_L_Busmonitor(LBusmonPtr l)
  {
    master->recv_L_Busmonitor(std::move(l));
  }
  void send_L_Data(LDataPtr l)
  {
    ERRORPRINTF (t, E_ERROR | 78, "packet not coded: %s", l->Decode(t));
  }

  /** sends a EMI frame asynchronous */
  virtual void sendReset()
  {
    send_Next();
  }
  virtual void recv_Data(CArray& c)
  {
    master->recv_Data(c);
  }
  virtual void abort_send()
  {
    ERRORPRINTF (t, E_ERROR | 79, "cannot abort");
  }

protected:
  LowLevelIface* master;
  /** configuration */
  IniSectionPtr cfg;
  /** debug output */
  TracePtr t;
};


/** base driver for talking to file descriptors */
class FDdriver:public LowLevelDriver
{
public:
  FDdriver (LowLevelIface* parent, IniSectionPtr& s);
  virtual ~FDdriver ();
  bool setup();
  void start();
  void stop(bool err);

protected:
  /** device connection */
  int fd = -1;

  /** queueing */
  SendBuf sendbuf;
  RecvBuf recvbuf;
  size_t read_cb(uint8_t *buf, size_t len);
  void error_cb();

  virtual void send_Data (CArray& c);

  void setup_buffers();
};


/** low-level filter: pass data to a driver, or to another filter */
class LowLevelFilter : public LowLevelDriver
{
public:
  LowLevelDriver *iface = nullptr;
  LowLevelFilter (LowLevelIface* parent, IniSectionPtr& s, LowLevelDriver* i = nullptr);
  virtual ~LowLevelFilter();

  virtual bool setup();
  virtual void start ()
  {
    iface->start();
  }
  virtual void stop (bool err)
  {
    iface->stop(err);
  }
  virtual void sendReset()
  {
    iface->sendReset();
  }
  virtual void send_Data(CArray& c)
  {
    iface->send_Data(c);
  }
  virtual void abort_send()
  {
    iface->abort_send();
  }
  virtual void send_L_Data(LDataPtr l)
  {
    iface->send_L_Data(std::move(l));
  }
  virtual void do_send_Local (CArray& l, int raw = 0);

protected:
  bool inserted = false; // don't propagate setup()
};

/** Base class for accepting a high-level KNX packet and forwarding it to
 * a LowLevelDriver
 */
class LowLevelAdapter : public HWBusDriver, public LowLevelIface
{
public:
  TracePtr tr() const
  {
    return t;
  }

  LowLevelAdapter(const LinkConnectPtr_& c, IniSectionPtr& s) : HWBusDriver(c,s),LowLevelIface()
  {
    t->setAuxName("LowA");
  }
  virtual ~LowLevelAdapter();

  bool setup()
  {
    if (iface == nullptr)
      return false;
    if (!iface->setup())
      goto ex;
    if (!HWBusDriver::setup())
      goto ex;
    return true;

ex:
    delete iface;
    iface = nullptr;

    return false;
  }

  void start()
  {
    if (iface)
      iface->start();
    else
      stopped(true);
  }
  void stop(bool err)
  {
    if (iface)
      iface->stop(err);
    else
      stopped(err);
  }

  void send_L_Data(LDataPtr l);
  void recv_L_Data(LDataPtr l)
  {
    HWBusDriver::recv_L_Data(std::move(l));
  }
  void recv_L_Busmonitor(LBusmonPtr l)
  {
    HWBusDriver::recv_L_Busmonitor(std::move(l));
  }

  void recv_Data(CArray& c)
  {
    t->TracePacket (0, "unknown data", c);
  }
  void send_Data(CArray& c)
  {
    t->TracePacket (0, "unknown data", c);
  }

  void started()
  {
    HWBusDriver::started();
  }
  void stopped(bool err)
  {
    HWBusDriver::stopped(err);
  }
  void do_send_Next()
  {
    HWBusDriver::send_Next();
  }
  void do_send_Local(CArray &d, int raw);

  FilterPtr findFilter(std::string name)
  {
    return HWBusDriver::findFilter(name);
  }

  virtual bool checkAddress(eibaddr_t addr) const
  {
    return HWBusDriver::checkAddress(addr);
  }

  virtual bool checkGroupAddress(eibaddr_t addr) const
  {
    return HWBusDriver::checkGroupAddress(addr);
  }

  virtual bool checkSysAddress(eibaddr_t addr)
  {
    return HWBusDriver::checkSysAddress(addr);
  }

  virtual bool checkSysGroupAddress(eibaddr_t addr)
  {
    return HWBusDriver::checkSysGroupAddress(addr);
  }

protected:
  LowLevelDriver* iface = nullptr;
};

/** pointer to a functions, which creates a Low Level interface
 * @exception Exception in the case of an error
 * @param conf string, which contain configuration
 * @param t trace output
 * @return new LowLevel interface
 */
typedef LowLevelDriver *(*LowLevel_Create_Func) (const char *conf,
    TracePtr tr);

#endif

/** @} */
