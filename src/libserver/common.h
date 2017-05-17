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
#include <assert.h>
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
  PRIO_LOW,
  PRIO_NORMAL,
  PRIO_URGENT,
  PRIO_SYSTEM,
} EIB_Priority;

/** EIB address type */
typedef enum
{ GroupAddress, IndividualAddress } EIB_AddrType;


#include "queue.h"
#include "exception.h"

/** add c to s as hex value
 * @param s string
 * @param c byte
 */
void addHex (String & s, uchar c);
/** add c to s as hex value
 * @param s string
 * @param c 16 bit int
 */
void add16Hex (String & s, uint16_t c);

/** get current time */
timestamp_t getTime ();

/** formats an EIB individual address */
String FormatEIBAddr (eibaddr_t a);
/** formats an EIB group address */
String FormatGroupAddr (eibaddr_t a);
/** formats an EIB domain address */
String FormatDomainAddr (domainaddr_t addr);
/** formats an EIB key */
String FormatEIBKey (eibkey_type addr);

#include "trace.h"

template<typename T>
inline T ignore_result(T x __attribute__((unused)))
{
    return x;
}

/** libev */
#if EV_MULTIPLICITY
  typedef struct ev_loop *LOOP_RESULT;
#else
  typedef int LOOP_RESULT;
#endif
extern LOOP_RESULT loop;


#endif
