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
  ClientConnection *con;
  T_BroadcastPtr c;

  void Run (pth_sem_t * stop);
  const char *Name() { return "broadcast"; }
public:
  A_Broadcast (ClientConnection * cc);
  ~A_Broadcast ();

  /** start processing */
  void Do (pth_event_t stop);
};

/** implements client interface to a group connection */
class A_Group:private Thread
{
  ClientConnection *con;
  T_GroupPtr c;

  void Run (pth_sem_t * stop);
  const char *Name() { return "group"; }
public:
  A_Group (ClientConnection * cc);
  ~A_Group ();

  /** start processing */
  void Do (pth_event_t stop);
};

/** implements client interface to a raw connection */
class A_TPDU:private Thread
{
  ClientConnection *con;
  T_TPDUPtr c;

  void Run (pth_sem_t * stop);
  const char *Name() { return "tpdu"; }
public:
  A_TPDU (ClientConnection * cc);
  ~A_TPDU ();

  /** start processing */
  void Do (pth_event_t stop);
};

/** implements client interface to a T_Indivdual connection */
class A_Individual:private Thread
{
  ClientConnection *con;
  T_IndividualPtr c;

  void Run (pth_sem_t * stop);
  const char *Name() { return "individual"; }
public:
  A_Individual (ClientConnection * cc);
  ~A_Individual ();

  /** start processing */
  void Do (pth_event_t stop);
};

/** implements client interface to a T_Connection connection */
class A_Connection:private Thread
{
  ClientConnection *con;
  T_ConnectionPtr c;

  void Run (pth_sem_t * stop);
  const char *Name() { return "connection"; }
public:
  A_Connection (ClientConnection * cc);
  ~A_Connection ();

  /** start processing */
  void Do (pth_event_t stop);
};

/** implements client interface to a group socket */
class A_GroupSocket:private Thread
{
  ClientConnection *con;
  GroupSocketPtr c;

  void Run (pth_sem_t * stop);
  const char *Name() { return "groupsocket"; }
public:
  A_GroupSocket (ClientConnection * cc);
  ~A_GroupSocket ();

  /** start processing */
  void Do (pth_event_t stop);
};

#endif
