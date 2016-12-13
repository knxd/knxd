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

#ifndef KNXD_COMMON_MYSTRING_H
#define KNXD_COMMON_MYSTRING_H

/* This has to stay because other source files rely on it */
#include <string.h>

#include <string>

class String: public std::string {
public:
	/**
	 * Construct an empty string.
	 */
	String() {}

	/**
	 * Copy constructor
	 */
	String(const String& lhs): std::string(lhs) {}

	/**
	 * Copy constructor
	 */
	String(const std::string& lhs): std::string(lhs) {}

	/**
	 * Copy constructor
	 */
	String(const char* lhs): std::string(lhs) {}

	/**
	 * Transform into a C-String
	 */
	const char* operator ()() const {
		return c_str();
	}

	/**
	 * Concatenation
	 */
	String operator +(const String& rhs) const {
		return String(static_cast<const std::string&>(*this)
		              + static_cast<const std::string&>(rhs));
	}
};

#endif
