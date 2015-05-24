/*
    EIBD eib bus access and management daemon
    Copyright (C) 2015 Matthias Urlichs <matthias@urlichs.de>

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

#include "layer2.h"
#include "layer3.h"

Layer2Interface::Layer2Interface (Layer3 *layer3)
{
  l3 = layer3;
  t = layer3->t;
  mode = BUSMODE_DOWN;
}

bool
Layer2Interface::init ()
{
  if (l3)
    l3->registerLayer2 (this);
  return true;
}

Layer2Interface::~Layer2Interface ()
{
  if (l3)
    l3->deregisterLayer2 (this);
}

bool
Layer2Interface::addAddress (eibaddr_t addr)
{
  unsigned i;
  for (i = 0; i < indaddr (); i++)
    if (indaddr[i] == addr)
      return 0;
  indaddr.add (addr);
  return 1;
}

bool
Layer2Interface::addGroupAddress (eibaddr_t addr)
{
  unsigned i;
  for (i = 0; i < groupaddr (); i++)
    if (groupaddr[i] == addr)
      return 0;
  groupaddr.add (addr);
  return 1;
}

bool
Layer2Interface::removeAddress (eibaddr_t addr)
{
  unsigned i;
  for (i = 0; i < indaddr (); i++)
    if (indaddr[i] == addr)
      {
        indaddr.deletepart (i, 1);
        return 1;
      }
  return 0;
}

bool
Layer2Interface::removeGroupAddress (eibaddr_t addr)
{
  unsigned i;
  for (i = 0; i < groupaddr (); i++)
    if (groupaddr[i] == addr)
      {
        groupaddr.deletepart (i, 1);
        return 1;
      }
  return 0;
}

bool
Layer2Interface::hasAddress (eibaddr_t addr)
{
  for (unsigned int i = 0; i < indaddr (); i++)
    if (indaddr[i] == addr)
      return true;
  return false;
}

bool
Layer2Interface::hasGroupAddress (eibaddr_t addr)
{
  for (unsigned int i = 0; i < groupaddr (); i++)
    if (groupaddr[i] == addr)
      return true;
  return false;
}

bool
Layer2Interface::enterBusmonitor ()
{
  if (mode != BUSMODE_DOWN)
    return false;
  mode = BUSMODE_MONITOR;
  return true;
}

bool
Layer2Interface::leaveBusmonitor ()
{
  if (mode != BUSMODE_MONITOR)
    return false;
  mode = BUSMODE_DOWN;
  return true;
}

bool
Layer2Interface::openVBusmonitor ()
{
  if (mode != BUSMODE_UP)
    return false;
  mode = BUSMODE_VMONITOR;
  return true;
}

bool
Layer2Interface::closeVBusmonitor ()
{
  if (mode != BUSMODE_VMONITOR)
    return false;
  mode = BUSMODE_UP;
  return true;
}

bool
Layer2Interface::Open ()
{
  if (mode != BUSMODE_DOWN)
    return false;
  mode = BUSMODE_UP;
  return true;
}

bool
Layer2Interface::Close ()
{
  if (mode != BUSMODE_UP)
    return false;
  mode = BUSMODE_UP;
  return true;
}

bool
Layer2Interface::Connection_Lost ()
{
  return false;
}

bool
Layer2Interface::Send_Queue_Empty ()
{
  return true;
}

