/*
    knxd EIB/KNX bus access and management daemon
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>
    Copyright (C) 2014 Michael Markstaller <michael@markstaller.de>

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

#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "layer3.h"
#include "localserver.h"
#include "inetserver.h"
#include "eibnetserver.h"
#include "groupcacheclient.h"

#define OPT_BACK_TUNNEL_NOQUEUE 1
#define OPT_BACK_TPUARTS_ACKGROUP 2
#define OPT_BACK_TPUARTS_ACKINDIVIDUAL 3
#define OPT_BACK_TPUARTS_DISCH_RESET 4
#define OPT_BACK_EMI_NOQUEUE 5

/** structure to store the arguments */
struct arguments
{
  /** port to listen */
  int port;
  /** path for unix domain socket */
  const char *name;
  /** path to pid file */
  const char *pidfile;
  /** path to trace log file */
  const char *daemon;
  /** trace level */
  int tracelevel;
  /** error level */
  int errorlevel;
  /** EIB address (for some backends) */
  eibaddr_t addr;
  /* EIBnet/IP server */
  bool tunnel;
  bool route;
  bool discover;
  bool groupcache;
  int backendflags;
  const char *serverip;
};
/** storage for the arguments*/
struct arguments arg;

/** aborts program with a printf like message */
void
die (const char *msg, ...)
{
  va_list ap;
  va_start (ap, msg);
  vprintf (msg, ap);
  printf ("\n");
  va_end (ap);

  if (arg.pidfile)
    unlink (arg.pidfile);

  exit (1);
}


#include "layer2conf.h"

/** structure to store layer 2 backends */
struct urldef
{
  /** URL-prefix */
  const char *prefix;
  /** factory function */
  Layer2_Create_Func Create;
  /** cleanup function */
  void (*Cleanup) ();
};

/** list of URLs */
struct urldef URLs[] = {
#undef L2_NAME
#define L2_NAME(a) { a##_PREFIX, a##_CREATE, a##_CLEANUP },
#include "layer2create.h"
  {0, 0, 0}
};

void (*Cleanup) ();

/** determines the right backend for the url and creates it */
Layer2Interface *
Create (const char *url, int flags, Trace * t)
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
	  return u->Create (url + p + 1, flags, t);
	}
      u++;
    }
  die ("url not supported");
  return 0;
}

/** parses an EIB individual address */
eibaddr_t
readaddr (const char *addr)
{
  int a, b, c;
  sscanf (addr, "%d.%d.%d", &a, &b, &c);
  return ((a & 0x0f) << 12) | ((b & 0x0f) << 8) | ((c & 0xff));
}

/** version */
const char *argp_program_version = "knxd " VERSION;
/** documentation */
static char doc[] =
  "knxd -- a commonication stack for EIB/KNX\n"
  "(C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>\n"
  "supported URLs are:\n"
#undef L2_NAME
#define L2_NAME(a) a##_URL
#include "layer2create.h"
  "\n"
#undef L2_NAME
#define L2_NAME(a) a##_DOC
#include "layer2create.h"
  "\n";

/** documentation for arguments*/
static char args_doc[] = "URL";

