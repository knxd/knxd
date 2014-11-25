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
#include "common.h"

#define TRACE_LEVEL_0 0x01
#define TRACE_LEVEL_1 0x02
#define TRACE_LEVEL_2 0x04
#define TRACE_LEVEL_3 0x08
#define TRACE_LEVEL_4 0x10
#define TRACE_LEVEL_5 0x20
#define TRACE_LEVEL_6 0x40
#define TRACE_LEVEL_7 0x80

#define LEVEL_FATAL 0
#define LEVEL_CRITICAL 1
#define LEVEL_ERROR 2
#define LEVEL_WARNING 3
#define LEVEL_NOTICE 4
#define LEVEL_INFO 5

/** implements debug output with different levels */
class Trace
{
  /** message levels to print */
  int layers;
  /** error levels to print */
  int level;
public:
    Trace ()
  {
    layers = 0;
    level = 0;
  }
  virtual ~ Trace ()
  {
  }

  /** sets trace level */
  virtual void SetTraceLevel (int l)
  {
    layers = l;
  }

  /** sets error level */
  virtual void SetErrorLevel (int l)
  {
    level = l;
  }

  /** prints a message with a hex dump unconditional
   * @param layer level of the message
   * @param inst pointer to the source
   * @param msg Message
   * @param Len length of the data
   * @param data pointer to the data
   */
  virtual void TracePacketUncond (int layer, void *inst, const char *msg,
				  int Len, const uchar * data);
  /** prints a message with a hex dump
   * @param layer level of the message
   * @param inst pointer to the source
   * @param msg Message
   * @param Len length of the data
   * @param data pointer to the data
   */
  void TracePacket (int layer, void *inst, const char *msg, int Len,
		    const uchar * data)
  {
    if (!ShowPrint (layer))
      return;
    TracePacketUncond (layer, inst, msg, Len, data);
  }
  /** prints a message with a hex dump
   * @param layer level of the message
   * @param inst pointer to the source
   * @param msg Message
   * @param c array with the data
   */
  void TracePacket (int layer, void *inst, const char *msg, const CArray & c)
  {
    TracePacket (layer, inst, msg, c (), c.array ());
  }

  /** like printf for this trace
   * @param layer level of the message
   * @param inst pointer to the source
   * @param msg Message
   */
  virtual void TracePrintf (int layer, void *inst, const char *msg, ...);

  /** like printf for errors
   * @param msgid message id
   * @param msg Message
   */
  virtual void ErrorPrintfUncond (unsigned int msgid, const char *msg, ...);

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

#define TRACEPRINTF(trace, layer, inst, msg, args...) do { if ((trace)->ShowPrint(layer)) (trace)->TracePrintf(layer, inst, msg, ##args); } while (0)
#define ERRORPRINTF(trace, msgid, inst, msg, args...) do { \
      if ((trace)->ShowPrint(((msgid)>>24)&0x0f)) (trace)->TracePrintf(((msgid)>>24)&0x0f, inst, msg, ##args); \
      if ((trace)->ShowError(msgid)) (trace)->ErrorPrintfUncond(msgid, msg, ##args); \
   } while (0)

#endif
