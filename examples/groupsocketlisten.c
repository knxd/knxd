/*
    EIB Demo program - reads group telegrams
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
  eibaddr_t src;
  uchar buf[200];

  if (ac != 2)
    die ("usage: %s url", ag[0]);
  con = EIBSocketURL (ag[1]);
  if (!con)
    die ("Open failed");

  if (EIBOpen_GroupSocket (con, 0) == -1)
    die ("Connect failed");

  while (1)
    {
      len = EIBGetGroup_Src (con, sizeof (buf), buf, &src, &dest);
      if (len == -1)
	die ("Read failed");
      if (len < 2)
	die ("Invalid Packet");
      if (buf[0] & 0x3 || (buf[1] & 0xC0) == 0xC0)
	{
	  printf ("Unknown APDU from ");
	  printIndividual (src);
	  printf (" to ");
	  printGroup (dest);
	  printf (": ");
	  printHex (len, buf);
	  printf ("\n");
	}
      else
	{
	  switch (buf[1] & 0xC0)
	    {
	    case 0x00:
	      printf ("Read");
	      break;
	    case 0x40:
	      printf ("Response");
	      break;
	    case 0x80:
	      printf ("Write");
	      break;
	    }
	  printf (" from ");
	  printIndividual (src);
	  printf (" to ");
	  printGroup (dest);
	  if (buf[1] & 0xC0)
	    {
	      printf (": ");
	      if (len == 2)
		printf ("%02X", buf[1] & 0x3F);
	      else
		printHex (len - 2, buf + 2);
	    }
	  printf ("\n");
	  fflush (stdout);
	}
    }

  EIBClose (con);
  return 0;
}
