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

#ifndef LAYER2_H
#define LAYER2_H

#include "layer2common.h"
#include "common.h"
#include "lpdu.h"
#include <memory>

class Layer3;
class Layer4common;

/** generic interface for an Layer 2 driver */
class Layer2 : public std::enable_shared_from_this<Layer2>
{
  friend class Layer3;

  /** my individual addresses */
  Array < eibaddr_t > indaddr;
  /** source addresses when the destination is my own */
  Array < eibaddr_t > revaddr;
  /** my group addresses */
  Array < eibaddr_t > groupaddr;

  bool allow_monitor;

  /** auto-deregister for "tasked" layer2 objects */
  void RunStop();

protected:
  /** auto-assigned. NON-bus connections only! */
  eibaddr_t remoteAddr;

public:
  /** debug output */
  Trace *t;
  /** our layer-3 (to send packets to) */
  Layer3 *l3;

  Layer2 (L2options *opt, Trace *tr = 0);
  virtual bool init (Layer3 *l3);

  /** sends a Layer 2 frame asynchronouse */
  virtual void Send_L_Data (LPDU * l) = 0;
  virtual void Send_L_Data (L_Data_PDU * l) { Send_L_Data((LPDU *)l); }

  /** try to add the individual addr to the device, return true if successful */
  virtual bool addAddress (eibaddr_t addr);
  /** add the reverse addr to the device, return true if successful */
  virtual bool addReverseAddress (eibaddr_t addr);
  /** try to add the group address addr to the device, return true if successful */
  virtual bool addGroupAddress (eibaddr_t addr);
  /** try to remove the individual address addr to the device, return true if successful */
  virtual bool removeAddress (eibaddr_t addr);
  /** try to remove the individual address addr to the device, return true if successful */
  virtual bool removeReverseAddress (eibaddr_t addr);
  /** try to remove the group address addr to the device, return true if successful */
  virtual bool removeGroupAddress (eibaddr_t addr);
  /** individual address known? */
  bool hasAddress (eibaddr_t addr);
  /** reverse address known? */
  bool hasReverseAddress (eibaddr_t addr);
  /** group address known? */
  bool hasGroupAddress (eibaddr_t addr);

  /** try to enter the busmonitor mode, return true if successful */
  virtual bool enterBusmonitor ();
  /** try to leave the busmonitor mode, return true if successful */
  virtual bool leaveBusmonitor ();

  /** try to enter the normal operation mode, return true if successful */
  virtual bool Open ();
  /** try to leave the normal operation mode, return true if successful */
  virtual bool Close ();
  /** return true, if all frames have been sent */
  virtual bool Send_Queue_Empty ();

protected:
  busmode_t mode;
};



/** pointer to a functions, which creates a Layer 2 interface
 * @exception Exception in the case of an error
 * @param conf string, which contain configuration
 * @param t trace output
 * @return new Layer 2 interface
 */
typedef Layer2Ptr (*Layer2_Create_Func) (const char *conf, L2options *opt);

/** Layer2 mix-in class for network interfaces
 * without "real" hardware behind them
 */
class Layer2mixin:public Layer2
{
public:
  Layer2mixin (Trace *tr) : Layer2 (NULL, tr)
    {
      t = tr;
    }
  void Send_L_Data (LPDU * l) { delete l; }
  virtual void Send_L_Data (L_Data_PDU * l) = 0;
  bool enterBusmonitor () { return 0; }
  bool leaveBusmonitor () { return 0; }
  bool Open () { return 1; }
  bool Close () { return 1; }
  bool Send_Queue_Empty () { return 1; }
};

/** Layer2 mix-in class for interfaces
 * which don't ever do anything,
 * e.g. server sockets used to accept() connections
 */
class Layer2virtual:public Layer2mixin
{
public:
  Layer2virtual (Trace *tr) : Layer2mixin (tr) { }
  void Send_L_Data (LPDU * l) { delete l; }
  void Send_L_Data (L_Data_PDU * l) { delete l; }
  bool addAddress (eibaddr_t addr UNUSED) { return 1; }
  bool addGroupAddress (eibaddr_t addr UNUSED) { return 1; }
  bool removeAddress (eibaddr_t addr UNUSED) { return 1; }
  bool removeGroupAddress (eibaddr_t addr UNUSED) { return 1; }
};

#endif
