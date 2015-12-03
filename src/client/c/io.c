/*
    EIBD client library - internals
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    In addition to the permissions in the GNU General Public License, 
    you may link the compiled version of this file into combinations
    with other programs, and distribute those combinations without any 
    restriction coming from the use of this file. (The General Public 
    License restrictions do apply in other respects; for example, they 
    cover modification of the file, and distribution when not linked into 
    a combine executable.)

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "config.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "eibclient-int.h"

/** send a request to eibd */
int
_EIB_SendRequest (EIBConnection * con, unsigned int size, uchar * data)
{
  uchar head[2];
  int i, start;

  if (size > 0xffff || size < 2)
    {
      errno = EINVAL;
      return -1;
    }
  head[0] = (size >> 8) & 0xff;
  head[1] = (size) & 0xff;

lp1:
  i = write (con->fd, &head, 2);
  if (i == -1 && errno == EINTR)
    goto lp1;
  if (i == -1)
    return -1;
  if (i != 2)
    {
      errno = ECONNRESET;
      return -1;
    }
  start = 0;
lp2:
  i = write (con->fd, data + start, size - start);
  if (i == -1 && errno == EINTR)
    goto lp2;
  if (i == -1)
    return -1;
  if (i == 0)
    {
      errno = ECONNRESET;
      return -1;
    }
  start += i;
  if (start < size)
    goto lp2;
  return 0;
}

int
_EIB_CheckRequest (EIBConnection * con, int block)
{
  int i;
  struct timeval tv;
  fd_set readset;

  if (!block)
    {
      tv.tv_sec = 0;
      tv.tv_usec = 0;
      FD_ZERO (&readset);
      FD_SET (con->fd, &readset);
      if (select (con->fd + 1, &readset, 0, 0, &tv) == -1)
	return -1;

      if (!FD_ISSET (con->fd, &readset))
	return 0;
    }

  if (con->readlen < 2)
    {
      uchar head[2];
      head[0] = (con->size >> 8) & 0xff;
      i = read (con->fd, &head + con->readlen, 2 - con->readlen);
      if (i == -1 && errno == EINTR)
	return 0;
      if (i == -1)
	return -1;
      if (i == 0)
	{
	  errno = ECONNRESET;
	  return -1;
	}
      con->readlen += i;
      con->size = (head[0] << 8) | (head[1]);
      if (con->size < 2)
	{
	  errno = ECONNRESET;
	  return -1;
	}

      if (con->size > con->buflen)
	{
	  con->buf = (uchar *) realloc (con->buf, con->size);
	  if (con->buf == 0)
	    {
	      con->buflen = 0;
	      errno = ENOMEM;
	      return -1;
	    }
	  con->buflen = con->size;
	}
      return 0;
    }

  if (con->readlen < con->size + 2)
    {
      i =
	read (con->fd, con->buf + (con->readlen - 2),
	      con->size - (con->readlen - 2));
      if (i == -1 && errno == EINTR)
	return 0;
      if (i == -1)
	return -1;
      if (i == 0)
	{
	  errno = ECONNRESET;
	  return -1;
	}
      con->readlen += i;
    }
  return 0;
}

/** receive packet from eibd */
int
_EIB_GetRequest (EIBConnection * con)
{
  do
    {
      if (_EIB_CheckRequest (con, 1) == -1)
	return -1;
    }
  while (con->readlen < 2
	 || (con->readlen >= 2 && con->readlen < con->size + 2));

  con->readlen = 0;

  return 0;
}
