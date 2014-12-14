/*
    EIB Demo program - read property
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
  int len, obj, prop, start, nr_of_elem;
  EIBConnection *con;
  uchar buf[255];
  eibaddr_t dest;
  char *prog = ag[0];

  if (ac != 7)
    die ("usage: %s url eibaddr obj prop start nr_of_elem", prog);
  con = EIBSocketURL (ag[1]);
  if (!con)
    die ("Open failed");
  dest = readaddr (ag[2]);
  obj = atoi (ag[3]);
  prop = atoi (ag[4]);
  start = atoi (ag[5]);
  nr_of_elem = atoi (ag[6]);

  if (EIB_MC_Individual_Open (con, dest) == -1)
    die ("Connect failed");

  len =
    EIB_MC_PropertyRead (con, obj, prop, start, nr_of_elem, sizeof (buf),
			 buf);
  if (len == -1)
    die ("Read failed");
  printHex (len, buf);

  EIBClose (con);
  return 0;
}
