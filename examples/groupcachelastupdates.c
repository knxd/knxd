/*
    EIB Demo program
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
  int i;
  int start;
  int timeout;
  uint16_t end;
  uchar buf[300];

  if (ac != 4)
    die ("usage: %s url start-position timeout", ag[0]);
  con = EIBSocketURL (ag[1]);
  if (!con)
    die ("Open failed");
  start = atoi (ag[2]);
  timeout = atoi (ag[3]);

  len = EIB_Cache_LastUpdates (con, start, timeout, sizeof (buf), buf, &end);
  if (len == -1)
    die ("Read failed");

  printf ("new position: %d\n", end);
  for (i = 0; i < len; i += 2)
    {
      eibaddr_t a = (buf[i] << 8) | buf[i + 1];
      printGroup (a);
      printf ("\n");
    }
  printf ("\n");

  EIBClose (con);
  return 0;
}
