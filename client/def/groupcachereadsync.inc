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

EIBC_COMPLETE (EIB_Cache_Read_Sync,
  EIBC_GETREQUEST
  EIBC_CHECKRESULT (EIB_CACHE_READ, 2)
  EIBC_RETURNERROR_UINT16 (4, ENODEV)
  EIBC_RETURNERROR_SIZE (6, ENOENT)
  EIBC_RETURN_PTR5 (2)
  EIBC_RETURN_BUF (6)
)

EIBC_ASYNC (EIB_Cache_Read_Sync, ARG_ADDR (dst, ARG_OUTADDR (src, ARG_OUTBUF (buf, ARG_UINT16 (age, ARG_NONE)))),
  EIBC_INIT_SEND (6)
  EIBC_READ_BUF (buf)
  EIBC_PTR5 (src)
  EIBC_SETADDR (dst, 2)
  EIBC_SETUINT16 (age, 4)
  EIBC_SEND (EIB_CACHE_READ)
  EIBC_INIT_COMPLETE (EIB_Cache_Read_Sync)
)
