/*
    EIB Demo program - common functions
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
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <unistd.h>
#include "config.h"
#include "eibclient.h"

/** unsigned char*/
typedef unsigned char uchar;

/** print hex dump of a buffer
 * \param len Length of the buffer
 * \param data buffer
 */
void printHex (int len, uchar * data);
/** aborts the program and prints message
 * \param msg Message (printf like)
 */
void die (const char *msg, ...);
/** parses a EIB address
 * \param addr string with the EIB address
 * \return EIB address
 */
eibaddr_t readaddr (const char *addr);
/** parses a EIB group address
 * \param addr string with the EIB address
 * \return EIB address
 */
eibaddr_t readgaddr (const char *addr);
/** parses a hex number
 * \param addr string
 * \return parsed hex number
 */
unsigned readHex (const char *addr);
/** parse hex numbers (byte size) out of a comand line
 * \param buf output buffer
 * \param size buffer size
 * \param ac argument count
 * \param ag argument array
 * \return parsed bytes
 */
int readBlock (uchar * buf, int size, int ac, char *ag[]);
/** prints a EIB individual address
 * \param addr EIB address
 */
void printIndividual (eibaddr_t addr);
void printGroup (eibaddr_t addr);

void parseKey (int *ac, char **ag[]);
void auth (EIBConnection *);
