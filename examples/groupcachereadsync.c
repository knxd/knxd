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
  uint16_t age = 0;
  EIBConnection *con;
  eibaddr_t dest;
  eibaddr_t src;
  uchar buf[200];

  if (ac != 3 && ac != 4)
    die ("usage: %s url eibaddr [age]", ag[0]);
  con = EIBSocketURL (ag[1]);
  if (!con)
    die ("Open failed");
  dest = readgaddr (ag[2]);
  if (ac == 4)
    age = atoi (ag[3]);

  len = EIB_Cache_Read_Sync (con, dest, &src, sizeof (buf), buf, age);
  if (len == -1)
    die ("Read failed");

  switch (buf[1] & 0xC0)
    {
    case 0x40:
      printf ("Response");
      break;
    case 0x80:
      printf ("Write");
      break;
    }
  printf (" from ");
  printIndividual (src);
  if (buf[1] & 0xC0)
    {
      printf (": ");
      if (len == 2)
	printf ("%02X", buf[1] & 0x3F);
      else
	printHex (len - 2, buf + 2);
    }
  printf ("\n");

  EIBClose (con);
  return 0;
}
