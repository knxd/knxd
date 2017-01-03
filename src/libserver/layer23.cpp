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

#include "layer23.h"

Layer23::Layer23 (Layer2Ptr l2) : Layer2shim(NULL, l2->t)
{
    this->l2 = l2;
    this->t = TracePtr(new Trace(*(l2->t.get()), std::string(Name())+":"+l2->t->name));
}

bool
Layer23::init (Layer3 *l3)
{
    this->l3 = l3;
    return l2->init(this);
}

void
Layer23::stop ()
{
  l2->stop();
}

/** implement all of layer2shim */

void
Layer23::Send_L_Data (LPDU * l)
{
  l2->Send_L_Data (l);
}

bool
Layer23::addAddress (eibaddr_t addr)
{
  return l2->addAddress (addr);
}

bool
Layer23::addGroupAddress (eibaddr_t addr)
{
  return l2->addGroupAddress (addr);
}

bool
Layer23::removeAddress (eibaddr_t addr)
{
  return l2->removeAddress (addr);
}

bool
Layer23::removeGroupAddress (eibaddr_t addr)
{
  return l2->removeGroupAddress (addr);
}

bool
Layer23::hasAddress (eibaddr_t addr)
{
  return l2->hasAddress (addr);
}

bool
Layer23::hasGroupAddress (eibaddr_t addr)
{
  return l2->hasGroupAddress (addr);
}

eibaddr_t
Layer23::getRemoteAddr ()
{
  return l2->getRemoteAddr ();
}


bool
Layer23::enterBusmonitor ()
{
  return l2->enterBusmonitor ();
}

bool
Layer23::leaveBusmonitor ()
{
  return l2->leaveBusmonitor ();
}


bool
Layer23::Open ()
{
  return l2->Open ();
}

bool
Layer23::Close ()
{
  return l2->Close ();
}


/** implement all of layer3 */

TracePtr
Layer23::tr()
{
  return this->t;
}

eibaddr_t
Layer23::getDefaultAddr()
{
  return l3->getDefaultAddr();
}

std::shared_ptr<GroupCache>
Layer23::getCache()
{
  return l3->getCache();
}

Layer3 *
Layer23::registerLayer2 (Layer2Ptr l2)
{
  Layer2Ptr c = clone(l2);
  return l3->registerLayer2 (c);
}

bool
Layer23::deregisterLayer2 (Layer2Ptr l2)
{
  return l3->deregisterLayer2 (shared_from_this());
}


bool
Layer23::registerBusmonitor (L_Busmonitor_CallBack * c)
{
  return l3->registerBusmonitor (c);
}

bool
Layer23::registerVBusmonitor (L_Busmonitor_CallBack * c)
{
  return l3->registerVBusmonitor (c);
}


bool
Layer23::deregisterBusmonitor (L_Busmonitor_CallBack * c)
{
  return l3->deregisterBusmonitor (c);
}

bool
Layer23::deregisterVBusmonitor (L_Busmonitor_CallBack * c)
{
  return l3->deregisterVBusmonitor (c);
}


void
Layer23::recv_L_Data (LPDU * l)
{
  l3->recv_L_Data (l);
}

bool
Layer23::hasAddress (eibaddr_t addr, Layer2Ptr l2)
{
  return l3->hasAddress (addr, l2);
}

bool
Layer23::hasGroupAddress (eibaddr_t addr, Layer2Ptr l2)
{
  return l3->hasGroupAddress (addr, l2);
}

void
Layer23::registerServer (BaseServer *s)
{
  l3->registerServer (s);
}

void
Layer23::deregisterServer (BaseServer *s)
{
  l3->deregisterServer (s);
}


eibaddr_t
Layer23::get_client_addr ()
{
  return l3->get_client_addr ();
}

