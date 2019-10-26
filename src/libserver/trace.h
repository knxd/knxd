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

/**
 * @file
 * @addtogroup Common
 * @{
 */

#ifndef TRACE_H
#define TRACE_H

#include <cstdarg>
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

enum trace_level
{
  LEVEL_NONE = 0,
  LEVEL_FATAL = 1,
  LEVEL_CRITICAL = 2,
  LEVEL_ERROR = 3,
  LEVEL_WARNING = 4,
  LEVEL_NOTICE = 5,
  LEVEL_INFO = 6,
  LEVEL_DEBUG = 7,
  LEVEL_TRACE = 8
};

constexpr unsigned int E_FATAL = LEVEL_FATAL<<28;
constexpr unsigned int E_CRTIICAL = LEVEL_CRITICAL<<28;
constexpr unsigned int E_ERROR = LEVEL_ERROR<<28;
constexpr unsigned int E_WARNING = LEVEL_WARNING<<28;
constexpr unsigned int E_NOTICE = LEVEL_NOTICE<<28;
constexpr unsigned int E_INFO = LEVEL_INFO<<28;

extern unsigned int trace_seq;
extern unsigned int trace_namelen;

/** implements debug output with different levels */
class Trace
{
public:
  /** set a new name */
  void setAuxName(const std::string name);

  IniSectionPtr cfg;

  /** name(s) and number of this tracer */
  std::string servername;
  std::string name;
  std::string auxname;
  std::string fullname() const;

  unsigned int seq;

  Trace (IniSectionPtr& s, const std::string& sn) :
    cfg(s->sub("debug")),
    servername(sn),
    name(s->name)
  {
    seq = ++trace_seq;
    gettimeofday(&started, NULL);
    setup();
  }

  Trace (Trace &orig, std::string name = "") :
    cfg(orig.cfg),
    servername(orig.servername),
    layers(orig.layers),
    level(orig.level),
    name(name.length() ? name : orig.name),
    started(orig.started),
    timestamps(orig.timestamps)
  {
    seq = ++trace_seq;
    setup();
  }

  Trace (Trace &orig, IniSectionPtr& s) :
    cfg(s->sub("debug")),
    servername(orig.servername),
    layers(orig.layers),
    level(orig.level),
    name(s->name),
    started(orig.started),
    timestamps(orig.timestamps)
  {
    seq = ++trace_seq;
    setup();
  }

  virtual ~Trace () = default;

  /** sets trace level */
  inline void SetTraceLevel (const int l)
  {
    layers = l;
  }

  void SetTimestamps (const bool l)
  {
    timestamps = l;
  }

  /** sets error level */
  void SetErrorLevel (const int l)
  {
    level = l;
  }

  /**
   * prints a message with a hex dump unconditional
   * @param layer level of the message
   * @param msg Message
   * @param Len length of the data
   * @param data pointer to the data
   */
  void TracePacketUncond (const int layer, const char *msg,
                          const int Len, const uint8_t * data);
  /**
   * prints a message with a hex dump
   * @param layer level of the message
   * @param msg Message
   * @param Len length of the data
   * @param data pointer to the data
   */
  inline void TracePacket (const int layer, const char *msg, const int Len,
                           const uint8_t * data)
  {
    if (!ShowPrint (layer))
      return;
    TracePacketUncond (layer, msg, Len, data);
  }

  /**
   * prints a message with a hex dump
   * @param layer level of the message
   * @param msg Message
   * @param c array with the data
   */
  inline void TracePacket (const int layer, const char *msg, const CArray & c)
  {
    TracePacket (layer, msg, c.size(), c.data());
  }

  /**
   * like printf for this trace
   * @param layer level of the message
   * @param msg Message
   */
  template <typename... Args>
  void TracePrintf (const int layer, const char *msg, const Args & ... args)
  {
    TraceHeader(layer);
    fmt::fprintf(stdout, msg, args ...);
    fmt::printf ("\n");
  }

  /**
   * like printf for errors
   * @param msgid message id
   * @param msg Message
   */
  template <typename... Args>
  void ErrorPrintfUncond (const unsigned int msgid, const char *msg, const Args & ... args)
  {
    char c = get_level_char((msgid >> 28) & 0x0f);
    if (servername.length())
      fmt::fprintf(stderr, "%s: ",servername.c_str());
    fmt::fprintf (stderr, "%c%08d: ", c, (msgid & 0xffffff));
    fmt::fprintf (stderr, "[%2d:%s] ", seq, name.c_str());

    fmt::fprintf (stderr, msg, args...);
    fprintf (stderr, "\n");
  }


  /**
   * should trace message be written
   * @parm layer level of the message
   * @return bool
   */
  inline bool ShowPrint (const int layer) const
  {
    return (layers & (1 << layer));
  }

  /**
   * should error message be written
   * @parm msgid level of the message
   * @return bool
   */
  inline bool ShowError (const unsigned int msgid) const
  {
    return (((msgid >> 28) & 0x0f) <= level);
  }

private:
  /** message levels to print */
  unsigned int layers = 0;
  /** error levels to print */
  unsigned int level = LEVEL_ERROR;
  /** when did we start up? */
  struct timeval started;
  /** print timestamps when tracing */
  bool timestamps = true;

  /** print the common header */
  void TraceHeader (const int layer);

  char get_level_char(const int level) const;

  void setup();
};

using TracePtr = std::shared_ptr<Trace>;

#define TRACEPRINTF(trace, layer, msg, args...) do { \
      if ((trace)->ShowPrint(layer)) (trace)->TracePrintf(layer, msg, ##args); \
   } while (0)
#define ERRORPRINTF(trace, msgid, msg, args...) do { \
      if ((trace)->ShowError(msgid)) (trace)->ErrorPrintfUncond(msgid, msg, ##args); \
   } while (0)

#endif

/** @} */
