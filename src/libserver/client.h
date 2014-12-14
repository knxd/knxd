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

#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include "eibtypes.h"

/** reads the type of a eibd packet */
#define EIBTYPE(buf) (((buf)[0]<<8)|((buf)[1]))
/** sets the type of a eibd packet*/
#define EIBSETTYPE(buf,type) do{(buf)[0]=(type>>8)&0xff;(buf)[1]=(type)&0xff;}while(0)

class Server;
class Layer3;
/** implements a client connection */
class ClientConnection:public Thread
{
  /** client connection */
  int fd;
  /** Layer 3 interface */
  Layer3 *l3;
  /** debug output */
  Trace *t;
  /** server */
  Server *s;
  /** buffer length*/
  unsigned buflen;

  void Run (pth_sem_t * stop);
public:
    ClientConnection (Server * s, Layer3 * l3, Trace * tr, int fd);
    virtual ~ ClientConnection ();
    /** reads a message and stores it in buf; aborts if stop occurs */
  int readmessage (pth_event_t stop);
  /** send a message and aborts if stop occurs */
  int sendmessage (int size, const uchar * msg, pth_event_t stop);
  /** send a reject; aborts if stop occurs */
  int sendreject (pth_event_t stop);
  /** sends a reject with the code code; aborts, if stop occurs */
  int sendreject (pth_event_t stop, int code);


  /** buffer*/
  uchar *buf;
  /** message length */
  unsigned size;
};

#endif
