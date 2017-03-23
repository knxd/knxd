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

std::string Trace::fullname()
{
  if (!auxname.length())
    return name;
  return name+'/'+auxname;
}

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
    fmt::printf("%s: ",servername);
  if (timestamps)
    fmt::printf ("Layer %d [%2d:%-*s %u.%03u] ", layer, seq, trace_namelen, fullname(), (unsigned int)tv.tv_sec,(unsigned int)tv.tv_usec/1000);
  else
    fmt::printf ("Layer %d [%2d:%s] ", layer, seq, fullname());
}

void
Trace::TracePacketUncond (int layer, const char *msg, int Len,
			  const uchar * data)
{
  int i;
  TraceHeader(layer);
  fmt::printf ("%s(%03d):", msg, Len);
  for (i = 0; i < Len; i++)
    fmt::printf (" %02X", data[i]);
  fmt::printf ("\n");
}

static const char *error_levels[] = {
    "none",
    "fatal",
    "error",
    "warning",
    "note",
    "info",
    "debug",
    "trace",
};

static int
error_level(std::string level, int def)
{
  if (level.size() == 0)
    return def;
  if(isdigit(level[0]))
    return strtoul(level.c_str(), NULL, 0);
  for(unsigned int i = 0; i < sizeof(error_levels)/sizeof(error_levels[0]); i++)
    if (level == error_levels[i])
      return i;
  return -1; // warning
}

void
Trace::setup()
{
  if (trace_namelen < this->name.length())
    trace_namelen = this->name.length();
  timestamps = cfg->value("timestamps",timestamps);
  layers = cfg->value("trace-mask",(int)layers);
  int nlevel = error_level(cfg->value("error-level",""),level);
  if (nlevel == -1)
    {
      std::cerr << "Unrecognized logging level: " << cfg->value("error-level","") << std::endl;
      return;
    }
  level = nlevel;
  setAuxName(cfg->value("name",name));
}

void
Trace::setAuxName(std::string name)
{
  if (name == this->name)
    return;

  unsigned int len = this->name.length()+auxname.length()+(auxname.length()>0);
  this->auxname = name;

  if (trace_namelen < len)
    trace_namelen = len;
}

char
Trace::get_level_char(int level)
{
  switch (level)
    {
    case LEVEL_NONE:
      return 'X';
    case LEVEL_FATAL:
      return 'F';
    case LEVEL_CRITICAL:
      return 'C';
    case LEVEL_ERROR:
      return 'E';
    case LEVEL_WARNING:
      return 'W';
    case LEVEL_NOTICE:
      return 'N';
    case LEVEL_INFO:
      return 'I';
    case LEVEL_DEBUG:
      return 'D';
    case LEVEL_TRACE:
      return 'T';
    default:
      return '?';
    }
}
