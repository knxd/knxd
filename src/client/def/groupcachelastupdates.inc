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

EIBC_COMPLETE (EIB_Cache_LastUpdates,
  EIBC_GETREQUEST
  EIBC_CHECKRESULT (EIB_CACHE_LAST_UPDATES, 2)
  EIBC_RETURN_PTR4 (2)
  EIBC_RETURN_BUF (4)
)

EIBC_ASYNC (EIB_Cache_LastUpdates, ARG_UINT16 (start, ARG_UINT8 (timeout, ARG_OUTBUF (buf, ARG_OUTUINT16 (ende, ARG_NONE)))),
  EIBC_INIT_SEND (5)
  EIBC_READ_BUF (buf)
  EIBC_PTR4 (ende)
  EIBC_SETUINT16 (start, 2)
  EIBC_SETUINT8 (timeout, 4)
  EIBC_SEND (EIB_CACHE_LAST_UPDATES)
  EIBC_INIT_COMPLETE (EIB_Cache_LastUpdates)
)

EIBC_COMPLETE (EIB_Cache_LastUpdates2,
  EIBC_GETREQUEST
  EIBC_CHECKRESULT (EIB_CACHE_LAST_UPDATES_2, 4)
  EIBC_RETURN_PTR7 (2)
  EIBC_RETURN_BUF (6)
)

EIBC_ASYNC (EIB_Cache_LastUpdates2, ARG_UINT32 (start, ARG_UINT8 (timeout, ARG_OUTBUF (buf, ARG_OUTUINT32 (ende, ARG_NONE)))),
  EIBC_INIT_SEND (7)
  EIBC_READ_BUF (buf)
  EIBC_PTR7 (ende)
  EIBC_SETUINT32 (start, 2)
  EIBC_SETUINT8 (timeout, 6)
  EIBC_SEND (EIB_CACHE_LAST_UPDATES_2)
  EIBC_INIT_COMPLETE (EIB_Cache_LastUpdates2)
)