/** option list */
static struct argp_option options[] = {
  {"listen-tcp", 'i', "PORT", OPTION_ARG_OPTIONAL,
   "listen at TCP port PORT (default 6720)"},
  {"listen-local", 'u', "FILE", OPTION_ARG_OPTIONAL,
   "listen at Unix domain socket FILE (default /tmp/eib)"},
  {"trace", 't', "LEVEL", 0, "set trace level"},
  {"error", 'f', "LEVEL", 0, "set error level"},
  {"eibaddr", 'e', "EIBADDR", 0,
   "set our own EIB-address to EIBADDR (default 0.0.1), for drivers, which need an address"},
  {"pid-file", 'p', "FILE", 0, "write the PID of the process to FILE"},
  {"daemon", 'd', "FILE", OPTION_ARG_OPTIONAL,
   "start the programm as daemon, the output will be written to FILE, if the argument present"},
#ifdef HAVE_EIBNETIPSERVER
  {"Tunnelling", 'T', 0, 0,
   "enable EIBnet/IP Tunneling in the EIBnet/IP server"},
  {"Routing", 'R', 0, 0, "enable EIBnet/IP Routing in the EIBnet/IP server"},
  {"Discovery", 'D', 0, 0,
   "enable the EIBnet/IP server to answer discovery and description requests (SEARCH, DESCRIPTION)"},
  {"Server", 'S', "ip[:port]", OPTION_ARG_OPTIONAL,
   "starts the EIBnet/IP server part"},
#endif
#ifdef HAVE_GROUPCACHE
  {"GroupCache", 'c', 0, 0,
   "enable caching of group communication network state"},
#endif
#ifdef HAVE_EIBNETIPTUNNEL
  {"no-tunnel-client-queuing", OPT_BACK_TUNNEL_NOQUEUE, 0, 0,
   "do not assume KNXnet/IP Tunneling bus interface can handle parallel cEMI requests"},
#endif
#ifdef HAVE_TPUARTs
  {"tpuarts-ack-all-group", OPT_BACK_TPUARTS_ACKGROUP, 0, 0,
   "tpuarts backend should generate L2 acks for all group telegrams"},
  {"tpuarts-ack-all-individual", OPT_BACK_TPUARTS_ACKINDIVIDUAL, 0, 0,
   "tpuarts backend should generate L2 acks for all individual telegrams"},
  {"tpuarts-disch-reset", OPT_BACK_TPUARTS_DISCH_RESET, 0, 0,
   "tpuarts backend should should use a full interface reset (for Disch TPUART interfaces)"},
#endif
  {"no-emi-send-queuing", OPT_BACK_EMI_NOQUEUE, 0, 0,
   "wait for L_Data_ind while sending (for all EMI based backends)"},
  {0}
};

