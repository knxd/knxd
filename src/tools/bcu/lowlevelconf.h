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

#ifndef LAYER2_CONF_H
#define LAYER2_CONF_H

#include "config.h"

#ifdef HAVE_FT12
#include "l-FT12.h"
#endif
#ifdef HAVE_PEI16
#include "l-PEI16.h"
#endif
#ifdef HAVE_PEI16s
#include "l-PEI16s.h"
#endif
#ifdef HAVE_USB
#include "l-USB.h"
#endif

#endif
