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

#ifndef C_FT12_H
#define C_FT12_H

#include "ft12.h"
#include "ft12cemi.h"
#include "emi2.h"
#include "layer3.h"

#define FT12_URL "ft12:/dev/ttySx\n"
#define FT12_DOC "ft12 connects over a serial line without any driver with the FT1.2 Protocol to a BCU 2\n\n"
#define FT12_PREFIX "ft12"
#define FT12_CREATE ft12_Create

inline Layer2Ptr 
ft12_Create (const char *dev, L2options *opt)
{
  return std::shared_ptr<EMI2Layer2>(new EMI2Layer2 (new FT12LowLevelDriver (dev, opt->t), opt));
}

#define FT12CEMI_URL "ft12cemi:/dev/ttySx\n"
#define FT12CEMI_DOC "ft12cemi connects over a serial line with the FT1.2+cEMI Protocol\n\n"
#define FT12CEMI_PREFIX "ft12cemi"
#define FT12CEMI_CREATE ft12cemi_Create

inline Layer2Ptr 
ft12cemi_Create (const char *dev, L2options *opt)
{
  return std::shared_ptr<FT12CEMILayer2>(new FT12CEMILayer2 (new FT12LowLevelDriver (dev, opt->t), opt));
}

#endif
