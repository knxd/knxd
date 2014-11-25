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

#ifdef HAVE_FT12
L2_NAME (FT12)
#endif
#ifdef HAVE_PEI16
  L2_NAME (PEI16)
#endif
#ifdef HAVE_TPUART
  L2_NAME (TPUART24)
  L2_NAME (TPUART26)
#endif
#ifdef HAVE_EIBNETIP
  L2_NAME (EIBNETIP)
#endif
#ifdef HAVE_EIBNETIPTUNNEL
  L2_NAME (EIBNETIPTUNNEL)
  L2_NAME (EIBNETIPTUNNELNAT)
#endif
#ifdef HAVE_PEI16s
  L2_NAME (PEI16s)
#endif
#ifdef HAVE_TPUARTs
  L2_NAME (TPUARTs)
#endif
#ifdef HAVE_USB
  L2_NAME (USB)
#endif
