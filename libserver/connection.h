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

#ifndef CONNECTION_H
#define CONNECTION_H

#include "layer4.h"
#include "client.h"

/** implements client interface to a broadcast connection */
class A_Broadcast:private Thread
{
  Layer3 *layer3;
  Trace *t;
  ClientConnection *con;
  T_Broadcast *c;

  void Run (pth_sem_t * stop);
public:
    A_Broadcast (Layer3 * l3, Trace * tr, ClientConnection * cc);
   ~A_Broadcast ();

   /** start processing */
  void Do (pth_event_t stop);
};

/** implements client interface to a group connection */
class A_Group:private Thread
{
  Layer3 *layer3;
  Trace *t;
  ClientConnection *con;
  T_Group *c;

  void Run (pth_sem_t * stop);
public:
    A_Group (Layer3 * l3, Trace * tr, ClientConnection * cc);
   ~A_Group ();

   /** start processing */
  void Do (pth_event_t stop);
};

/** implements client interface to a raw connection */
class A_TPDU:private Thread
{
  Layer3 *layer3;
  Trace *t;
  ClientConnection *con;
  T_TPDU *c;

  void Run (pth_sem_t * stop);
public:
    A_TPDU (Layer3 * l3, Trace * tr, ClientConnection * cc);
   ~A_TPDU ();

   /** start processing */
  void Do (pth_event_t stop);
};

/** implements client interface to a T_Indivdual connection */
class A_Individual:private Thread
{
  Layer3 *layer3;
  Trace *t;
  ClientConnection *con;
  T_Individual *c;

  void Run (pth_sem_t * stop);
public:
    A_Individual (Layer3 * l3, Trace * tr, ClientConnection * cc);
   ~A_Individual ();

   /** start processing */
  void Do (pth_event_t stop);
};

/** implements client interface to a T_Connection connection */
class A_Connection:private Thread
{
  Layer3 *layer3;
  Trace *t;
  ClientConnection *con;
  T_Connection *c;

  void Run (pth_sem_t * stop);
public:
    A_Connection (Layer3 * l3, Trace * tr, ClientConnection * cc);
   ~A_Connection ();

   /** start processing */
  void Do (pth_event_t stop);
};

/** implements client interface to a group socket */
class A_GroupSocket:private Thread
{
  Layer3 *layer3;
  Trace *t;
  ClientConnection *con;
  GroupSocket *c;

  void Run (pth_sem_t * stop);
public:
    A_GroupSocket (Layer3 * l3, Trace * tr, ClientConnection * cc);
   ~A_GroupSocket ();

   /** start processing */
  void Do (pth_event_t stop);
};

#endif
