/*
    EIBD eib bus access and management daemon
    Copyright (C) 2015 Matthias Urlichs <matthias@urlichs.de>

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

#ifndef DUMMY_H
#define DUMMY_H
#include "layer2.h"
#include "layer23.h"

/** dummy L2 driver, does nothing */
class DummyL2Driver:public Layer2
{
  const char *Name() { return "Dummy"; }
public:
  DummyL2Driver (L2options *opt);
  ~DummyL2Driver ();
  bool init (Layer3 *l3);

  void send_L_Data (L_Data_PDU * l);
};

/** dummy L2 filter, is transparent */
class DummyL2Filter:public Layer23
{
  const char *Name() { return "Dummy"; }
public:
  DummyL2Filter (L2options *opt, Layer2Ptr l2);
  ~DummyL2Filter ();
  Layer2Ptr clone(Layer2Ptr l2);

  void send_L_Data (L_Data_PDU * l);
};

#endif
