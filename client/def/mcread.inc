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

EIBC_COMPLETE (EIB_MC_Read,
  EIBC_GETREQUEST
  EIBC_CHECKRESULT (EIB_MC_READ, 2)
  EIBC_RETURN_BUF (2)
)

EIBC_ASYNC (EIB_MC_Read, ARG_UINT16 (addr, ARG_OUTBUF_LEN (buf, ARG_NONE)),
  EIBC_INIT_SEND (6)
  EIBC_READ_BUF (buf)
  EIBC_SETUINT16 (addr, 2)
  EIBC_SETLEN (EIBC_READ_LEN (buf), 4)
  EIBC_SEND (EIB_MC_READ)
  EIBC_INIT_COMPLETE (EIB_MC_Read)
)
