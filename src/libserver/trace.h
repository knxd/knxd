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

#ifndef TRACE_H
#define TRACE_H

#include <stdarg.h>
#include <sys/time.h>
#include <memory>
#include <iostream>
#include "common.h"
#include "inifile.h"

#define LEVEL_FATAL 0
#define LEVEL_CRITICAL 1
#define LEVEL_ERROR 2
#define LEVEL_WARNING 3
#define LEVEL_NOTICE 4
#define LEVEL_INFO 5

#define E_FATAL (LEVEL_FATAL<<28)
#define E_CRTIICAL (LEVEL_CRITICAL<<28)
#define E_ERROR (LEVEL_ERROR<<28)
#define E_WARNING (LEVEL_WARNING<<28)
#define E_NOTICE (LEVEL_NOTICE<<28)
#define E_INFO (LEVEL_INFO<<28)

extern unsigned int trace_seq;
extern unsigned int trace_namelen;

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
    return atoi(level.c_str());
  for(int i = 0; i < sizeof(error_levels)/sizeof(error_levels[0]); i++)
    if (level == error_levels[i])
      return i;
  std::cerr << "Unrecognized logging level: " << level << std::endl;
  return 3; // warning
}

/** implements debug output with different levels */
class Trace
{
  /** message levels to print */
  unsigned int layers = 0;
  /** error levels to print */
  unsigned int level = 3;
  /** when did we start up? */
  struct timeval started;
  /** print timestamps when tracing */
  bool timestamps = true;

  /** print the common header */
  void TraceHeader (int layer);

public:
  /** name(s) and number of this tracer */
  std::string servername;
  std::string name;
  unsigned int seq;

  IniSection &cfg;

  Trace (IniSection& s, std::string& sn) : cfg(s), servername(sn)
  {
    seq = ++trace_seq;
    gettimeofday(&started, NULL);
    name = s.name;
  }
  bool setup(bool quiet=false);

  Trace (Trace &orig, std::string name) : cfg(orig.cfg)
  {
    this->layers = orig.layers;
    this->level = orig.level;
    this->name = name;
    this->started = orig.started;
    this->timestamps = orig.timestamps;
    this->seq = ++trace_seq;
    if (trace_namelen < this->name.length())
      trace_namelen = this->name.length();
  }

  Trace (Trace &orig, IniSection& s) : cfg(s)
  {
    this->layers = orig.layers;
    this->level = orig.level;
    this->name = name;
    this->started = orig.started;
    this->timestamps = orig.timestamps;
    this->seq = ++trace_seq;
    if (trace_namelen < this->name.length())
      trace_namelen = this->name.length();
    setup();
  }

  ~Trace ()
  {
  }

  /** sets trace level */
  void SetTraceLevel (int l)
  {
    layers = l;
  }

  void SetTimestamps (bool l)
  {
    timestamps = l;
  }

  /** sets error level */
  void SetErrorLevel (int l)
  {
    level = l;
  }

  /** prints a message with a hex dump unconditional
   * @param layer level of the message
   * @param msg Message
   * @param Len length of the data
   * @param data pointer to the data
   */
  void TracePacketUncond (int layer, const char *msg,
				  int Len, const uchar * data);
  /** prints a message with a hex dump
   * @param layer level of the message
   * @param msg Message
   * @param Len length of the data
   * @param data pointer to the data
   */
  void TracePacket (int layer, const char *msg, int Len,
		    const uchar * data)
  {
    if (!ShowPrint (layer))
      return;
    TracePacketUncond (layer, msg, Len, data);
  }
  /** prints a message with a hex dump
   * @param layer level of the message
   * @param msg Message
   * @param c array with the data
   */
  void TracePacket (int layer, const char *msg, const CArray & c)
  {
    TracePacket (layer, msg, c.size(), c.data());
  }

  /** like printf for this trace
   * @param layer level of the message
   * @param msg Message
   */
  void TracePrintf (int layer, const char *msg, ...);

  /** like printf for errors
   * @param msgid message id
   * @param msg Message
   */
  void ErrorPrintfUncond (unsigned int msgid, const char *msg, ...);

  /** should trace message be written
   * @parm layer level of the message
   * @return bool
   */
  bool ShowPrint (int layer)
  {
    if (layers & (1 << layer))
      return 1;
    else
      return 0;
  }

  /** should error message be written
   * @parm msgid level of the message
   * @return bool
   */
  bool ShowError (unsigned int msgid)
  {
    if (((msgid >> 28) & 0x0f) <= level)
      return 1;
    else
      return 0;
  }
};

typedef std::shared_ptr<Trace> TracePtr;


#define TRACEPRINTF(trace, layer, msg, args...) do { if ((trace)->ShowPrint(layer)) (trace)->TracePrintf(layer, msg, ##args); } while (0)
#define ERRORPRINTF(trace, msgid, msg, args...) do { \
      if ((trace)->ShowError(msgid)) (trace)->ErrorPrintfUncond(msgid, msg, ##args); \
   } while (0)

#endif
