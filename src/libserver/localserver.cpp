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

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include "localserver.h"

LocalServer::LocalServer (BaseRouter& r, IniSectionPtr& s)
    : NetServer (r,s)
{
  t->setAuxName("local");
}

bool
LocalServer::setup()
{
  path = cfg->value("path","/run/knx");
  ignore_when_systemd = cfg->value("systemd-ignore",(path == "/run/knx"));
  if (!NetServer::setup())
    return false;
  return true;
}

void
LocalServer::start()
{
  if (ignore_when_systemd && static_cast<Router &>(router).using_systemd)
    {
      may_fail = true;
      stopped();
      return;
    }

  struct sockaddr_un addr;
  TRACEPRINTF (t, 8, "OpenLocalSocket %s", path);
  addr.sun_family = AF_LOCAL;
  strncpy (addr.sun_path, path.c_str(), sizeof (addr.sun_path));

  fd = socket (AF_LOCAL, SOCK_STREAM, 0);
  if (fd == -1)
    {
      ERRORPRINTF (t, E_ERROR | 15, "OpenLocalSocket %s: socket: %s", path, strerror(errno));
      goto ex3;
    }

  if (bind (fd, (struct sockaddr *) &addr, sizeof (addr)) == -1)
    {
      /* 
       * dead file? 
       */
      if (errno == EADDRINUSE)
        {
          if (connect(fd, (struct sockaddr *) &addr, sizeof (addr)) == 0)
            {
          ex:
              ERRORPRINTF (t, E_ERROR | 16, "OpenLocalSocket %s: bind: %s", path, strerror(errno));
              goto ex2;
            }
          else if (errno == ECONNREFUSED)
            {
              ::unlink (path.c_str());
              if (bind (fd, (struct sockaddr *) &addr, sizeof (addr)) == -1)
                goto ex;
            }
          else
            {
              ERRORPRINTF (t, E_ERROR | 18, "Existing socket %s: connect: %s", path, strerror(errno));
              goto ex2;
            }
        }
    }

  if (listen (fd, 10) == -1)
    {
      ERRORPRINTF (t, E_ERROR | 17, "OpenLocalSocket %s: listen: %s", path, strerror(errno));
      goto ex2;
    }

  this->path = path;
  TRACEPRINTF (t, 8, "LocalSocket opened");
  NetServer::start();
  return;

ex2:
  close (fd);
  fd = -1;
ex3:
  NetServer::stop();
  return;
}

void
LocalServer::stop()
{
  if (fd >= 0)
    {
      close(fd);
      fd = -1;
      if (path.size())
        ::unlink (path.c_str());
    }
  NetServer::stop();
}

LocalServer::~LocalServer ()
{
  if (fd >= 0)
    {
      close(fd);
      if (path.size())
        ::unlink (path.c_str());
    }
}

