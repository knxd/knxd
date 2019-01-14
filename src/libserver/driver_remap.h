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

// no protective #ifdef here!
//

#undef SERVER
#undef SERVER_
#undef FILTER
#undef FILTER_
#undef DRIVER
#undef DRIVER_

#ifdef NO_MAP

#define SERVER(_cls,_name) \
       DSERVER(_cls,_name)
#define SERVER_(_cls,_base,_name) \
       DSERVER_(_cls,_base,_name)

#define FILTER(_cls,_name) \
       DFILTER(_cls,_name)
#define FILTER_(_cls,_base,_name) \
       DFILTER_(_cls,_base,_name)

#define DRIVER(_cls,_name) \
       DDRIVER(_cls,_name)
#define DRIVER_(_cls,_base,_name) \
       DDRIVER_(_cls,_base,_name)

#else // NO_MAP

#define SERVER(_cls,_name) \
       RSERVER(_cls,_name)
#define SERVER_(_cls,_base,_name) \
       RSERVER_(_cls,_base,_name)

#define FILTER(_cls,_name) \
       RFILTER(_cls,_name)
#define FILTER_(_cls,_base,_name) \
       RFILTER_(_cls,_base,_name)

#define DRIVER(_cls,_name) \
       RDRIVER(_cls,_name)
#define DRIVER_(_cls,_base,_name) \
       RDRIVER_(_cls,_base,_name)

#endif // NO_MAP

