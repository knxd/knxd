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

#ifndef INIFILE_H
#define INIFILE_H

#include "inih.h"
#include <unordered_map>
#include <string>

typedef std::unordered_map<std::string,std::string> ValueMap;

class IniData;

class IniSection {
    const std::string name; // aliased from the mapping's key
    ValueMap values;
    IniData& parent;
  public:
    const std::string& value(const std::string& name, const std::string& def);
    std::string& operator[](const char *name);
    int value(const std::string& name, int def);
    bool value(const std::string& name, bool def);

    IniSection(IniData& parent, const std::string& n);
    bool add(const char *name, const char *value);

    void write(std::ostream& file);
};

typedef std::unordered_map<std::string,IniSection> SectionMap;
class IniData {
    SectionMap sections;

public:
    bool read_only = false;

    // method callback
    IniData();

    IniSection& operator[](const char *name);
    IniSection& operator[](const std::string& name);

    bool add(const char *section, const char *name, const char *value);

    int parse(const std::string& filename);
    int parse(std::istream& file);

    void write(std::ostream& file);
};

#endif
