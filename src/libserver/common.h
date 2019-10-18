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

#ifndef COMMON_H
#define COMMON_H

#include "types.h"
#include <cassert>
#include <ev++.h>

/** Domain address */
typedef uint16_t domainaddr_t;
/** serial number */
typedef uchar serialnumber_t[6];
/** memory pointer for EIB devices */
typedef uint16_t memaddr_t;

/** object index type */
typedef uchar objectno_t;
/** property ID type */
typedef uchar propertyid_t;

/** timestamp type*/
typedef long long timestamp_t;

/** EIB message priority */
typedef enum
{
  PRIO_SYSTEM = 0,
  PRIO_URGENT = 1,
  PRIO_NORMAL = 2,
  PRIO_LOW = 3
} EIB_Priority;

/** EIB address type */
typedef enum
{
  IndividualAddress = 0,
  GroupAddress = 1
} EIB_AddrType;


#include "queue.h"
#include "exception.h"

/** add c to s as hex value
 * @param s string
 * @param c byte
 */
void addHex (std::string & s, const uchar c);
/** add c to s as hex value
 * @param s string
 * @param c 16 bit int
 */
void add16Hex (std::string & s, const uint16_t c);

/** get current time */
timestamp_t getTime ();

/** formats an EIB individual address */
std::string FormatEIBAddr (eibaddr_t a);
/** formats an EIB group address */
std::string FormatGroupAddr (eibaddr_t a);
/** formats an EIB domain address */
std::string FormatDomainAddr (domainaddr_t addr);
/** formats an EIB key */
std::string FormatEIBKey (eibkey_type addr);

#include "trace.h"

/** libev */
#if EV_MULTIPLICITY
  typedef struct ev_loop *LOOP_RESULT;
#else
  typedef int LOOP_RESULT;
#endif
extern LOOP_RESULT loop;


#endif
