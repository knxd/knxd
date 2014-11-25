/*
    EIBD client library
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include "eibclient-int.h"

#include "config.h"

/** resolve host name */
static int
GetHostIP (struct sockaddr_in *sock, const char *Name)
{
#ifdef HAVE_GETHOSTBYNAME_R
  int len = 2000;
  struct hostent host;
  char *buf = (char *) malloc (len);
  int res;
  int err;
#endif
  struct hostent *h;
  if (!Name)
    return 0;
  memset (sock, 0, sizeof (*sock));
#ifdef HAVE_SOCKADDR_IN_LEN
  sock->sin_len = sizeof (*sock);
#endif
#ifdef HAVE_GETHOSTBYNAME_R
  do
    {
      res = gethostbyname_r (Name, &host, buf, len, &h, &err);
      if (res == ERANGE)
	{
	  len += 2000;
	  buf = (char *) realloc (buf, len);
	}
      if (!buf)
	return 0;
    }
  while (res == ERANGE);

  if (res || !h)
    {
      free (buf);
      return 0;
    }
#else
  h = gethostbyname (Name);
  if (!h)
    return 0;
#endif
  sock->sin_family = h->h_addrtype;
  sock->sin_addr.s_addr = (*((unsigned long *) h->h_addr_list[0]));
#ifdef HAVE_GETHOSTBYNAME_R
  if (buf)
    free (buf);
#endif
  return 1;
}

EIBConnection *
EIBSocketRemote (const char *host, int port)
{
  EIBConnection *con = (EIBConnection *) malloc (sizeof (EIBConnection));
  struct sockaddr_in addr;
  int val = 1;
  if (!con)
    {
      errno = ENOMEM;
      return 0;
    }

  if (!GetHostIP (&addr, host))
    {
      free (con);
      errno = ECONNREFUSED;
      return 0;
    }
  addr.sin_port = htons (port);

  con->fd = socket (addr.sin_family, SOCK_STREAM, 0);
  if (con->fd == -1)
    {
      free (con);
      return 0;
    }

  if (connect (con->fd, (struct sockaddr *) &addr, sizeof (addr)) == -1)
    {
      int saveerr = errno;
      close (con->fd);
      free (con);
      errno = saveerr;
      return 0;
    }
  setsockopt (con->fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof (val));
  con->complete = 0;
  con->buflen = 0;
  con->buf = 0;
  con->readlen = 0;

  return con;
}
