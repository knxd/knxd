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
 * This 
 */

#ifndef IOBUF_H
#define IOBUF_H

#include "types.h"
#include "callbacks.h"
#include <assert.h>
#include <ev++.h>
#include <queue.h>

void set_non_blocking(int fd);

class SendBuf
{
  ev::io io;
  void io_cb (ev::io &w, int revents);

public:
  error_cb on_error_cb;
  void on_error() {}

  SendBuf() {} // dead

  SendBuf(int fd) {
    init(fd);
  }
  void init(int fd) {
    assert (this->fd == -1);
    set_non_blocking(fd);
    this->fd = fd;
    io.set<SendBuf, &SendBuf::io_cb>(this);
    on_error_cb.set<SendBuf,&SendBuf::on_error>(this);
  };

  virtual ~SendBuf() {
    while (!sendqueue.isempty()) {
      const CArray *buf = sendqueue.get();
      delete buf;
    }
    if (sendbuf)
      delete sendbuf;
  };

  void start();
  void stop();

  void write(const uchar *buf, size_t len) {
    CArray *data = new CArray(buf, len);
    write(data);
  }

  void write(const CArray *data) {
    sendqueue.put(data);
    if (!ready) {
        ready = true;
        io.start();
    }
  }

protected:
  /** client connection */
  int fd = -1;

  /** sending */
  const CArray *sendbuf = nullptr;
  unsigned sendpos;
  Queue <const CArray *> sendqueue;
  bool ready = false;
};

class RecvBuf
{
  ev::io io;
  void io_cb (ev::io &w, int revents);

public:
  error_cb on_error_cb;
  recv_cb on_recv_cb;

  template<class K, void (K::*method)()>
  void set_error_cb (K *object)
  {
    on_error_cb.set<K,&K::method>(object);
  }

  template<class K, void (K::*method)()>
  void set_recv_cb (K *object)
  {
    on_recv_cb.set<K,&K::method>(object);
  }

  // dummy methods, to be overridden
  void on_error() {}
  size_t on_data(uint8_t *buf, size_t len) { return len; }

  RecvBuf() {} // dead
  RecvBuf(int fd) {
    init(fd);
  }
  void init(int fd) {
    assert (this->fd == -1);
    set_non_blocking(fd);
    this->fd = fd;
    io.set<RecvBuf, &RecvBuf::io_cb>(this);
    on_error_cb.set<RecvBuf,&RecvBuf::on_error>(this);
    on_recv_cb.set<RecvBuf,&RecvBuf::on_data>(this);
  };
  virtual ~RecvBuf() {};

  void start();
  void stop();
  bool running;

protected:
  /** client connection */
  int fd = -1;

  /** receiving */
  uint8_t recvbuf[1024];
  size_t recvpos = 0;
  int len = 0; // of current block
  void feed_out();

};

#endif
