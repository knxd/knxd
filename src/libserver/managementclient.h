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

#ifndef MANAGEMENT_CLIENT_H
#define MANAGEMENT_CLIENT_H

#include "client.h"

/** reads all individual address of devices in the programming mode 
 * @param c client connection
 */
void ReadIndividualAddresses (ClientConnPtr c, uint8_t *buf, size_t len);
/** change programming mode of a device
 * @param c client connection
 */
void ChangeProgMode (ClientConnPtr c, uint8_t *buf, size_t len);

/** read the mask version of a device
 * @param c client connection
 */
void GetMaskVersion (ClientConnPtr c, uint8_t *buf, size_t len);

/** write a individual address 
 * @param c client connection
 */
void WriteIndividualAddress (ClientConnPtr c, uint8_t *buf, size_t len);

/** opens and handles a management connection
 * @param c client connection
 */
void ManagementConnection (ClientConnPtr c);
class ManagementConnection : public A_Base {
  ManagementConnection (ClientConnPtr c, uint8_t *buf, size_t len);
  void recv(uint8_t *buf, size_t len);
};

/** Loads an image in a BCU
 * @param c client connection
 */
void LoadImage (ClientConnPtr c, uint8_t *buf, size_t len);

/** opens and handles a individual connection
 * @param c client connection
 */
class ManagementIndividual : public A_Base {
  ManagementIndividual (ClientConnPtr c, uint8_t *buf, size_t len);
  void recv(uint8_t *buf, size_t len);
};

#endif