/** parses and stores an option */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = (struct arguments *) state->input;
  switch (key)
    {
    case 'T':
      arguments->tunnel = 1;
      break;
    case 'R':
      arguments->route = 1;
      break;
    case 'D':
      arguments->discover = 1;
      break;
    case 'S':
      arguments->serverip = (arg ? arg : "224.0.23.12");
      break;
    case 'u':
      arguments->name = (char *) (arg ? arg : "/tmp/eib");
      break;
    case 'i':
      arguments->port = (arg ? atoi (arg) : 6720);
      break;
    case 't':
      arguments->tracelevel = (arg ? atoi (arg) : 0);
      break;
    case 'f':
      arguments->errorlevel = (arg ? atoi (arg) : 0);
      break;
    case 'e':
      arguments->addr = readaddr (arg);
      break;
    case 'p':
      arguments->pidfile = arg;
      break;
    case 'd':
      arguments->daemon = (char *) (arg ? arg : "/dev/null");
      break;
    case 'c':
      arguments->groupcache = 1;
      break;
    case OPT_BACK_TUNNEL_NOQUEUE:
      arguments->backendflags |= FLAG_B_TUNNEL_NOQUEUE;
      break;
    case OPT_BACK_TPUARTS_ACKGROUP:
      arguments->backendflags |= FLAG_B_TPUARTS_ACKGROUP;
      break;
    case OPT_BACK_TPUARTS_ACKINDIVIDUAL:
      arguments->backendflags |= FLAG_B_TPUARTS_ACKINDIVIDUAL;
      break;
    case OPT_BACK_TPUARTS_DISCH_RESET:
      arguments->backendflags |= FLAG_B_TPUARTS_DISCH_RESET;
      break;
    case OPT_BACK_EMI_NOQUEUE:
      arguments->backendflags |= FLAG_B_EMI_NOQUEUE;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/** information for the argument parser*/
static struct argp argp = { options, parse_opt, args_doc, doc };

#ifdef HAVE_EIBNETIPSERVER
EIBnetServer *
startServer (Layer3 * l3, Trace * t)
{
  EIBnetServer *c;
  char *ip;
  int port;
  if (!arg.serverip)
    return 0;

  char *a = strdup (arg.serverip);
  char *b;
  if (!a)
    die ("out of memory");
  for (b = a; *b; b++)
    if (*b == ':')
      break;
  if (*b == ':')
    {
      *b = 0;
      port = atoi (b + 1);
    }
  else
    port = 3671;
  c = new EIBnetServer (a, port, arg.tunnel, arg.route, arg.discover, l3, t);
  if (!c->init ())
    die ("initilization of the EIBnet/IP server failed");
  free (a);
  return c;
}
#endif

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

int
main (int ac, char *ag[])
{
  int index;
  Queue < Server * >server;
  Server *s;
  Layer2Interface *l2;
  Layer3 *l3;
#ifdef HAVE_EIBNETIPSERVER
  EIBnetServer *serv = 0;
#endif

  memset (&arg, 0, sizeof (arg));
  arg.addr = 0x0001;
  arg.errorlevel = LEVEL_WARNING;

  argp_parse (&argp, ac, ag, 0, &index, &arg);
  if (index > ac - 1)
    die ("url expected");
  if (index < ac - 1)
    die ("unexpected parameter");

  if (arg.port == 0 && arg.name == 0 && arg.serverip == 0)
    die ("No listen-address given");

  signal (SIGPIPE, SIG_IGN);
  pth_init ();

  Trace t;
  t.SetTraceLevel (arg.tracelevel);
  t.SetErrorLevel (arg.errorlevel);

  /*
  if (getuid () == 0)
    ERRORPRINTF (&t, 0x37000001, 0, "EIBD should not run as root");
  */
  
  if (arg.daemon)
    {
      int fd = open (arg.daemon, O_WRONLY | O_APPEND | O_CREAT, FILE_MODE);
      if (fd == -1)
	die ("Can not open file %s", arg.daemon);
      int i = fork ();
      if (i < 0)
	die ("fork failed");
      if (i > 0)
	exit (0);
      close (1);
      close (2);
      close (0);
      dup2 (fd, 1);
      dup2 (fd, 2);
      close (fd);
      setsid ();
    }


  FILE *pidf;
  if (arg.pidfile)
    if ((pidf = fopen (arg.pidfile, "w")) != NULL)
      {
	fprintf (pidf, "%d", getpid ());
	fclose (pidf);
      }

  l2 = Create (ag[index], arg.backendflags, &t);
  if (!l2 || !l2->init ())
    die ("initialisation of the backend failed");
  l3 = new Layer3 (l2, &t);
  if (arg.port)
    {
      s = new InetServer (l3, &t, arg.port);
      if (!s->init ())
	die ("initialisation of the knxd inet protocol failed");
      server.put (s);
    }
  if (arg.name)
    {
      s = new LocalServer (l3, &t, arg.name);
      if (!s->init ())
	die ("initialisation of the knxd unix protocol failed");
      server.put (s);
    }
#ifdef HAVE_EIBNETIPSERVER
  serv = startServer (l3, &t);
#endif
#ifdef HAVE_GROUPCACHE
  if (!CreateGroupCache (l3, &t, arg.groupcache))
    die ("initialisation of the group cache failed");
#endif

  signal (SIGINT, SIG_IGN);
  signal (SIGTERM, SIG_IGN);

  int sig;
  do
    {
      sigset_t t1;
      sigemptyset (&t1);
      sigaddset (&t1, SIGINT);
      sigaddset (&t1, SIGHUP);
      sigaddset (&t1, SIGTERM);

      pth_sigwait (&t1, &sig);

      if (sig == SIGHUP && arg.daemon)
	{
	  int fd =
	    open (arg.daemon, O_WRONLY | O_APPEND | O_CREAT, FILE_MODE);
	  if (fd == -1)
	    {
	      ERRORPRINTF (&t, 0x27000002, 0, "can't open log file %s",
			   arg.daemon);
	      continue;
	    }
	  close (1);
	  close (2);
	  dup2 (fd, 1);
	  dup2 (fd, 2);
	  close (fd);
	}

    }
  while (sig == SIGHUP);

  signal (SIGINT, SIG_DFL);
  signal (SIGTERM, SIG_DFL);

  while (!server.isempty ())
    delete server.get ();
#ifdef HAVE_EIBNETIPSERVER
  if (serv)
    delete serv;
#endif
#ifdef HAVE_GROUPCACHE
  DeleteGroupCache ();
#endif

  delete l3;
  if (Cleanup)
    Cleanup ();

  if (arg.pidfile)
    unlink (arg.pidfile);

  pth_exit (0);
  return 0;
}
