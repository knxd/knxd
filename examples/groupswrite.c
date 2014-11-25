/*
    EIB Demo program - send A_GroupValue_Write
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
#include "common.h"

int
main (int ac, char *ag[])
{
  int len;
  EIBConnection *con;
  eibaddr_t dest;
  uchar buf[3] = { 0, 0x80 };

  if (ac != 4)
    die ("usage: %s url eibaddr val", ag[0]);
  con = EIBSocketURL (ag[1]);
  if (!con)
    die ("Open failed");
  dest = readgaddr (ag[2]);
  buf[1] |= readHex (ag[3]) & 0x3f;

  if (EIBOpenT_Group (con, dest, 1) == -1)
    die ("Connect failed");

  len = EIBSendAPDU (con, 2, buf);
  if (len == -1)
    die ("Request failed");
  printf ("Send request\n");

  EIBClose (con);
  return 0;
}
