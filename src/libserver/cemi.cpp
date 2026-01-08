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

  // Read address from config - format: X.Y.Z in "addr" key
  std::string addrStr = cfg->value("addr", "");
  if (!addrStr.empty())
    {
      int a, b, c;
      if (sscanf(addrStr.c_str(), "%d.%d.%d", &a, &b, &c) == 3)
        {
          config_addr = ((a & 0x0f) << 12) | ((b & 0x0f) << 8) | (c & 0xff);
          config_addr_set = true;
        }
    }
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
  init_step = 0;
  read_subnet = 0;
  read_device = 0;
  need_write_addr = false;
  reset_timer.start(3.0, 0);  // Longer timeout for multi-step init
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
      CArray msg;

      switch (init_step)
        {
        case 0:
          // Step 0: Set comm mode to Data Link Layer
          init_step = 1;
          msg.resize(8);
          msg[0] = 0xf6; // M_PropWrite.req
          msg[1] = 0x00; // Object Type High (cEMI Server)
          msg[2] = 0x08; // Object Type Low
          msg[3] = 0x01; // Object Instance
          msg[4] = 0x34; // PID_COMM_MODE (52)
          msg[5] = 0x10; // NoE=1, SIx high
          msg[6] = 0x01; // SIx low
          msg[7] = 0x00; // Data Link Layer mode
          send_Data(msg);
          break;

        case 1:
          // Step 1: Read PID_SUBNET_ADDR
          init_step = 2;
          msg.resize(7);
          msg[0] = 0xfc; // M_PropRead.req
          msg[1] = 0x00; // Object Type High (Device Object)
          msg[2] = 0x00; // Object Type Low
          msg[3] = 0x01; // Object Instance
          msg[4] = 0x39; // PID_SUBNET_ADDR (57)
          msg[5] = 0x10; // NoE=1, SIx high
          msg[6] = 0x01; // SIx low
          send_Data(msg);
          break;

        case 2:
          // Step 2: Read PID_DEVICE_ADDR (after receiving subnet response)
          init_step = 3;
          msg.resize(7);
          msg[0] = 0xfc; // M_PropRead.req
          msg[1] = 0x00; // Object Type High (Device Object)
          msg[2] = 0x00; // Object Type Low
          msg[3] = 0x01; // Object Instance
          msg[4] = 0x3a; // PID_DEVICE_ADDR (58)
          msg[5] = 0x10; // NoE=1, SIx high
          msg[6] = 0x01; // SIx low
          send_Data(msg);
          break;

        case 3:
          // Step 3: Evaluate and decide
          {
            eibaddr_t usb_addr = ((uint16_t)read_subnet << 8) | read_device;
            bool is_unconfigured = (usb_addr == 0x0000) || (usb_addr == 0xFFFF);

            if (!config_addr_set)
              {
                // No addr configured - warn about individual addressing
                ERRORPRINTF(t, E_WARNING | 45,
                  "No addr configured; group communication works but individual addressing may fail");
                init_step = 6; // Skip to completion
                do_send_Next();
              }
            else if (is_unconfigured)
              {
                // USB is unconfigured - program it
                TRACEPRINTF(t, 1, "USB interface unconfigured, setting address to %d.%d.%d",
                  (config_addr >> 12) & 0xF, (config_addr >> 8) & 0xF, config_addr & 0xFF);
                need_write_addr = true;
                init_step = 4;
                do_send_Next();
              }
            else if (usb_addr == config_addr)
              {
                // Addresses match - all good, no log needed
                init_step = 6; // Skip to completion
                do_send_Next();
              }
            else
              {
                // Mismatch - warn but don't overwrite
                ERRORPRINTF(t, E_WARNING | 46,
                  "USB interface has address %d.%d.%d but config specifies %d.%d.%d - individual addressing may fail",
                  (usb_addr >> 12) & 0xF, (usb_addr >> 8) & 0xF, usb_addr & 0xFF,
                  (config_addr >> 12) & 0xF, (config_addr >> 8) & 0xF, config_addr & 0xFF);
                init_step = 6; // Skip to completion
                do_send_Next();
              }
          }
          break;

        case 4:
          // Step 4: Write PID_SUBNET_ADDR
          init_step = 5;
          msg.resize(8);
          msg[0] = 0xf6; // M_PropWrite.req
          msg[1] = 0x00; // Object Type High (Device Object)
          msg[2] = 0x00; // Object Type Low
          msg[3] = 0x01; // Object Instance
          msg[4] = 0x39; // PID_SUBNET_ADDR (57)
          msg[5] = 0x10; // NoE=1, SIx high
          msg[6] = 0x01; // SIx low
          msg[7] = (config_addr >> 8) & 0xFF; // Subnet
          send_Data(msg);
          break;

        case 5:
          // Step 5: Write PID_DEVICE_ADDR
          init_step = 6;
          msg.resize(8);
          msg[0] = 0xf6; // M_PropWrite.req
          msg[1] = 0x00; // Object Type High (Device Object)
          msg[2] = 0x00; // Object Type Low
          msg[3] = 0x01; // Object Instance
          msg[4] = 0x3a; // PID_DEVICE_ADDR (58)
          msg[5] = 0x10; // NoE=1, SIx high
          msg[6] = 0x01; // SIx low
          msg[7] = config_addr & 0xFF; // Device
          send_Data(msg);
          break;

        default:
          // Step 6: Done
          after_reset = false;
          reset_timer.stop();
          EMI_Common::started();
          break;
        }
    }
  else
    EMI_Common::do_send_Next();
}

void CEMIDriver::recv_Data(CArray& c)
{
  // Intercept M_PropRead.con (0xFB) and M_PropWrite.con (0xF5) during initialization
  if (after_reset && c.size() >= 7)
    {
      uint8_t mc = c[0];

      if (mc == 0xfb && c.size() >= 8) // M_PropRead.con
        {
          uint8_t pid = c[4];
          uint8_t value = c[7];

          if (pid == 0x39) // PID_SUBNET_ADDR
            {
              read_subnet = value;
              TRACEPRINTF(t, 2, "Read USB subnet address: 0x%02X", value);
            }
          else if (pid == 0x3a) // PID_DEVICE_ADDR
            {
              read_device = value;
              TRACEPRINTF(t, 2, "Read USB device address: 0x%02X", value);
            }
          // Property responses act as confirmations - continue init
          do_send_Next();
          return;
        }
      else if (mc == 0xf5) // M_PropWrite.con
        {
          // Property write confirmed - continue init
          do_send_Next();
          return;
        }
    }

  // Call parent handler for all other messages
  EMI_Common::recv_Data(c);
}

const uint8_t *
CEMIDriver::getIndTypes() const
{
  static const uint8_t indTypes[] = { 0x2E, 0x29, 0x2B };
  return indTypes;
}

