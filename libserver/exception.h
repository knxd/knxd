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

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "string.h"

/** exception cause */
typedef enum
{
  /** inconsistent PDU */
  PDU_WRONG_FORMAT,
  /** inconsistent size in a PDU */
  PDU_INCONSISTENT_SIZE,
  /** initialization failed */
  DEV_OPEN_FAIL,
  /** runtime system error */
  DEV_SYSTEM_ERROR,
  /** initialisation of a Layer 4 connection failed */
  L4_INIT_FAIL,
  /** no response on Layer 4 connection */
  L4_NO_RESPONSE,
} Exception_Type;

/** represents an exception */
class Exception
{
  /** stores exception code*/
  Exception_Type ex;
public:

  /** initialises exception with code t */
  Exception (Exception_Type t)
    {
      ex=t;
    }

  /** returns exception code */
  Exception_Type getException()
    {
      return ex;
    }
};


#endif
