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

#ifndef F_NAT_H
#define F_NAT_H

#include "nat.h"
#include "layer23.h"

#define NAT_S_URL "single\n"
#define NAT_S_DOC "The 'single' filter is used for talking to single-node gateways.\n" \
	"It zeroes the source address when sending, and restores the (physical) destination address when receiving.\n\n"
#define NAT_S_PREFIX "single"
#define NAT_S_FILTER nat_Filter

inline Layer2Ptr
nat_Filter (const char *dev UNUSED, L2options *opt, Layer2Ptr l2)
{
  return std::shared_ptr<NatL2Filter>(new NatL2Filter (opt, l2));
}

#endif
