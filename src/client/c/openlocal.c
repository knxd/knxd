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

#include <sys/socket.h>
#include <sys/un.h>

#include "eibclient-int.h"

EIBConnection *
EIBSocketLocal (const char *path)
{
  EIBConnection *con = (EIBConnection *) malloc (sizeof (EIBConnection));
  struct sockaddr_un addr;
  if (!con)
    {
      errno = ENOMEM;
      return 0;
    }
  addr.sun_family = AF_LOCAL;
  strncpy (addr.sun_path, path, sizeof (addr.sun_path));
  addr.sun_path[sizeof (addr.sun_path) - 1] = 0;

  con->fd = socket (AF_LOCAL, SOCK_STREAM, 0);
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
  con->complete = 0;
  con->buflen = 0;
  con->buf = 0;
  con->readlen = 0;

  return con;
}
