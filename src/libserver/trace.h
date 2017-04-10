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
#include <fmt/format.h>
#include "common.h"
#include "inifile.h"

#include "config.h"
#if HAVE_FMT_PRINTF
#include <fmt/printf.h>
#endif

#define LEVEL_NONE 0
#define LEVEL_FATAL 1
#define LEVEL_CRITICAL 2
#define LEVEL_ERROR 3
#define LEVEL_WARNING 4
#define LEVEL_NOTICE 5
#define LEVEL_INFO 6
#define LEVEL_DEBUG 7
#define LEVEL_TRACE 8

#define E_FATAL (LEVEL_FATAL<<28)
#define E_CRTIICAL (LEVEL_CRITICAL<<28)
#define E_ERROR (LEVEL_ERROR<<28)
#define E_WARNING (LEVEL_WARNING<<28)
#define E_NOTICE (LEVEL_NOTICE<<28)
#define E_INFO (LEVEL_INFO<<28)

extern unsigned int trace_seq;
extern unsigned int trace_namelen;

/** implements debug output with different levels */
class Trace
{
  /** message levels to print */
  unsigned int layers = 0;
  /** error levels to print */
  unsigned int level = LEVEL_ERROR;
  /** when did we start up? */
  struct timeval started;
  /** print timestamps when tracing */
  bool timestamps = true;

  /** print the common header */
  void TraceHeader (int layer);

  char get_level_char(int level);

  void setup();

public:
  /** set a new name */
  void setAuxName(std::string name);

  IniSectionPtr cfg;

  /** name(s) and number of this tracer */
  std::string servername;
  std::string name;
  std::string auxname;
  std::string fullname();

  unsigned int seq;

  Trace (IniSectionPtr& s, const std::string& sn) : cfg(s->sub("debug")), servername(sn)
  {
    seq = ++trace_seq;
    gettimeofday(&started, NULL);
    name = s->name;
    setup();
  }

  Trace (Trace &orig, std::string name = "") : cfg(orig.cfg), servername(orig.servername)
  {
    this->layers = orig.layers;
    this->level = orig.level;
    this->name = name.length() ? name : orig.name;
    this->started = orig.started;
    this->timestamps = orig.timestamps;
    this->seq = ++trace_seq;
    setup();
  }

  Trace (Trace &orig, IniSectionPtr& s) : cfg(s->sub("debug")), servername(orig.servername)
  {
    this->layers = orig.layers;
    this->level = orig.level;
    this->name = s->name;
    this->started = orig.started;
    this->timestamps = orig.timestamps;
    this->seq = ++trace_seq;
    setup();
  }

  virtual ~Trace ()
  {
  }

  /** sets trace level */
  inline void SetTraceLevel (int l)
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
  inline void TracePacket (int layer, const char *msg, int Len,
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
  inline void TracePacket (int layer, const char *msg, const CArray & c)
  {
    TracePacket (layer, msg, c.size(), c.data());
  }

  /** like printf for this trace
   * @param layer level of the message
   * @param msg Message
   */
  template <typename... Args>
  void TracePrintf (int layer, const char *msg, const Args & ... args)
    {
        TraceHeader(layer);
        fmt::fprintf(stdout, msg, args ...);
        fmt::printf ("\n");
    }

  /** like printf for errors
   * @param msgid message id
   * @param msg Message
   */
  template <typename... Args>
  void ErrorPrintfUncond (unsigned int msgid, const char *msg, const Args & ... args)
    {
      char c = get_level_char((msgid >> 28) & 0x0f); 
      if (servername.length())
        fmt::fprintf(stderr, "%s: ",servername.c_str());
      fmt::fprintf (stderr, "%c%08d: ", c, (msgid & 0xffffff));
      fmt::fprintf (stderr, "[%2d:%s] ", seq, name.c_str());

      fmt::fprintf (stderr, msg, args...); 
      fprintf (stderr, "\n");
    } 


  /** should trace message be written
   * @parm layer level of the message
   * @return bool
   */
  inline bool ShowPrint (int layer)
  {
    return (layers & (1 << layer));
  }

  /** should error message be written
   * @parm msgid level of the message
   * @return bool
   */
  inline bool ShowError (unsigned int msgid)
  {
    return (((msgid >> 28) & 0x0f) <= level);
  }
};

typedef std::shared_ptr<Trace> TracePtr;


#define TRACEPRINTF(trace, layer, msg, args...) do { if ((trace)->ShowPrint(layer)) (trace)->TracePrintf(layer, msg, ##args); } while (0)
#define ERRORPRINTF(trace, msgid, msg, args...) do { \
      if ((trace)->ShowError(msgid)) (trace)->ErrorPrintfUncond(msgid, msg, ##args); \
   } while (0)

#endif
