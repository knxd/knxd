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
 * This code implements "streaming" read and write buffers.
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
  InfoCallback on_error;
  InfoCallback on_next;
  void error_cb() {}
  void next_cb() {}

  SendBuf() {} // dead

  SendBuf(int fd) {
    init(fd);
  }
  void init(int fd) {
    assert (this->fd == -1);
    assert (fd >= 0);
    set_non_blocking(fd);
    this->fd = fd;
    io.set<SendBuf, &SendBuf::io_cb>(this);
    on_error.set<SendBuf,&SendBuf::error_cb>(this);
    on_next.set<SendBuf,&SendBuf::next_cb>(this);
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
  void stop(bool clear = false);

  void write(const uchar *buf, size_t len) {
    CArray *data = new CArray(buf, len);
    write(data);
  }

  void write(const CArray *data);

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
  bool quick = false;

public:
  InfoCallback on_error;
  DataCallback on_read;

  // dummy methods, to be overridden
  void error_cb() {}
  size_t recv_cb(uint8_t *buf UNUSED, size_t len) { return len; }

  RecvBuf() {} // dead
  RecvBuf(int fd) {
    init(fd);
  }
  void init(int fd) {
    assert (this->fd == -1);
    assert (fd >= 0);
    set_non_blocking(fd);
    this->fd = fd;
    io.set<RecvBuf, &RecvBuf::io_cb>(this);
    on_error.set<RecvBuf,&RecvBuf::error_cb>(this);
    on_read.set<RecvBuf,&RecvBuf::recv_cb>(this);
  };
  void low_latency() { quick = true; }
  virtual ~RecvBuf() {};

  void start();
  void stop(bool clear = false);
  bool running = false;

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
