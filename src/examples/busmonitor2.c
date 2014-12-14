/*
    EIB Demo program - binary busmonitor
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
  uchar buf[255];
  int len;
  EIBConnection *con;
  if (ac != 2)
    die ("usage: %s url", ag[0]);
  con = EIBSocketURL (ag[1]);
  if (!con)
    die ("Open failed");

  if (EIBOpenBusmonitor (con) == -1)
    die ("Open Busmonitor failed");

  while (1)
    {
      len = EIBGetBusmonitorPacket (con, sizeof (buf), buf);
      if (len == -1)
	die ("Read failed");
      printHex (len, buf);
      printf ("\n");
      fflush (stdout);
    }

  EIBClose (con);
  return 0;
}
