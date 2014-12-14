EIBC_LICENSE(
/*
    EIBD client library
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    In addition to the permissions in the GNU General Public License, 
    you may link the compiled version of this file into combinations
    with other programs, and distribute those combinations without any 
    restriction coming from the use of this file. (The General Public 
    License restrictions do apply in other respects; for example, they 
    cover modification of the file, and distribution when not linked into 
    a combine executable.)

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
)

EIBC_COMPLETE(EIB_M_WriteIndividualAddress,
  EIBC_GETREQUEST
  EIBC_RETURNERROR (EIB_ERROR_ADDR_EXISTS, EADDRINUSE)
  EIBC_RETURNERROR (EIB_ERROR_TIMEOUT, ETIMEDOUT)
  EIBC_RETURNERROR (EIB_ERROR_MORE_DEVICE, EADDRNOTAVAIL)
  EIBC_CHECKRESULT (EIB_M_INDIVIDUAL_ADDRESS_WRITE, 2)
  EIBC_RETURN_OK
)

EIBC_ASYNC (EIB_M_WriteIndividualAddress, ARG_ADDR (dest, ARG_NONE),
  EIBC_INIT_SEND (4)
  EIBC_SETADDR (dest, 2)
  EIBC_SEND (EIB_M_INDIVIDUAL_ADDRESS_WRITE)
  EIBC_INIT_COMPLETE (EIB_M_WriteIndividualAddress)
)
