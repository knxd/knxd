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

#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include "layer3.h"

class ClientConnection;
/** implements the frontend (but opens no connection) */
class Server:protected Thread
{
  /** Layer 3 interface */
  Layer3 *l3;
  /** open client connections*/
    Array < ClientConnection * >connections;

  void Run (pth_sem_t * stop);
protected:
  /** debug output */
    Trace * t;
    /** server socket */
  int fd;

  virtual void setupConnection (int cfd);

    Server (Layer3 * l3, Trace * tr);
public:
    virtual ~ Server ();

  virtual bool init () = 0;
    /** deregister client connection */
  bool deregister (ClientConnection * con);
};

#endif
