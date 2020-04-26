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

#include "monitor.h"
#include "cm_tp1.h"

bool
MonitorL2Filter::setup()
{
  auto cn = conn.lock();
  if (cn == nullptr)
    return false;
  t->TracePrintf (0, "State setup");
  return true;
}


void
MonitorL2Filter::recv_L_Data (LDataPtr l)
{
  CArray cm_tp1_array = L_Data_to_CM_TP1(l);
  LBusmonPtr mon_l = LBusmonPtr(new L_Busmon_PDU ());
  mon_l->lpdu.set (cm_tp1_array);
  Filter::recv_L_Busmonitor(std::move(mon_l));
  Filter::recv_L_Data(std::move(l));
}
