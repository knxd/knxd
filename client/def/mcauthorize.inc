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

EIBC_COMPLETE (EIB_MC_Authorize,
  EIBC_GETREQUEST
  EIBC_CHECKRESULT (EIB_MC_AUTHORIZE, 3)
  EIBC_RETURN_UINT8 (2)
)

EIBC_ASYNC (EIB_MC_Authorize, ARG_KEY (key, ARG_NONE),
  EIBC_INIT_SEND (6)
  EIBC_SETKEY (key, 2)
  EIBC_SEND (EIB_MC_AUTHORIZE)
  EIBC_INIT_COMPLETE (EIB_MC_Authorize)
)
