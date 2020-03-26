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

#include "retry.h"

RetryFilter::RetryFilter (const LinkConnectPtr_& c, IniSectionPtr& s) : Filter(c,s)
{
  trigger.set<RetryFilter, &RetryFilter::trigger_cb>(this);
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
  if (std::dynamic_pointer_cast<LinkConnect>(conn.lock()) == nullptr)
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

  retries = 0;

  auto c = std::static_pointer_cast<LinkConnect>(conn.lock());
  flush = cfg->value("flush", false);
  may_fail = cfg->value("may-fail", c->x_may_fail);
  max_retry = cfg->value("max-retries", (c->x_max_retries < 0) ? 1 : c->x_max_retries);
  retry_delay = cfg->value("retry-delay", c->x_retry_delay);
  send_timeout = cfg->value("send-timeout", 5.);
  if (send_timeout < 0.01)
    {
      ERRORPRINTF(t, E_ERROR | 144, "send_timeout must be positive");
      return false;
    }
  start_timeout = cfg->value("start-timeout", 10.);
  if (start_timeout < 0.01)
    {
      ERRORPRINTF(t, E_ERROR | 145, "start_timeout must be positive");
      return false;
    }
  return true;
}

void
RetryFilter::started()
{
  switch(state)
    {
    case R_GOING_UP:
      state = R_UP;
      trigger.start();
      break;

    default:
      ERRORPRINTF(t, E_WARNING | 113, "state %d??", state);
      break;
    }
  Filter::started();
}

void
RetryFilter::send_Next()
{
  switch(state)
    {
    case R_UP:
      if (msg == nullptr)
        ERRORPRINTF(t, E_WARNING | 115, "spurious send_Next");
      else
        {
          msg = nullptr;
          timeout.stop();
          Filter::send_Next();
        }
      break;

    default:
      ERRORPRINTF(t, E_WARNING | 136, "spurious send_Next in %d", state);
      break;
    }
}

void
RetryFilter::send_L_Data (LDataPtr l)
{
  switch(state)
    {
    case R_UP:
      if (msg != nullptr)
        ERRORPRINTF(t, E_WARNING | 137, "spurious send");
      else
        {
          msg = LDataPtr(new L_Data_PDU (*l));
          timeout.start(send_timeout, 0);
          Filter::send_L_Data(std::move(l));
        }
      break;

    default:
      if (! flush)
        ERRORPRINTF(t, E_WARNING | 138, "spurious send in %d", state);
      Filter::send_Next();
      break;
    }
}

void
RetryFilter::start()
{
  switch(state)
    {
    case R_DOWN:
      state = R_GOING_UP;
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
  state = err ? R_GOING_ERROR : R_GOING_DOWN;
  if (want_up && flush && msg) {
    msg = nullptr;
    Filter::send_Next();
  }
  Filter::stop(err);
}

void
RetryFilter::stopped(bool err)
{
  if (! want_up)
    goto off;

  switch(state)
    {
    case R_GOING_UP:
    case R_UP:
      state = R_ERROR;
      if (!retries && !may_fail)
        goto off;

      retries++;
      if (max_retry && (retries >= max_retry))
        goto off;

      if (msg && flush)
        {
          msg = nullptr;
          Filter::send_Next();
        }
      timeout.start(retry_delay);
      return;

    default:
      goto off;
    }

off:
  state = R_DOWN;
  msg = nullptr;
  Filter::stopped(err);
}

void
RetryFilter::trigger_cb (ev::async &, int)
{
  switch(state)
    {
    case R_GOING_UP:
      Filter::start();
      break;

    case R_UP:
      if (msg)
        Filter::send_L_Data(LDataPtr(new L_Data_PDU (*msg)));
      break;

    default:
      ERRORPRINTF(t, E_WARNING | 142, "trigger: wrong state %d",state);
      break;
    }
}

void
RetryFilter::timeout_cb (ev::timer &, int)
{
  switch(state)
    {
    case R_UP:
      if (msg == nullptr)
        {
          ERRORPRINTF(t, E_WARNING | 139, "spurious timeout UP");
          break;
        }
      retries++;
      if ((max_retry > 0) && (retries >= max_retry))
        {
          ERRORPRINTF(t, E_WARNING | 140, "send: max retries");
          stop_(true);
        }
      timeout.start(send_timeout);
      Filter::send_L_Data(LDataPtr(new L_Data_PDU (*msg)));
      break;

    case R_GOING_UP:
    case R_ERROR:
      if (!retries && !may_fail)
        {
          stop_(true);
          break;
        }
      retries++;
      if (max_retry && (retries >= max_retry))
        {
          stop_(true);
          break;
        }
      ERRORPRINTF(t, E_WARNING | 140, "open: retrying");
      state = R_GOING_UP;
      timeout.start(start_timeout);
      Filter::start();
      break;

    default:
      ERRORPRINTF(t, E_WARNING | 141, "timeout: wrong state %d",state);
      break;
    }
}

