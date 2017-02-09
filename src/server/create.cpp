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

#include "create.h"
#include "layer2conf.h"
#include "filterconf.h"

/** structure to store layer 2 backends */
struct layer2def
{
  /** URL-prefix */
  const char *prefix;
  /** factory function */
  Layer2_Create_Func Create;
};

/** structure to store layer 2 backends */
struct filterdef
{
  /** URL-prefix */
  const char *prefix;
  /** factory function */
  Layer2_Filter_Func Create;
};

/** list of layer2 drivers */
struct layer2def Layer2List[] = {
#undef L2_NAME
#define L2_NAME(a) { a##_PREFIX, a##_CREATE },
#include "layer2create.h"
  {0, 0}
};

/** list of filters */
struct filterdef FilterList[] = {
#undef L2_NAME
#define L2_NAME(a) { a##_PREFIX, a##_FILTER },
#include "filtercreate.h"
  {0, 0}
};

void die (const char *msg, ...);

Layer2Ptr
CreateLayer2 (const char *url, L2options *opt)
{
  unsigned int p = 0;
  struct layer2def *u = Layer2List;
  while (url[p] && url[p] != ':')
    p++;
  while (u->prefix)
    {
      if (strlen (u->prefix) == p && !memcmp (u->prefix, url, p))
        return u->Create (url + p + (url[p] == ':'), opt);
      u++;
    }
  die ("Layer2 '%s' not supported", url);
  return 0;
}

Layer2Ptr
AddLayer2Filter (const char *url, L2options *opt, Layer2Ptr l2)
{
  unsigned int p = 0;
  struct filterdef *u = FilterList;
  while (url[p] && url[p] != ':')
    p++;
  while (u->prefix)
    {
      if (strlen (u->prefix) == p && !memcmp (u->prefix, url, p))
        return u->Create (url + p + (url[p] == ':'), opt, l2);
      u++;
    }
  die ("Filter '%s' not supported", url);
  return 0;
}

