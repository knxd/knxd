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
#include <iostream>

main(int argc, const char *argv[])
{
  if(argc < 2) 
    {
      std::cerr << "Usage: " << argv[0] << "goodfile badfile…" << std::endl;
      exit(1);
    }

  IniData i;
  int errl = i.parse(argv[1]);
  if (errl)
    {
      std::cerr << "Parse error in '" << argv[1] << "', line" << errl << ". Stop." <<std::endl;
      exit(2);
    }

  IniSectionPtr a = i["main"];
  if ((*a)["foo"] != "bar")
    {
      std::cerr << "in section 'main': foo should be 'bar' but is '" << (*a)["foo"] << "'" << std::endl;
      exit(1);
    }
  if (a->value("baz",99) != 42)
    {
      std::cerr << "in section 'main': baz should be 42 but is " << a->value("baz",88) << std::endl;
      exit(1);
    }
  if (!a->value("duh",false))
    {
      std::cerr << "in section 'main': duh should be true" << std::endl;
      exit(1);
    };

  std::cerr << "'Good' test completed. Ignore the following errors." << std::endl;
  while(argc > 2)
    {
      IniData ix;
      if (!ix.parse(argv[--argc])) 
        {
          std::cerr << "file '" << argv[argc] << "should not parse cleanly" << std::endl;
          exit(1);
        }
    }
  std::cerr << "All tests completed correctly." << std::endl;
  exit(0);
}
