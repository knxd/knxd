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
 * This 
 */

#include <iostream>
#include <fstream>

#include <assert.h>
#include <string.h>

#include "inifile.h"
#include "types.h"

IniSection::IniSection(IniData &p, const std::string& n) : values(), name(n), parent(p) { }
IniSection::IniSection(IniData &p, const std::string&& n) : values(), name(n), parent(p) { }

const std::string&
IniSection::value(const std::string& name, const std::string& def)
{
  auto v = values.find(name);
  if (v == values.end())
    {
      v = values.find("use");
      if (v == values.end())
        return def;
      return parent[v->second].value(name,def);
    }

  return v->second;
}
const std::string
IniSection::value(const std::string& name, const char *def)
{
  std::string s = def;
  return value(name,s);
}

static const std::string empty = "";

std::string&
IniSection::operator[](const char *name)
{
  if (parent.read_only)
    {
      auto v = values.find(name);
      assert (v != values.end());
      return v->second;
    } else {
      auto res = values.emplace(name, "");
      return res.first->second;
    }
}

int
IniSection::value(const std::string& name, int def)
{
  static const std::string empty = "";
  const std::string v { value(name, empty) };
  if (!v.size())
    return def;
  size_t pos;
  int res = std::stoi(v, &pos, 0);
  if (pos == v.size())
    return res;
  std::cerr << "Parse error: Not an integer: " << name << "=" << v << std::endl;
  return def;
}

bool
IniSection::value(const std::string& name, bool def)
{
  static const std::string empty = "";
  const std::string& v { value(name, empty) };
  if (!v.size())
    return def;
  if (!v.compare("Y"))
    return true;
  if (!v.compare("N"))
    return false;
  if (!v.compare("y"))
    return true;
  if (!v.compare("n"))
    return false;
  if (!v.compare("1"))
    return true;
  if (!v.compare("0"))
    return false;
  if (!v.compare("true"))
    return true;
  if (!v.compare("false"))
    return false;
  if (!v.compare("True"))
    return true;
  if (!v.compare("False"))
    return false;
  if (!v.compare("TRUE"))
    return true;
  if (!v.compare("FALSE"))
    return false;

  std::cerr << "Parse error: Not a bool: " << name << "=" << v << std::endl;
  return def;
};

IniData::IniData() : sections() { }

static int
inidata_handler(void* user, const char* section, const char* name, const char* value)
{
  IniData *data = (IniData *)user;
  return data->add(section, name, value);
}

bool
IniData::add(const char *section, const char *name, const char *value)
{
  auto res = sections.emplace(std::piecewise_construct,
                std::forward_as_tuple(section),
                std::forward_as_tuple(*this, section));
  if (! res.second && name == NULL)
    {
      std::cerr << "Parse error: Duplicate section: " << section << std::endl;
      return false;
    }

  IniSection *s = &(res.first->second);
  if (name == NULL)
    return true;
  return s->add(name,value);
}

IniSection&
IniData::operator[](const char *name)
{
  // for now we always create a section.
  if (false) // read_only
    {
      auto v = sections.find(name);
      assert (v != sections.end());
      return v->second;
    }
  else
    {
      auto res = sections.emplace(std::piecewise_construct,
                    std::forward_as_tuple(name),
                    std::forward_as_tuple(*this, name));
      return res.first->second;
    }
}

IniSection&
IniSection::sub(const char *name)
{
  return parent.sub(name, *this);
}

IniSection&
IniData::sub(const char *name, IniSection& def)
{
  // for now we always create a section.
  auto v = sections.find(name);
  if (v == sections.end())
    return def;
  return v->second;
}

bool
IniSection::add(const char *name, const char *value)
{
  if (value == NULL)
    value = "TRUE"; // assume bool flag

  auto res2 = values.emplace(name, value);
  if (! res2.second)
    {
      std::cerr << "Parse error: Duplicate value: " << name << std::endl;
      return false;
    }
  return true;
}

static char*
inidata_reader(char* str, int num, void* stream)
{
  std::istream *f = (std::istream *)stream;

  if (f->rdstate() & std::ifstream::eofbit)
    return NULL;
  f->getline(str, num);
  if (f->rdstate() & std::ifstream::failbit)
    return NULL;
  return str;
}

int
IniData::parse(const std::string& filename)
{
  std::filebuf fb;
  if (filename == "-") {
    return parse(std::cin);
  } else if (fb.open (filename,std::ios::in)) {
    std::istream s(&fb);
    return parse(s);
  } else {
    std::cerr << "Unable to open: " << filename << ": " << strerror(errno) << std::endl;
    return 0;
  }
}

int
IniData::parse(std::istream& file)
{
  int res = ini_parse_stream(&inidata_reader, (void *)&file,
                          &inidata_handler, (void *)this);
  read_only = true;
  return res;
}

void
IniSection::write(std::ostream& file)
{
  ITER(i,values)
    file << i->first << " = " << i->second << std::endl;
}

void
IniData::write(std::ostream& file)
{
  ITER(i,sections)
    {
      file << '[' << i->first << ']' << std::endl;
      i->second.write(file);
    }
}


