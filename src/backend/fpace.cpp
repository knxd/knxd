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

#include "fpace.h"

PaceFilter::PaceFilter (const LinkConnectPtr_& c, IniSectionPtr& s) : Filter(c,s)
{
  timer.set<PaceFilter, &PaceFilter::timer_cb>(this);
  state = P_DOWN;
}

PaceFilter::~PaceFilter()
{
  timer.stop();
}

bool
PaceFilter::setup()
{
  if (findFilter("queue", true) == nullptr
      && std::dynamic_pointer_cast<LinkConnect>(conn.lock()) != nullptr)
    ERRORPRINTF(t, E_NOTICE, "The 'pace' filter without a queue acts globally.");
  if (!Filter::setup())
    return false;
  delay = cfg->value("delay",15)/1000.;
  if (delay <= 0)
    {
      ERRORPRINTF(t, E_ERROR, "The delay must be >0");
      return false;
    }
  byte_delay = cfg->value("delay-per-byte",1)/1000.;
  if (byte_delay < 0)
    {
      ERRORPRINTF(t, E_ERROR, "The delay must be >0");
      return false;
    }
  factor_in = cfg->value("incoming",0.75);
  if (factor_in < 0)
    {
      ERRORPRINTF(t, E_ERROR, "The factor for incoming packets must be >=0");
      return false;
    }
  return true;
}

void
PaceFilter::start()
{
  nr_in = 0;
  size_in = 0;
  Filter::start();
}

void
PaceFilter::started()
{
  switch(state)
    {
    case P_DOWN:
      if (want_next)
        Filter::send_Next();
      state = P_IDLE;
      break;
    default:
      ERRORPRINTF(t, E_WARNING, "state %d??", state);
      break;
    }
  Filter::started();
}

void
PaceFilter::stopped()
{
  state = P_DOWN;
  want_next = false;
  timer.stop();
  Filter::stopped();
}

void
PaceFilter::send_Next()
{

  switch(state)
    {
    case P_DOWN:
      want_next = true;
      break;
    case P_IDLE:
      state = P_BUSY;
      timer.start(last_len*byte_delay + delay);
      break;
    case P_BUSY:
      ERRORPRINTF(t, E_WARNING, "send_next on busy pacer?");
      break;
    }
}

void
PaceFilter::timer_cb (ev::timer &w UNUSED, int revents UNUSED)
{
  if (state != P_BUSY)
    return;
  if (factor_in > 0 && nr_in > 0)
    {
      timer.start((size_in*byte_delay + nr_in*delay) * factor_in);
      nr_in  = 0;
      size_in = 0;
      return;
    }
  state = P_IDLE;
  Filter::send_Next();
}

void
PaceFilter::send_L_Data (LDataPtr l)
{
  last_len = l->data.size();
  Filter::send_L_Data(std::move(l));
}

void
PaceFilter::recv_L_Data (LDataPtr l)
{
  nr_in += 1;
  size_in = l->data.size();
  Filter::recv_L_Data(std::move(l));
}

