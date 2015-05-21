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
#include "layer2.h"
#include "localserver.h"
#include "inetserver.h"
#include "eibnetserver.h"
#include "groupcacheclient.h"

#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#include "systemdserver.h"
#endif

#define OPT_BACK_TUNNEL_NOQUEUE 1
#define OPT_BACK_TPUARTS_ACKGROUP 2
#define OPT_BACK_TPUARTS_ACKINDIVIDUAL 3
#define OPT_BACK_TPUARTS_DISCH_RESET 4
#define OPT_BACK_EMI_NOQUEUE 5

/** structure to store the arguments */
class arguments
{
public:
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

  /** do I have enough to do? */
  unsigned int has_work;

  /* EIBnet/IP multicast server flags */
  bool tunnel;
  bool route;
  bool discover;

  int backendflags;
  const char *serverip;
  const char *eibnetname;

private:
  /** our L3 instance (singleton (so far!)) */
  Layer3 *layer3;
  /** The current tracer */
  Trace *t;
  bool trace_used;

public:
  arguments () {
    addr = 0x0001;
  }
  ~arguments () {
    delete layer3;
  }

  /** get the L3 instance */
  Layer3 *l3()
    {
      if (layer3 == 0) 
        {
          layer3 = new Layer3 (addr, tracer());
          addr = 0;
        }
      return layer3;
    }
  bool has_l3() 
    {
      return layer3 != NULL;
    }
    
  /** get the current tracer.
   * Call with 'true' when you modify the tracer
   * and with 'false' when you want to use it.
   */
  Trace *tracer(bool fresh = false)
    {
      if (fresh && trace_used)
        {
          Trace *tr = new Trace(*t);
          l3()->registerTracer(t);
          t = tr;
          trace_used = false;
        }
      else if (! t)
        {
          t= new Trace();
          trace_used = !fresh;

          t->SetErrorLevel (LEVEL_WARNING); // default
        }
      return t;
    }
  void finish_l3 ()
    {
      if (trace_used)
        l3()->registerTracer(t);
      else if (t)
        delete t;
      t = NULL;
    }
};

/** storage for the arguments*/
arguments arg;

/** aborts program with a printf like message */
void
die (const char *msg, ...)
{
  va_list ap;
  int err = errno;

  va_start (ap, msg);
  vprintf (msg, ap);
  printf (": %s\n", strerror(err));
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
  void (*Cleanup) ();
};

/** list of URLs */
struct urldef URLs[] = {
#undef L2_NAME
#define L2_NAME(a) { a##_PREFIX, a##_CREATE, a##_CLEANUP },
#include "layer2create.h"
  {0, 0, 0}
};

/** determines the right backend for the url and creates it */
Layer2Interface *
Create (const char *url, int flags, Layer3 * l3)
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
        return u->Create (url + p + 1, flags, l3);
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
  "(C) 2005-2015 Martin Koegler <mkoegler@auto.tuwien.ac.at> et al.\n"
  "Supported Layer-2 URLs are:\n"
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
   "start the programm as daemon. Output will be written to FILE if given"},
