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

#ifndef LAYER23_H
#define LAYER23_H

#include "layer2.h"
#include "layer3.h"

/** generic interface for an Layer 2 driver */
class Layer23 : public Layer2shim, public Layer3
{
public:
  Layer23 (Layer2Ptr l2);

  // If you override: need to call Layer23::init last!
  virtual bool init (Layer3 *l3);

  // If you override: need to call Layer23::stop last!
  // Do not deregister here, the driver will call your deregisterLayer2().
  virtual void stop ();
  virtual const char *Name() { return "?-F"; }

  // You must override this, and return an instance of yourself,
  // with l2 set to the given l2.
  virtual Layer2Ptr clone (Layer2Ptr l2) = 0;

protected:
  Layer2Ptr l2;

public:

  /** implement all of layer2shim */

  void send_L_Data (LDataPtr l);

  bool addAddress (eibaddr_t addr);
  bool addGroupAddress (eibaddr_t addr);
  bool removeAddress (eibaddr_t addr);
  bool removeGroupAddress (eibaddr_t addr);
  bool hasAddress (eibaddr_t addr);
  bool hasGroupAddress (eibaddr_t addr);
  eibaddr_t getRemoteAddr();

  bool enterBusmonitor ();
  bool leaveBusmonitor ();

  bool Open ();
  bool Close ();

  /** implement all of layer3 */

  TracePtr tr();
  eibaddr_t getDefaultAddr();
  std::shared_ptr<GroupCache> getCache();

  Layer3 * registerLayer2 (Layer2Ptr l2);

  // Your stop() may or may not have been called already.
  // Do not call Layer23::stop() from here.
  bool deregisterLayer2 (Layer2Ptr l2);

  bool registerBusmonitor (L_Busmonitor_CallBack * c);
  bool registerVBusmonitor (L_Busmonitor_CallBack * c);

  bool deregisterBusmonitor (L_Busmonitor_CallBack * c);
  bool deregisterVBusmonitor (L_Busmonitor_CallBack * c);

  void recv_L_Data (LDataPtr l);
  void recv_L_Busmonitor (LBusmonPtr l);
  bool hasAddress (eibaddr_t addr, Layer2Ptr l2 = nullptr);
  bool hasGroupAddress (eibaddr_t addr, Layer2Ptr l2 = nullptr);
  void registerServer (BaseServer *s);
  void deregisterServer (BaseServer *s);

  eibaddr_t get_client_addr ();
  void release_client_addr (eibaddr_t addr);
};

#endif
