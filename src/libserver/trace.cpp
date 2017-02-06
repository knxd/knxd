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
#include <sys/time.h>

#include "trace.h"

static bool trace_started = false;

unsigned int trace_seq = 0;
unsigned int trace_namelen = 3;

void
Trace::TraceHeader (int layer)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  if (tv.tv_usec < started.tv_usec) {
    tv.tv_usec += 1000000;
    tv.tv_sec -= 1;
  }
  tv.tv_usec -= started.tv_usec;
  tv.tv_sec -= started.tv_sec;

  if (!trace_started) {
      trace_started = true;
      setvbuf(stdout, NULL, _IOLBF, 0);
      setvbuf(stderr, NULL, _IOLBF, 0);
  }
  if (servername.length())
    printf("%s: ",servername.c_str());
  if (timestamps)
    printf ("Layer %d [%2d:%-*s %u.%03u] ", layer, seq, trace_namelen, name.c_str(), (unsigned int)tv.tv_sec,(unsigned int)tv.tv_usec/1000);
  else
    printf ("Layer %d [%2d:%s] ", layer, seq, name.c_str());
}

void
Trace::TracePacketUncond (int layer, const char *msg, int Len,
			  const uchar * data)
{
  int i;
  TraceHeader(layer);
  printf ("%s(%03d):", msg, Len);
  for (i = 0; i < Len; i++)
    printf (" %02X", data[i]);
  printf ("\n");
}

void
Trace::TracePrintf (int layer, const char *msg, ...)
{
  va_list ap;
  TraceHeader(layer);
  va_start (ap, msg);
  vprintf (msg, ap);
  printf ("\n");
  va_end (ap);
}

void
Trace::ErrorPrintfUncond (unsigned int msgid, const char *msg, ...)
{
  va_list ap;
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
  if (servername.length())
    fprintf(stderr, "%s: ",servername.c_str());
  fprintf (stderr, "%c%08d: ", c, (msgid & 0xffffff));
  va_start (ap, msg);
  vfprintf (stderr, msg, ap);
  fprintf (stderr, "\n");
  va_end (ap);
}
