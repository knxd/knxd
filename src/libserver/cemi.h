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

#ifndef EIB_CEMI_H
#define EIB_CEMI_H

#include "emi_common.h"

/** CEMI backend */
class CEMIDriver:public EMI_Common
{
  void cmdEnterMonitor();
  void cmdLeaveMonitor();
  void cmdOpen(); 
  void cmdClose();
  void started(); // do sendReset
  const uint8_t * getIndTypes(); 
  EMIVer getVersion() { return vCEMI; }

  unsigned int maxPacketLen();
  void sendLocal_done_cb(bool success);

  bool after_reset = false;
protected:
  enum { N_bad, N_up, N_down, N_open, N_reset } sendLocal_done_next = N_bad;

private:
  ev::timer reset_timer;
  void reset_timer_cb(ev::timer &w, int revents);

  virtual CArray lData2EMI (uchar code, const LDataPtr &p) 
  { return L_Data_ToCEMI(code, p); }
  virtual LDataPtr EMI2lData (const CArray & data) 
  { return CEMI_to_L_Data(data, t); }

public:
  CEMIDriver (LowLevelIface* c, IniSectionPtr& s, LowLevelDriver *i = nullptr);
  virtual ~CEMIDriver ();
  void do_send_Next();
};

#endif  /* EIB_CEMI_H */
