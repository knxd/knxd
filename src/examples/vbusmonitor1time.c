/*
    EIB Demo program - virtual text busmonitor with ISO8601 timestamps
    Copyright (C) 2005-2008 Martin Koegler <mkoegler@auto.tuwien.ac.at>
    Copyright (C) 2010 Michael Markstaller <michael@markstaller.de>

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "common.h"
#include <time.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h> // for gettimeofday
#endif

int
main (int ac, char *ag[])
{
//MM hires-time
  struct timeval tv; 
  struct tm* ptm; 
  char time_string[40]; 
  long milliseconds; 
  /* Obtain the time of day, and convert it to a tm struct. */ 
  gettimeofday (&tv, NULL); 
  ptm = localtime (&tv.tv_sec); 
  /* Format the date and time, down to a single second. */ 
  strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm); 
  /* Compute milliseconds from microseconds. */ 
  milliseconds = tv.tv_usec / 1000; 
  /* Print the formatted time, in seconds, followed by a decimal point and the milliseconds. */ 
  printf ("%s.%03ld\n", time_string, milliseconds); 
//MM end time

  uchar buf[255];
  int len;
  EIBConnection *con;
  if (ac != 2)
    die ("usage: %s url", ag[0]);
  con = EIBSocketURL (ag[1]);
  if (!con)
    die ("Open failed");

  if (EIBOpenVBusmonitorText (con) == -1)
    die ("Open Busmonitor failed");

  while (1)
    {
      len = EIBGetBusmonitorPacket (con, sizeof (buf), buf);
      if (len == -1)
	die ("Read failed");
      buf[len] = 0;
		  gettimeofday (&tv, NULL); 
		  ptm = localtime (&tv.tv_sec); 
		  strftime (time_string, sizeof (time_string), "%H:%M:%S", ptm); 
		  milliseconds = tv.tv_usec / 1000; 
      printf ("%s.%03ld %s\n", time_string, milliseconds, buf);
      fflush (stdout);
    }

  EIBClose (con);
  return 0;
}
/* modiefied time out */
