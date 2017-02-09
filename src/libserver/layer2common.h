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

#ifndef LAYER2COMMON_H
#define LAYER2COMMON_H

#include <memory>
#include "trace.h"

class Layer2shim;
typedef std::shared_ptr<Layer2shim> Layer2Ptr;

/** Bus modes. The enum is designed to allow bitwise tests
 * (& BUSMODE_UP and * & BUSMODE_MONITOR) */
typedef enum {
  BUSMODE_DOWN = 0,
  BUSMODE_UP,
  BUSMODE_MONITOR,
  BUSMODE_VMONITOR,
} busmode_t;

typedef struct {
  unsigned int flags;
  unsigned int send_delay;
  TracePtr t;
} L2options;

class Layer3;

#define FLAG_B_TUNNEL_NOQUEUE (1<<0)
#define FLAG_B_TPUARTS_ACKGROUP (1<<1)
#define FLAG_B_TPUARTS_ACKINDIVIDUAL (1<<2)
#define FLAG_B_TPUARTS_DISCH_RESET (1<<3)
#define FLAG_B_EMI_NOQUEUE (1<<4)
#define FLAG_B_NO_MONITOR (1<<5)

#endif
