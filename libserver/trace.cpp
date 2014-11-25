/*
    EIBD eib bus access and management daemon
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

#include <stdio.h>
#include <stdarg.h>

#include "trace.h"

void
Trace::TracePacketUncond (int layer, void *inst, const char *msg, int Len,
			  const uchar * data)
{
  int i;
  int t = time (0);
  printf ("Layer %d(%08X,%08X) %s(%03d):", layer, (unsigned long) inst, t,
	  msg, Len);
  for (i = 0; i < Len; i++)
    printf (" %02X", data[i]);
  printf ("\n");
}

void
Trace::TracePrintf (int layer, void *inst, const char *msg, ...)
{
  va_list ap;
  int t = time (0);
  printf ("Layer %d(%08X,%08X) ", layer, (unsigned long) inst, t);
  va_start (ap, msg);
  vprintf (msg, ap);
  printf ("\n");
  va_end (ap);
}

void
Trace::ErrorPrintfUncond (unsigned int msgid, const char *msg, ...)
{
  va_list ap;
  int t = time (0);
  char c;
  switch ((msgid >> 28) & 0x0f)
    {
    case LEVEL_FATAL:
      c = 'F';
      break;
    case LEVEL_CRITICAL:
      c = 'C';
      break;
    case LEVEL_ERROR:
      c = 'E';
      break;
    case LEVEL_WARNING:
      c = 'W';
      break;
    case LEVEL_NOTICE:
      c = 'N';
      break;
    case LEVEL_INFO:
      c = 'I';
      break;
    default:
      c = '?';
    }
  fprintf (stderr, "%c%08d: ", c, (msgid & 0xffffff));
  va_start (ap, msg);
  vfprintf (stderr, msg, ap);
  fprintf (stderr, "\n");
  va_end (ap);
}
