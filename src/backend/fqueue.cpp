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

#include "fqueue.h"

QueueFilter::QueueFilter (const LinkConnectPtr_& c, IniSectionPtr& s) : Filter(c,s)
{
  trigger.set<QueueFilter, &QueueFilter::trigger_cb>(this);
  trigger.start();
  state = Q_DOWN;
}

QueueFilter::~QueueFilter()
{
  trigger.stop();
}

bool
QueueFilter::setup()
{
  if(std::dynamic_pointer_cast<LinkConnect>(conn.lock()) == nullptr)
    {
      ERRORPRINTF(t, E_ERROR, "You can't use the 'queue' filter globally");
      return false;
    }
  if (findFilter("queue", true) != nullptr)
    {
      ERRORPRINTF(t, E_WARNING, "Two queue filters on a link does not make sense");
      return false;
    }
  if (!Filter::setup())
    return false;
  // XXX options?
  return true;
}

void
QueueFilter::started()
{
  switch(state)
    {
    case Q_DOWN:
      state = Q_IDLE;
      break;
    default:
      ERRORPRINTF(t, E_WARNING, "state %d??", state);
      break;
    }
  Filter::started();
}

void
QueueFilter::stopped()
{
  buf.clear();
  state = Q_DOWN;
  Filter::stopped();
}

void
QueueFilter::send_Next()
{
  switch(state)
    {
    case Q_DOWN:
      ERRORPRINTF(t, E_WARNING, "send_Next while down");
      break;
    case Q_IDLE:
      ERRORPRINTF(t, E_WARNING, "spurious send_Next");
      break;
    case Q_BUSY:
      trigger.send();
      // fall thru
    case Q_SENDING:
      state = Q_IDLE;
      break;
    }
}

void
QueueFilter::trigger_cb (ev::async &w UNUSED, int revents UNUSED)
{
  while (!buf.isempty() && state == Q_IDLE)
    {
      state = Q_SENDING;
      LDataPtr l = buf.get();
      Filter::send_L_Data(std::move(l));
    }
  if (state == Q_SENDING)
    state = Q_BUSY;
}

void
QueueFilter::send_L_Data (LDataPtr l)
{
  switch(state)
    {
    case Q_IDLE:
      // TODO if the queue is empty, short-cut
      trigger.send();
    case Q_BUSY:
    case Q_SENDING:
      buf.emplace(std::move(l));
      Filter::send_Next();
      break;
    default:
      break;
    }
}