#ifdef HAVE_EIBNETIPSERVER
  {"Tunnelling", 'T', 0, 0,
   "enable EIBnet/IP Tunneling in the EIBnet/IP server"},
  {"Routing", 'R', 0, 0, "enable EIBnet/IP Routing in the EIBnet/IP server"},
  {"Discovery", 'D', 0, 0,
   "enable the EIBnet/IP server to answer discovery and description requests (SEARCH, DESCRIPTION)"},
  {"Server", 'S', "ip[:port]", OPTION_ARG_OPTIONAL,
   "starts the EIBnet/IP server part"},
  {"Name", 'n', "SERVERNAME", 0, "The name of the EIBnet/IP server as shown in ETS (default is knxd)"},
  {"layer2", 'b', "driver:[arg]", 0,
   "the Layer-2 driver to use."},
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
      {
        const char *serverip = "";
        if (arg)
          serverip = arg;
        if (!*serverip) 
          serverip = "224.0.23.12";

        const char *name = arguments->eibnetname;

        BaseServer *c;
        int port = 0;
        char *a = strdup (serverip);
        char *b;
        if (!a)
          die ("out of memory");
        b = strchr (a, ':');
        if (b)
          {
            *b = 0;
            port = atoi (b + 1);
          }
        if (port <= 0)
          port = 3671;

        c = new EIBnetServer (a, port, arguments->tunnel, arguments->route, arguments->discover,
                              arguments->l3(), arguments->tracer(),
                              (name && *name) ? name : "knxd");
        if (!c->init ())
          die ("initialization of the EIBnet/IP server failed");
        free (a);
        arguments->tunnel = false;
        arguments->route = false;
        arguments->discover = false;
        arguments->eibnetname = 0;
        arguments->has_work |= 0x02;
      }
      break;
    case 'u':
      {
        BaseServer *s;
        const char *name = "";
        if (arg)
          name = arg;
        if (!*name)
          name = "/tmp/eib";
        s = new LocalServer (arguments->l3(), arguments->tracer(), name);
        if (!s->init ())
          die ("initialisation of the knxd unix protocol failed");
        arguments->has_work |= 0x02;
      }
      break;
    case 'i':
      {
        BaseServer *s = NULL;
        int port = arg ? atoi (arg) : 0;
        if (port == 0)
          port = 6720;
        if (port > 0)
          s = new InetServer (arguments->l3(), arguments->tracer(), port);
        if (!s || !s->init ())
          die ("initialisation of the knxd inet protocol failed");
        arguments->has_work |= 0x02;
      }
      break;
    case 't':
      arguments->tracer(true)->SetTraceLevel (arg ? atoi (arg) : 0);
      break;
    case 'f':
      arguments->tracer(true)->SetErrorLevel (arg ? atoi (arg) : 0);
      break;
    case 'e':
      if (arguments->has_l3 ())
      arguments->addr = readaddr (arg);
      break;
    case 'p':
      arguments->pidfile = arg;
      break;
    case 'd':
      arguments->daemon = (char *) (arg ? arg : "/dev/null");
      break;
    case 'c':
      if (!CreateGroupCache (arguments->l3(), arguments->tracer(), true))
        die ("initialisation of the group cache failed");
      break;
    case 'n':
      arguments->eibnetname = (char *)arg;
      if(arguments->eibnetname[0] == '=')
          arguments->eibnetname++;
      if(strlen(arguments->eibnetname) >= 30)
          die("EIBnetServer/IP name must be shorter than 30 bytes");
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
    case ARGP_KEY_ARG:
    case 'b':
      {
        Layer2Interface *l2 = Create (arg, arguments->backendflags, arguments->l3 ());
        if (!l2 || !l2->init ())
          die ("initialisation of backend '%s' failed", arg);
        arguments->backendflags = 0;
        arguments->has_work |= (arguments->has_work & 0x01) ? 0x02 : 0x01;
        /* The idea is that having two or more L2 interfaces to route between,
         * but no network-or-whatever front-end, is perfectly reasonable. 
         */
        break;
      }
    case ARGP_KEY_FINI:
      if (arguments->backendflags)
        die ("You need to use backend flags in front of the affected backend");

#ifdef HAVE_SYSTEMD
      {
        const int num_fds = sd_listen_fds(0);

        if( num_fds < 0 )
          die("Error getting fds from systemd.");
        // zero FDs from systemd is not a bug

        for( int fd = SD_LISTEN_FDS_START; fd < SD_LISTEN_FDS_START+num_fds; ++fd )
          {
            if( sd_is_socket(fd, AF_UNSPEC, SOCK_STREAM, 1) <= 0 )
              die("Error: socket not of expected type.");

            s = new SystemdServer(arguments->l3(), arguments->tracer(), fd);
            if (!s->init ())
              die ("initialisation of the systemd socket failed");
            arguments->has_work |= 0x02;
          }
      }
#endif

      if (arguments->tunnel || arguments->route || arguments->discover || 
          arguments->eibnetname)
        die ("Option '-S' starts the multicast server.\n"
             "-T/-R/-D/-n after that are useless.");
      if (!(arguments->has_work & 0x01))
        die ("I know of no Layer-2 interface. Giving up.");
      if (!(arguments->has_work & 0x02))
        die ("I have one Layer-2 interface but no network. Giving up.");
      arguments->finish_l3();
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/** information for the argument parser*/
static struct argp argp = { options, parse_opt, args_doc, doc };

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

int
main (int ac, char *ag[])
{
  int index;

  argp_parse (&argp, ac, ag, ARGP_IN_ORDER, &index, &arg);

  signal (SIGPIPE, SIG_IGN);
  pth_init ();

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


  signal (SIGINT, SIG_IGN);
  signal (SIGTERM, SIG_IGN);

  // main loop
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
	      ERRORPRINTF (arg.tracer(), 0x27000002, 0, "can't open log file %s",
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

#ifdef HAVE_GROUPCACHE
  DeleteGroupCache ();
#endif

  if (arg.pidfile)
    unlink (arg.pidfile);

  pth_exit (0);
  return 0;
}
