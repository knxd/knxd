/*
    EIB Demo program - set a key in a BCU
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
  int level, k;
  EIBConnection *con;
  eibaddr_t dest;
  uint8_t key[4];
  char *prog = ag[0];

  parseKey (&ac, &ag);
  if (ac != 5)
    die ("usage: %s [-k key] url eibaddr level key", prog);
  con = EIBSocketURL (ag[1]);
  if (!con)
    die ("Open failed");
  dest = readaddr (ag[2]);
  level = atoi (ag[3]);
  sscanf (ag[4], "%x", &k);
  key[0] = (k >> 24) & 0xff;
  key[1] = (k >> 16) & 0xff;
  key[2] = (k >> 8) & 0xff;
  key[3] = (k >> 0) & 0xff;

  if (EIB_MC_Connect (con, dest) == -1)
    die ("Connect failed");
  auth (con);

  k = EIB_MC_SetKey (con, key, level);
  if (k == -1)
    die ("SetKey failed");

  EIBClose (con);
  return 0;
}
