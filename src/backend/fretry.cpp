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

RetryFilter::RetryFilter (const LinkConnectPtr_& c, IniSectionPtr& s) : Filter(c,s)
{
  trigger.set<RetryFilter, &RetryFilter::trigger_cb>(this);
  trigger.start();
  timeout.set<RetryFilter, &RetryFilter::timeout_cb>(this);
  state = R_DOWN;
}

RetryFilter::~RetryFilter()
{
  trigger.stop();
  timeout.stop();
}

bool
RetryFilter::setup()
{
  if(std::dynamic_pointer_cast<LinkConnect>(conn.lock()) == nullptr)
    {
      ERRORPRINTF(t, E_ERROR | 4, "You can't use the 'retry' filter globally");
      return false;
    }
  if (findFilter("retry", true) != nullptr)
    {
      ERRORPRINTF(t, E_ERROR | 112, "Two queue filters on a link does not make sense");
      return false;
    }
  if (!Filter::setup())
    return false;

  retry = 0;
  flush = cfg->value("flush",false);
  may_fail = cfg->value("may-fail",false);
  max_retry = cfg->value("max-retry",0);
  open_timeout = cfg->value("open-timeout",0);
  send_timeout = cfg->value("send-timeout",0);
  return true;
}

void
RetryFilter::started()
{
  switch(state)
    {
    case Q_DOWN:
      state = Q_IDLE;
      break;
    default:
      ERRORPRINTF(t, E_WARNING | 113, "state %d??", state);
      break;
    }
  Filter::started();
}

void
RetryFilter::stopped(bool err)
{
  
  state = Q_DOWN;
  Filter::stopped(err);
}

void
RetryFilter::send_Next()
{
  switch(state)
    {
    case Q_DOWN:
      ERRORPRINTF(t, E_WARNING | 114, "send_Next while down");
      break;
    case Q_IDLE:
      ERRORPRINTF(t, E_WARNING | 115, "spurious send_Next");
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
RetryFilter::trigger_cb (ev::async &, int)
{
  while (!buf.empty() && state == Q_IDLE)
    {
      state = Q_SENDING;
      LDataPtr l = buf.get();
      Filter::send_L_Data(std::move(l));
    }
  if (state == Q_SENDING)
    state = Q_BUSY;
}

void
RetryFilter::send_L_Data (LDataPtr l)
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

void
RetryFilter::start()
{
  switch(state)
    {
    case Q_DOWN:
      state = Q_IDLE;
      break;
    default:
      ERRORPRINTF(t, E_WARNING | 113, "state %d??", state);
      break;
    }
  Filter::start();
}

void
RetryFilter::stop(bool err)
{
  want_up = false;
  stop_(err);
}

void
RetryFilter::stop_(bool err)
{
  state = R_GOING_ERROR if err else R_GOING_DOWN;
  if (want_up && flush && msg) {
    msg = nullptr;
    Filter::send_Next();
  }
  Filter::stop(err);
}

void
RetryFilter::send_Next()
{
  switch(state)
    {
    case R_WAIT:
      state = R_UP;
      msg = nullptr;
      timeout.stop();
      Filter::send_Next();
      break;
    default:
      msg = nullptr;
      break;
    }
}

void
RetryFilter::trigger_cb (ev::async &, int)
{
}

void
RetryFilter::timeout_cb (ev::async &, int)
{
  switch(state)
    {
    case R_WAIT:
    case R_GOING_UP:
      self.stop_(true);
      break;
    default:
      break;
}

void
RetryFilter::send_L_Data (LDataPtr l)
{
  switch(state)
    {
    case R_UP:
      self.msg = l;
      Filter::send_L_Data(l);
      timeout.start(send_timeout, 0);
      break;

    case R_GOING_UP:
    case R_GOING_DOWN:
    case R_GOING_ERROR:
    case R_ERROR:
      if flush:
        Filter::send_Next();
      else:
        self.msg = l;
      // TODO if the queue is empty, short-cut
      trigger.send();

    default:
      ERRORPRINTF(t, E_ERROR | 135, "Retry: Send: wrong state %d", state);
      break;
    }
}

