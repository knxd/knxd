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

#ifndef LOWLEVEL_H
#define LOWLEVEL_H

#include "common.h"
#include "link.h"
#include "emi.h"

typedef void (*packet_cb_t)(void *data, CArray *p);

class LowLevelDriver;
#ifdef NO_MAP
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

/** implements interface for a Driver to send packets for the EMI1/2 driver */
class LowLevelIface
{
public:
  virtual ~LowLevelIface();

  virtual TracePtr tr() = 0;
  virtual void started() = 0;
  virtual void stopped() = 0;
  virtual void send_Next() = 0;
  virtual void recv_Data(CArray& c) = 0;

  virtual void recv_L_Data(LDataPtr l) = 0;
  virtual void recv_L_Busmonitor(LBusmonPtr l) = 0;
};

class LowLevelDriver : public LowLevelIface
{
public:
  typedef LowLevelIface* first_arg;

  TracePtr tr() { return t; }
private:
  bool is_local = false;

protected:
  LowLevelIface* master;
  /** configuration */
  IniSection &cfg;
  /** debug output */
  TracePtr t;

public:
  LowLevelDriver (LowLevelIface* parent, IniSection &s) : cfg(s)
    {
      t = TracePtr(new Trace(*parent->tr(),s));
      master = parent;
    }

  void resetMaster(LowLevelIface* parent)
    {
      master = parent;
    }

  virtual ~LowLevelDriver ();

  virtual bool setup () { return true; }
  virtual void start () { started(); }
  virtual void stop () { stopped(); }

  void started() { master->started(); }
  void stopped() { master->stopped(); }
  void send_Next();
  void recv_L_Data(LDataPtr l) { master->recv_L_Data(std::move(l)); }
  void recv_L_Busmonitor(LBusmonPtr l) { master->recv_L_Busmonitor(std::move(l)); }

  /** sends a EMI frame asynchronous */
  virtual void send_Data (CArray& l) = 0;
  /** like send_Data but busy-waits for send_Next call */
  virtual void send_Local (CArray& l);
  inline void send_Data (CArray&& l) { CArray lx = l; send_Data(lx); }
  inline void send_Local (CArray&& l) { CArray lx = l; send_Local(lx); }
  virtual void sendReset() {}
  virtual void recv_Data(CArray& c) { master->recv_Data(c); }
};

class LowLevelFilter : public LowLevelDriver
{
public:
  LowLevelDriver *iface;
  LowLevelFilter (LowLevelIface* parent, IniSection &s) : LowLevelDriver(parent,s) {}
  LowLevelFilter (LowLevelDriver* i, LowLevelIface* parent, IniSection &s)
      : LowLevelDriver(parent,s)
    {
      iface = i;
    }
  virtual ~LowLevelFilter();

  virtual bool setup()
    {
      if (iface == nullptr)
        return false;
      return iface->setup();
    }
  virtual void start () { iface->start(); }
  virtual void stop () { iface->stop(); }
  virtual void sendReset() { iface->sendReset(); }
  virtual void send_Data(CArray& c) { iface->send_Data(c); }
};

class LowLevelAdapter : public BusDriver, public LowLevelIface
{
protected:
  LowLevelDriver* iface = nullptr;
public:
  TracePtr tr() { return t; }

  LowLevelAdapter(const LinkConnectPtr_& c, IniSection& s) : BusDriver(c,s),LowLevelIface() {}
  virtual ~LowLevelAdapter();

  bool setup()
    {
      if (iface == nullptr)
        return false;
      if (!iface->setup())
        goto ex;
      if (!BusDriver::setup())
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
        stopped();
    }
  void stop()
    {
      if (iface)
        iface->stop();
      else
        stopped();
    }

  void send_L_Data(LDataPtr l)
    {
      if (!iface)
        return;
      CArray pdu = L_Data_ToEMI (0x11, l);
      iface->send_Data(pdu);
    }
  void recv_L_Data(LDataPtr l) { BusDriver::recv_L_Data(std::move(l)); }
  void recv_L_Busmonitor(LBusmonPtr l) { BusDriver::recv_L_Busmonitor(std::move(l)); }

  void recv_Data(CArray& c)
    {
      LDataPtr l = EMI_to_L_Data (c, t);
      BusDriver::recv_L_Data(std::move(l));
    }

  inline void started() { BusDriver::started(); }
  inline void stopped() { BusDriver::stopped(); }
  inline void send_Next() { BusDriver::send_Next(); }
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
