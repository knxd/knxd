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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include "addrtab.h"
#include "lowlevelconf.h"

/** aborts program with a printf like message */
void
die (const char *msg, ...)
{
  va_list ap;
  va_start (ap, msg);
  vprintf (msg, ap);
  printf ("\n");
  va_end (ap);

  exit (1);
}

/** structure to store low level backends */
struct urldef
{
  /** URL-prefix */
  const char *prefix;
  /** factory function */
  LowLevel_Create_Func Create;
  /** cleanup function */
  void (*Cleanup) ();
};

/** list of URLs */
struct urldef URLs[] = {
#undef L2_NAME
#define L2_NAME(a) { a##_PREFIX, a##_CREATE, a##_CLEANUP },
#include "lowlevelcreate.h"
  {0, 0, 0}
};

void (*Cleanup) ();

/** determines the right backend for the url and creates it */
LowLevelDriverInterface *
Create (const char *url, Trace * t)
{
  unsigned int p = 0;
  struct urldef *u = URLs;
  while (url[p] && url[p] != ':')
    p++;
  if (url[p] != ':')
    die ("not a valid url");
  while (u->prefix)
    {
      if (strlen (u->prefix) == p && !memcmp (u->prefix, url, p))
	{
	  Cleanup = u->Cleanup;
	  return u->Create (url + p + 1, t);
	}
      u++;
    }
  die ("url not supported");
  return 0;
}

/** version */
const char *argp_program_version = "bcuread " VERSION;
/** documentation */
static char doc[] =
  "bcuread -- read BCU memory\n"
  "(C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>\n"
  "supported URLs are:\n"
#undef L2_NAME
#define L2_NAME(a) a##_URL
#include "lowlevelcreate.h"
  "\n"
#undef L2_NAME
#define L2_NAME(a) a##_DOC
#include "lowlevelcreate.h"
  "\n";

/** structure to store the arguments */
struct arguments
{
  /** trace level */
  int tracelevel;
};
/** storage for the arguments*/
struct arguments arg;

unsigned
readHex (const char *addr)
{
  int i;
  sscanf (addr, "%x", &i);
  return i;
}

/** documentation for arguments*/
static char args_doc[] = "URL addr len";

/** option list */
static struct argp_option options[] = {

  {"trace", 't', "LEVEL", 0, "set trace level"},
  {0}
};


/** parses and stores an option */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = (struct arguments *) state->input;
  switch (key)
    {
    case 't':
      arguments->tracelevel = (arg ? atoi (arg) : 0);
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/** information for the argument parser*/
static struct argp argp = { options, parse_opt, args_doc, doc };


int
main (int ac, char *ag[])
{
  int addr;
  int len;
  int index;
  CArray result;
  LowLevelDriverInterface *iface = 0;
  memset (&arg, 0, sizeof (arg));

  argp_parse (&argp, ac, ag, 0, &index, &arg);
  if (index > ac - 3)
    die ("more parameter expected");
  if (index < ac - 3)
    die ("unexpected parameter");

  signal (SIGPIPE, SIG_IGN);
  pth_init ();

  Trace t;
  t.SetTraceLevel (arg.tracelevel);

  iface = Create (ag[index], &t);
  if (!iface)
    die ("initialisation failed");
  if (!iface->init ())
    die ("initialisation failed");

  addr = readHex (ag[index + 1]);
  len = atoi (ag[index + 2]);

  int res = readEMIMem (iface, addr, len, result);
  if (!res)
    {
      printf ("Read failed");
    }
  else
    {
      for (int i = 0; i < result (); i++)
	printf ("%02x ", result[i]);
      printf ("\n");
    }

  delete iface;
  if (Cleanup)
    Cleanup ();

  pth_exit (0);
  return 0;
}
