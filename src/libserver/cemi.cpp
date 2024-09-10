/*
    EIBD eib bus access and management daemon
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>

    cEMI support for USB
    Copyright (C) 2013 Meik Felser <felser@cs.fau.de>

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

#include "cemi.h"

#include "emi.h"

unsigned int
CEMIDriver::maxPacketLen() const
{
  return 50;
}

CEMIDriver::CEMIDriver (LowLevelIface* c, IniSectionPtr& s, LowLevelDriver *i) : EMI_Common(c,s,i)
{
  t->setAuxName("CEMI");
  sendLocal_done.set<CEMIDriver,&CEMIDriver::sendLocal_done_cb>(this);
  reset_timer.set<CEMIDriver,&CEMIDriver::reset_timer_cb>(this);
}

void
CEMIDriver::sendLocal_done_cb(bool success)
{
  if (!success || sendLocal_done_next == N_bad)
    {
      stopped(true);
      LowLevelFilter::stopped(true);
    }
  else if (sendLocal_done_next == N_down)
    LowLevelFilter::stopped(false);
  else if (sendLocal_done_next == N_up)
    LowLevelFilter::started();
  else if (sendLocal_done_next == N_reset)
    EMI_Common::started();
}

void CEMIDriver::cmdEnterMonitor()
{
  stop(true);
}
void CEMIDriver::cmdLeaveMonitor()
{
  stop(true);
}
void CEMIDriver::cmdOpen()
{
  LowLevelDriver::started();
}
void CEMIDriver::cmdClose()
{
  LowLevelDriver::stop(false);
}

void CEMIDriver::started()
{
  sent_comm_mode = false;
  after_reset = true;
  reset_timer.start(0.5,0);
  sendReset();
}

void CEMIDriver::reset_timer_cb(ev::timer &, int)
{
  ERRORPRINTF(t, E_ERROR | 44, "reset timed out");
  stop(true);
}

void CEMIDriver::do_send_Next()
{
  if (after_reset)
    {
      if (!sent_comm_mode)
        {
          sent_comm_mode = true;

          // Set the comm mode to "Data Link Layer" (0x00)
          CArray set_comm_mode;
          set_comm_mode.resize (8);
          set_comm_mode[0] = 0xf6; // Message Code (MC), M_PropWrite.req
          // Interface Object Type = cEMI Server Object
          set_comm_mode[1] = 0x00; // IOTH
          set_comm_mode[2] = 0x08; // IOTL
          set_comm_mode[3] = 0x01; // Object Instance (OI)
          set_comm_mode[4] = 0x34; // Property ID (PID), PID_COMM_MODE (52)
          // number of elements (NoE) = 0x1
          // start index (SIx) = 0x001
          set_comm_mode[5] = 0x10; // NoE, SIx
          set_comm_mode[6] = 0x01; // SIx
          set_comm_mode[7] = 0x00; // Data (0x00 is "Data Link Layer")
          send_Data (set_comm_mode);
        }
      else
        {
          after_reset = false;
          reset_timer.stop();
          EMI_Common::started();
        }
    }
  else
    EMI_Common::do_send_Next();
}

const uint8_t *
CEMIDriver::getIndTypes() const
{
  static const uint8_t indTypes[] = { 0x2E, 0x29, 0x2B };
  return indTypes;
}

