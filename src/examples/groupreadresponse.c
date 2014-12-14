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
#include <sys/time.h>
#include <sys/types.h>

#include "common.h"

int
main (int ac, char *ag[])
{
  int len;
  EIBConnection *con;
  fd_set read;
  eibaddr_t dest;
  eibaddr_t src;
  uchar req_buf[2] = { 0, 0 };
  uchar buf[200];
  struct timeval tv;

  if (ac != 3)
    die ("usage: %s url eibaddr", ag[0]);
  con = EIBSocketURL (ag[1]);
  if (!con)
    die ("Open failed");
  dest = readgaddr (ag[2]);

  if (EIBOpenT_Group (con, dest, 0) == -1)
    die ("Connect failed");

  len = EIBSendAPDU (con, 2, req_buf);
  if (len == -1)
    die ("Request failed");
  printf ("Send request\n");

  while (1)
    {
      tv.tv_usec=0;
      tv.tv_sec=10;
    lp:
      FD_ZERO (&read);
      FD_SET (EIB_Poll_FD (con), &read);

      if (select (EIB_Poll_FD (con) + 1, &read, 0, 0, &tv) == -1)
	die ("select failed");

      len = EIB_Poll_Complete (con);
      if (len == -1)
	die ("Read failed");
      if (len == 0)
	{
	  if(tv.tv_sec == 0 && tv.tv_usec==0)
	    goto end;
	goto lp;
	}

      len = EIBGetAPDU_Src (con, sizeof (buf), buf, &src);
      if (len == -1)
	die ("Read failed");
      if (len < 2)
	die ("Invalid Packet");
      if (buf[0] & 0x3 || (buf[1] & 0xC0) == 0xC0)
	{
	  printf ("Unknown APDU from ");
	  printIndividual (src);
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
	  if (buf[1] & 0xC0)
	    {
	      printf (": ");
	      if (len == 2)
		printf ("%02X", buf[1] & 0x3F);
	      else
		printHex (len - 2, buf + 2);
	    }
	  printf ("\n");
	  switch (buf[1] & 0xC0)
	    {
	    case 0x80:
	    case 0x40:
	      goto end;
	    }
	}
    }
 end:
  EIBClose (con);
  return 0;
}
