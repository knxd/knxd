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

/**
 * @file
 * @ingroup KNX_03_06_03_03
 * External Message Interface 2.0
 * @{
 */

#ifndef EIB_EMI2_H
#define EIB_EMI2_H

#include "emi_common.h"

/** EMI2 backend */
class EMI2Driver:public EMI_Common
{
public:
  EMI2Driver (LowLevelIface* c, IniSectionPtr& s, LowLevelDriver *i = nullptr);
  virtual ~EMI2Driver () = default;
  void do_send_Next();

private:
  bool reset_ack_wait = false;
  ev::timer reset_timer;

  virtual void cmdEnterMonitor() override;
  virtual void cmdLeaveMonitor() override;
  virtual void cmdOpen() override;
  virtual void cmdClose() override;
  void started(); // do sendReset
  virtual const uint8_t * getIndTypes() const override;
  virtual EMIVer getVersion() const override
  {
    return vEMI2;
  }
  void sendLocal_done_cb(bool success);
  void reset_timer_cb(ev::timer& w, int revents);
  enum { N_bad, N_up, N_want_close, N_want_leave, N_down, N_open, N_enter } sendLocal_done_next = N_bad;
};

#endif

/** @} */
