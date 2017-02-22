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

typedef void (*c_recv_cb_t)(void *data, CArray *p);

class c_recv_cb {
    c_recv_cb_t cb_code = 0;
    void *cb_data = 0;

    void set_ (const void *data, c_recv_cb_t cb)
    {
      this->cb_data = (void *)data;
      this->cb_code = cb;
    }

public:
    // function callback
    template<void (*function)(CArray *p)>
    void set (void *data = 0) throw ()
    {
      set_ (data, function_thunk<function>);
    }

    template<void (*function)(CArray *p)>
    static void function_thunk (void *arg, CArray *p)
    {
      function(p);
    }

    // method callback
    template<class K, void (K::*method)(CArray *p)>
    void set (K *object)
    {
      set_ (object, method_thunk<K, method>);
    }

    template<class K, void (K::*method)(CArray *p)>
    static void method_thunk (void *arg, CArray *p)
    {
      (static_cast<K *>(arg)->*method) (p);
    }

    void operator()(CArray *p) {
        (*cb_code)(cb_data, p);
    }
};

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


typedef enum
{ vERROR, vEMI1 = 1, vEMI2 = 2, vCEMI = 3, vRaw, vDiscovery, vTIMEOUT } EMIVer;

/** implements interface for a Driver to send packets for the EMI1/2 driver */
class LowLevelDriver
{
public:
  typedef DriverPtr first_arg;
private:
  void recv_discard(CArray *p);

protected:
  DriverPtr master;
  /** configuration */
  IniSection &cfg;
  /** debug output */
  TracePtr t;
public:
  LowLevelDriver (const DriverPtr& parent, IniSection &s) : cfg(s)
  {
    t = TracePtr(new Trace(*parent->t,s));
    master = parent;
    reset();
  }
  void reset() {
    on_recv.set<LowLevelDriver, &LowLevelDriver::recv_discard>(this);
  }

  virtual ~LowLevelDriver ();

  virtual bool setup (DriverPtr master)
    {
      this->master = master;
      return true;
    }
  virtual void start () = 0;
  virtual void stop () = 0;

  void started() { master->started(); }
  void stopped() { master->stopped(); }

  /** sends a EMI frame asynchronous */
  virtual void Send_Packet (CArray l) = 0;

  /** resets the connection */
  virtual void SendReset () = 0;

  virtual EMIVer getEMIVer () = 0;

  c_recv_cb on_recv;

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
