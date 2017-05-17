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

#ifndef IPSUPPORT_H
#define IPSUPPORT_H

#include <netinet/in.h>
#include "common.h"
#include "lpdu.h"

class Trace;

/** resolve host name */
bool GetHostIP (TracePtr t, struct sockaddr_in *sock, const std::string& name);
/** gets source address for a route */
bool GetSourceAddress (TracePtr t, 
                      const struct sockaddr_in *dest,
		      struct sockaddr_in *src);
/** convert a to EIBnet/IP format */
CArray IPtoEIBNetIP (const struct sockaddr_in *a, bool nat);
/** convert EIBnet/IP IP Address to a */
bool EIBnettoIP (const CArray & buf, struct sockaddr_in *a,
		const struct sockaddr_in *src, bool & nat);
bool compareIPAddress (const struct sockaddr_in &a,
		       const struct sockaddr_in &b);

#endif
