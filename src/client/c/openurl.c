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

#include "eibclient-int.h"

EIBConnection *
EIBSocketURL (const char *url)
{
  if (!url)
    {
      errno = EINVAL;
      return 0;
    }
  if (!strncmp (url, "local:", 6))
    {
      url += 6;
      return EIBSocketLocal (*url ? url : "/run/knx");
    }
  if (!strncmp (url, "ip:", 3))
    {
      char *a, *b;
      int port;
      EIBConnection *c;

      url += 3;
      a = strdup (*url ? url : "localhost");
      if (!a)
	{
	  errno = ENOMEM;
	  return 0;
	}
      b = strchr (a,':');
      if (b)
	{
	  *b = 0;
	  port = atoi (b + 1);
	}
      else
	port = 6720;
      c = EIBSocketRemote (a, port);
      free (a);
      return c;
    }
  fprintf(stderr, "Unknown URL prefix, need 'local:' or 'ip:'\n");
  errno = EINVAL;
  return 0;
}
