/*
    BCU SDK bcu development enviroment
    Copyright (C) 2016 Matthias Urlichs <matthias@urlichs.de>

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

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "iobuf.h"

void SendBuf::write(const CArray *data)
{
  if (!ready)
    {
      ssize_t len = ::write(fd, data->data(), data->size());
      if (len == (ssize_t)data->size())
        {
          delete data;
          return;
        }
      sendbuf = data;
      sendpos = (len>0) ? len : 0;
    }
  else
    sendqueue.push(data);
  if (!ready) {
      ready = true;
      io.start();
  }
}

void
SendBuf::io_cb (ev::io &w UNUSED, int revents UNUSED)
{
    while (sendbuf || !sendqueue.isempty()) {
        if (sendbuf) {
            int i = ::write(fd,sendbuf->data()+sendpos, sendbuf->size()-sendpos);
            if (i > 0) {
                sendpos += i;
                if (sendpos < sendbuf->size())
                    return;
            } else {
                if (i == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) {
                    io.stop();
                    on_error();
                }
                return;
            }
        }

        delete sendbuf;
        sendbuf = nullptr;

        if (!sendqueue.isempty()) {
            sendbuf = sendqueue.get();
            sendpos = 0;
        }
    }
    ready = false;
    io.stop();
    on_next();
}

void
RecvBuf::io_cb (ev::io &w UNUSED, int revents UNUSED)
{
    bool some = false;
    while(sizeof(recvbuf) > recvpos) {
	int i = ::read(fd, recvbuf+recvpos, quick ? 1 : (sizeof(recvbuf)-recvpos));
	if (i <= 0) {
            if (some)
                break;
	    if (i == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) {
		io.stop();
		on_error();
	    }
	    break;
	}
	recvpos += i;
        if (!quick)
            break;
        some = true;
    }
    feed_out();
}

void RecvBuf::feed_out()
{
    while (running && recvpos > 0) {
        size_t i = on_read(recvbuf,recvpos);
        if (i == 0) {
            if (recvpos == sizeof(recvbuf)) {
                io.stop();
                on_error();
            }
            return;
        }
        if (i == recvpos)
            recvpos = 0;
        else {
            recvpos -= i;
            memmove(recvbuf,recvbuf+i,recvpos);
        }
    }

}

void
set_non_blocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return;
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

void
RecvBuf::start()
{
    if (running)
      return;
    running = true;
    io.start(fd, ev::READ);
    feed_out();
}

void
SendBuf::start()
{
    io.start(fd, ev::WRITE);
}

void
RecvBuf::stop(bool clear)
{
    if (!running)
        running = false;
    io.stop();
    if (clear)
      fd = -1;
}

void
SendBuf::stop(bool clear)
{
    io.stop();
    if (clear)
      fd = -1;
}

