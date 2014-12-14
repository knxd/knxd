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
#include "common.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

void
printHex (int len, uchar * data)
{
  int i;
  for (i = 0; i < len; i++)
    printf ("%02X ", data[i]);
}

void
die (const char *msg, ...)
{
  va_list ap;
  va_start (ap, msg);
  vprintf (msg, ap);
  va_end (ap);
  printf (": %s\n", strerror (errno));
  exit (1);
}

eibaddr_t
readaddr (const char *addr)
{
  int a, b, c;
  if (sscanf (addr, "%d.%d.%d", &a, &b, &c) == 3)
    return ((a & 0x0f) << 12) | ((b & 0x0f) << 8) | ((c & 0xff));
  if (sscanf (addr, "%x", &a) == 1)
    return a & 0xffff;
  die ("invalid individual address %s", addr);
}

void
printIndividual (eibaddr_t addr)
{
  printf ("%d.%d.%d", (addr >> 12) & 0x0f, (addr >> 8) & 0x0f, (addr) & 0xff);
}

void
printGroup (eibaddr_t addr)
{
  printf ("%d/%d/%d", (addr >> 11) & 0x1f, (addr >> 8) & 0x07, (addr) & 0xff);
}

eibaddr_t
readgaddr (const char *addr)
{
  int a, b, c;
  if (sscanf (addr, "%d/%d/%d", &a, &b, &c) == 3)
    return ((a & 0x01f) << 11) | ((b & 0x07) << 8) | ((c & 0xff));
  if (sscanf (addr, "%d/%d", &a, &b) == 2)
    return ((a & 0x01f) << 11) | ((b & 0x7FF));
  if (sscanf (addr, "%x", &a) == 1)
    return a & 0xffff;
  die ("invalid group address format %s", addr);
}

unsigned
readHex (const char *addr)
{
  int i;
  sscanf (addr, "%x", &i);
  return i;
}

int
readBlock (uchar * buf, int size, int ac, char *ag[])
{
  int i = 0;
  while (size - i > 0 && ac > 0)
    {
      buf[i] = readHex (ag[i]);
      i++;
      ac--;
    }
  return i;
}

static int havekey = 0;
static uint8_t eibkey[4];

void
parseKey (int *ac, char **ag[])
{
  uint32_t k;
  if (*ac < 3)
    return;
  if (!strcmp ((*ag)[1], "-k"))
    {
      sscanf ((*ag)[2], "%x", &k);
      havekey = 1;
      eibkey[0] = (k >> 24) & 0xff;
      eibkey[1] = (k >> 16) & 0xff;
      eibkey[2] = (k >> 8) & 0xff;
      eibkey[3] = (k >> 0) & 0xff;
      *ac -= 2;
      *ag += 2;
    }
}

void
auth (EIBConnection * con)
{
  int res;
  if (!havekey)
    return;
  res = EIB_MC_Authorize (con, eibkey);
  if (res == -1)
    die ("authorize failed");
  printf ("Level %d\n", res);
}
