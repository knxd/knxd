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

#include "inifile.h"

#include <assert.h>
#include <stdio.h>

main(int argc, const char *argv[])
{
  if(argc != 2) 
    {
      fprintf(stderr, "Usage: %s testfile\n", argv[0]);
      exit(1);
    }

  IniData i;
  i.parse(argv[1]);

  IniSection &a = i["main"];
  assert (a["foo"] == "bar");
  assert (a.value("baz",99) == 42);
  assert (a.value("duh",false));
}
