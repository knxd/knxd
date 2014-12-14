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

#ifndef THREADS_H
#define THREADS_H

#include <pthsem.h>

/** interface for a class started by a thread */
class Runable
{
public:
};

/** pointer to an thread entry point in Runable
 * the thread should exit, if stopcond can be deceremented
 */
typedef void (Runable::*THREADENTRY) (pth_sem_t * stopcond);

/** implements a Thread */
class Thread
{
  /** delete at stop */
  bool autodel;
  /** C entry point for the threads */
  static void *ThreadWrapper (void *arg);
  /** thread id */
  pth_t tid;
  /** object to run */
  Runable *obj;
  /** entry point */
  THREADENTRY entry;
  /** stop condition */
  pth_sem_t should_stop;
  /** priority */
  int prio;

protected:
  /** main function of the thread
   * @param stop if stop can be decemented, the routine should exit
   */
    virtual void Run (pth_sem_t * stop);
public:
    /** create a thread
     * if o and t are not present, Run is runned, which has to be replaced
     * @param o Object to run
     * @param t Entry point
     */
    Thread (int Priority = PTH_PRIO_STD, Runable * o = 0, THREADENTRY t = 0);
    virtual ~ Thread ();

    /** starts the thread*/
  void Start ();
  /** stops the thread, if it is running */
  void Stop ();
  /** stops the thread and delete it asynchronous */
  void StopDelete ();
};


#endif
