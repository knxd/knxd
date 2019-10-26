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
 * @addtogroup Common
 * @{
 */

#ifndef COMMON_H
#define COMMON_H

#include <cassert>

#include <ev++.h>

#include "queue.h"
#include "trace.h"
#include "types.h"

/** Domain address */
using domainaddr_t = uint16_t;

/** timestamp type*/
using timestamp_t = long long;

/**
 * add c to s as hex value
 * @param s string
 * @param c byte
 */
void addHex (std::string & s, const uint8_t c);

/**
 * add c to s as hex value
 * @param s string
 * @param c 16 bit int
 */
void add16Hex (std::string & s, const uint16_t c);

/** get current time */
timestamp_t getTime ();

/** formats an EIB individual address */
std::string FormatEIBAddr (const eibaddr_t a);
/** formats an EIB group address */
std::string FormatGroupAddr (const eibaddr_t a);
/** formats an EIB domain address */
std::string FormatDomainAddr (const domainaddr_t addr);
/** formats an EIB key */
std::string FormatEIBKey (const eibkey_type addr);

/** libev */
#if EV_MULTIPLICITY
using LOOP_RESULT = struct ev_loop *;
#else
using LOOP_RESULT = int;
#endif
extern LOOP_RESULT loop;

#endif

/** @} */
