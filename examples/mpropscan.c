/*
    EIB Demo program - list properties
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
  uchar buf[10000];
  int len;
  int i;
  EIBConnection *con;
  eibaddr_t dest;
  char *prog = ag[0];

  parseKey (&ac, &ag);
  if (ac != 3)
    die ("usage: %s [-k key] url eibaddr", prog);
  con = EIBSocketURL (ag[1]);
  if (!con)
    die ("Open failed");
  dest = readaddr (ag[2]);

  if (EIB_MC_Connect (con, dest) == -1)
    die ("Connect failed");
  auth (con);

  len = EIB_MC_PropertyScan (con, sizeof (buf), buf);
  if (len == -1)
    die ("Read failed");
  for (i = 0; i < len; i += 6)
    if (buf[i + 1] == 1 && buf[i + 2] == 4)
      printf ("Obj: %d Property: %d Type: %d Objtype:%d Access:%02X\n",
	      buf[i + 0], buf[i + 1], buf[i + 2],
	      (buf[i + 3] << 8) | buf[i + 4], buf[i + 5]);
    else
      printf ("Obj: %d Property: %d Type: %d Count:%d Access:%02X\n",
	      buf[i + 0], buf[i + 1], buf[i + 2],
	      (buf[i + 3] << 8) | buf[i + 4], buf[i + 5]);

  EIBClose (con);
  return 0;
}
