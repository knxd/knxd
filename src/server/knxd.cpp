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
#include <ev++.h>
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

// The NOQUEUE options are deprecated
#define OPT_BACK_TUNNEL_NOQUEUE 1
#define OPT_BACK_TPUARTS_ACKGROUP 2
#define OPT_BACK_TPUARTS_ACKINDIVIDUAL 3
#define OPT_BACK_TPUARTS_DISCH_RESET 4
#define OPT_BACK_EMI_NOQUEUE 5
#define OPT_STOP_NOW 6
#define OPT_FORCE_BROADCAST 7
#define OPT_BACK_SEND_DELAY 8

#define OPT_ARG(_arg,_state,_default) (arg ? arg : \
        (state->argv[state->next] && state->argv[state->next][0] && (state->argv[state->next][0] != '-')) ?  \
            state->argv[state->next++] : _default)

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

  /** Start of address block to be assigned dynamically to clients */
  eibaddr_t alloc_addrs;
  /** Length of address block to be assigned dynamically to clients */
  int alloc_addrs_len;
  /* EIBnet/IP multicast server flags */
  bool tunnel;
  bool route;
  bool discover;

  L2options l2opts;
  const char *serverip;
  const char *eibnetname;

  bool stop_now;
  bool force_broadcast;
private:
  /** our L3 instance (singleton (so far!)) */
  Layer3 *layer3;

public:
  /** The current tracer */
  Trace t;

  arguments (): t("main") {
    addr = 0x0001;
  }
  ~arguments () {
  }

  /** get the L3 instance */
  Layer3 *l3()
    {
      if (layer3 == 0) 
        {
          TracePtr tr = tracer("layer3", false);
          layer3 = new Layer3 (addr, tr, force_broadcast);
          addr = 0;
          if (alloc_addrs_len)
            {
              layer3->set_client_block (alloc_addrs, alloc_addrs_len);
              alloc_addrs_len = 0;
            }
        }
      return layer3;
    }
  bool has_l3() 
    {
      return layer3 != NULL;
    }
  void free_l3() 
    {
      delete layer3;
    }
    
  /** get the current tracer.
   * Call with 'true' when you want to change the tracer
   * and with 'false' when you want to use it.
   *
   * If the current tracer has been used, it's not modified; instead, it is
   * passed to Layer3 (which will deallocate it when it ends) and copied to
   * a new instance.
   */

  TracePtr tracer(std::string name, bool reg = true)
    {
      TracePtr tr = TracePtr(new Trace(t, name));
      return tr;
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
  if (err)
    printf (": %s\n", strerror(err));
  else
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
};

/** list of URLs */
struct urldef URLs[] = {
#undef L2_NAME
#define L2_NAME(a) { a##_PREFIX, a##_CREATE },
#include "layer2create.h"
  {0, 0}
};

/** determines the right backend for the url and creates it */
Layer2Ptr 
Create (const char *url, L2options *opt)
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
        return u->Create (url + p + 1, opt);
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
  if (sscanf (addr, "%d.%d.%d", &a, &b, &c) != 3)
    die ("Address needs to look like X.X.X");
  return ((a & 0x0f) << 12) | ((b & 0x0f) << 8) | ((c & 0xff));
}

bool
readaddrblock (struct arguments *args, const char *addr)
{
  int a, b, c;
  if (sscanf (addr, "%d.%d.%d:%d", &a, &b, &c, &args->alloc_addrs_len) != 4)
    die ("Address block needs to look like X.X.X:X");
  args->alloc_addrs = ((a & 0x0f) << 12) | ((b & 0x0f) << 8) | ((c & 0xff));
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
  "Arguments are processed in order.\n"
  "Modifiers affect the device mentioned afterwards.\n"
  ;

/** documentation for arguments*/
static char args_doc[] = "URL";

/** option list */
static struct argp_option options[] = {
  {"listen-tcp", 'i', "PORT", OPTION_ARG_OPTIONAL,
   "listen at TCP port PORT (default 6720)"},
  {"listen-local", 'u', "FILE", OPTION_ARG_OPTIONAL,
   "listen at Unix domain socket FILE (default /run/knx)"},
  {"trace", 't', "MASK", 0,
   "set trace flags (bitmask)"},
  {"error", 'f', "LEVEL", 0,
   "set error level"},
  {"eibaddr", 'e', "EIBADDR", 0,
   "set our EIB address to EIBADDR (default 0.0.1)"},
  {"client-addrs", 'E', "ADDRSTART", 0,
   "assign addresses ADDRSTART through ADDRSTART+n to clients"},
  {"pid-file", 'p', "FILE", 0, "write the PID of the process to FILE"},
  {"daemon", 'd', "FILE", OPTION_ARG_OPTIONAL,
   "start the programm as daemon. Output will be written to FILE if given"},
#ifdef HAVE_EIBNETIPSERVER
  {"Tunnelling", 'T', 0, 0,
   "enable EIBnet/IP Tunneling in the EIBnet/IP server"},
  {"Routing", 'R', 0, 0,
   "enable EIBnet/IP Routing in the EIBnet/IP server"},
  {"Discovery", 'D', 0, 0,
   "enable the EIBnet/IP server to answer discovery and description requests (SEARCH, DESCRIPTION)"},
  {"Server", 'S', "ip[:port]", OPTION_ARG_OPTIONAL,
   "starts an EIBnet/IP multicast server"},
  {"Name", 'n', "SERVERNAME", 0,
   "name of the EIBnet/IP server (default is 'knxd')"},
#endif
  {"layer2", 'b', "driver:[arg]", 0,
   "a Layer-2 driver to use (knxd supports more than one)"},
#ifdef HAVE_GROUPCACHE
  {"GroupCache", 'c', 0, 0,
   "enable caching of group communication network state"},
#endif
#ifdef HAVE_EIBNETIPTUNNEL
  {"no-tunnel-client-queuing", OPT_BACK_TUNNEL_NOQUEUE, 0, 0,
   "wait 30msec between transmitting packets. Obsolete, please use --send-delay=30"},
#endif
#if defined(HAVE_TPUARTs) || defined(HAVE_TPUARTs_TCP)
  {"tpuarts-ack-all-group", OPT_BACK_TPUARTS_ACKGROUP, 0, 0,
   "tpuarts backend should generate L2 acks for all group telegrams"},
  {"tpuarts-ack-all-individual", OPT_BACK_TPUARTS_ACKINDIVIDUAL, 0, 0,
   "tpuarts backend should generate L2 acks for all individual telegrams"},
  {"tpuarts-disch-reset", OPT_BACK_TPUARTS_DISCH_RESET, 0, 0,
   "tpuarts backend should should use a full interface reset (for Disch TPUART interfaces)"},
#endif
  {"send-delay", OPT_BACK_SEND_DELAY, "DELAY", OPTION_ARG_OPTIONAL,
   "wait after sending a packet"},
  {"no-emi-send-queuing", OPT_BACK_EMI_NOQUEUE, 0, 0,
   "wait for ACK after transmitting packets. Obsolete, please use --send-delay=500"},
  {"no-monitor", 'N', 0, 0,
   "the next Layer2 interface may not enter monitor mode"},
  {"allow-forced-broadcast", OPT_FORCE_BROADCAST, 0, 0,
   "Treat routing counter 7 as per KNX spec (dangerous)"},
  {"stop-right-now", OPT_STOP_NOW, 0, OPTION_HIDDEN,
   "immediately stops the server after a successful start"},
  {0}
};

/** parses and stores an option */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = (struct arguments *) state->input;
  switch (key)
    {
#ifdef HAVE_EIBNETIPSERVER
    case 'T':
      arguments->tunnel = 1;
      arguments->has_work++;
      break;
    case 'R':
      arguments->route = 1;
      arguments->has_work++;
      break;
    case 'D':
      arguments->discover = 1;
      break;
    case 'S':
      {
        const char *serverip;
        const char *name = arguments->eibnetname;
        std::string tracename;

        EIBnetServerPtr c;
        int port = 0;
        char *a = strdup (OPT_ARG(arg, state, ""));
        char *b;
        if (!a)
          die ("out of memory");
        b = strchr (a, ':');
        if (b)
          {
            *b++ = 0;
            port = atoi (b);
          }
        if (port <= 0)
          port = 3671;
        serverip = a;
        if (!*serverip) 
          serverip = "224.0.23.12";

        if (!name || !*name) {
            name = "knxd";
            tracename = "mcast";
        } else {
            tracename = "mcast:";
            tracename += name;
        }

        c = EIBnetServerPtr(new EIBnetServer (arguments->tracer(tracename), name));
        if (!c->init (arguments->l3(), serverip, port, arguments->tunnel, arguments->route, arguments->discover))
        {
          free(a);
          die ("initialization of the EIBnet/IP server failed");
        }
        free (a);
        arguments->tunnel = false;
        arguments->route = false;
        arguments->discover = false;
        arguments->eibnetname = 0;
      }
      break;
    case 'n':
      arguments->eibnetname = (char *)arg;
      if(arguments->eibnetname[0] == '=')
        arguments->eibnetname++;
      if(strlen(arguments->eibnetname) >= 30)
        die("EIBnetServer/IP name must be shorter than 30 bytes");
      break;
#endif
    case 'u':
      {
        BaseServerPtr s;
        const char *name = OPT_ARG(arg,state,"/run/knx");
        s = BaseServerPtr(new LocalServer (arguments->tracer(name), name));
        if (!s->init (arguments->l3()))
          die ("initialisation of the knxd unix protocol failed");
        arguments->has_work++;
      }
      break;
    case 'i':
      {
        BaseServerPtr s = nullptr;
        int port = atoi(OPT_ARG(arg,state,"6720"));
        if (port > 0)
          s = BaseServerPtr(new InetServer (arguments->tracer("inet"), port));
        if (!s || !s->init (arguments->l3()))
          die ("initialisation of the knxd inet protocol failed");
        arguments->has_work++;
      }
      break;
    case 't':
      if (arg)
        {
          char *x;
          unsigned long level = strtoul(arg, &x, 0);
          if (*x)
            die ("Trace level: '%s' is not a number", arg);
          arguments->t.SetTraceLevel (level);
        }
      else
        arguments->t.SetTraceLevel (0);
      break;
    case 'f':
      arguments->t.SetErrorLevel (arg ? atoi (arg) : 0);
      break;
    case 'e':
      if (arguments->has_l3 ())
	{
	  die ("You need to specify '-e' earlier");
	}
      arguments->addr = readaddr (arg);
      break;
    case 'E':
      readaddrblock (arguments, arg);
      break;
    case 'p':
      arguments->pidfile = arg;
      break;
    case 'd':
      arguments->daemon = OPT_ARG(arg,state,"/dev/null");
      break;
#if HAVE_GROUPCACHE
    case 'c':
      if (!CreateGroupCache (arguments->l3(), arguments->tracer("cache"), true))
        die ("initialisation of the group cache failed");
      break;
#endif
    case OPT_FORCE_BROADCAST:
      arguments->force_broadcast = true;
      break;
    case OPT_STOP_NOW:
      arguments->stop_now = true;
      break;
    case OPT_BACK_TUNNEL_NOQUEUE: // obsolete
      ERRORPRINTF (&arguments->t, E_WARNING | 41, 0, "The option '--no-tunnel-client-queuing' is obsolete.");
      ERRORPRINTF (&arguments->t, E_WARNING | 42, 0, "Please use '--send-delay=30'.");
      arguments->l2opts.send_delay = 30; // msec
      break;
    case OPT_BACK_EMI_NOQUEUE: // obsolete
      ERRORPRINTF (&arguments->t, E_WARNING | 43, 0, "The option '--no-emi-send-queuing' is obsolete.");
      ERRORPRINTF (&arguments->t, E_WARNING | 44, 0, "Please use '--send-delay=500'.");
      arguments->l2opts.send_delay = 500; // msec
      break;
    case OPT_BACK_SEND_DELAY:
      arguments->l2opts.send_delay = atoi(OPT_ARG(arg,state,"30"));
      break;
    case OPT_BACK_TPUARTS_ACKGROUP:
      arguments->l2opts.flags |= FLAG_B_TPUARTS_ACKGROUP;
      break;
    case OPT_BACK_TPUARTS_ACKINDIVIDUAL:
      arguments->l2opts.flags |= FLAG_B_TPUARTS_ACKINDIVIDUAL;
      break;
    case OPT_BACK_TPUARTS_DISCH_RESET:
      arguments->l2opts.flags |= FLAG_B_TPUARTS_DISCH_RESET;
      break;
    case 'N':
      arguments->l2opts.flags |= FLAG_B_NO_MONITOR;
      break;
    case ARGP_KEY_ARG:
    case 'b':
      {
	arguments->l2opts.t = arguments->tracer(arg);
        Layer2Ptr l2 = Create (arg, &arguments->l2opts);
        if (!l2 || !l2->init (arguments->l3 ()))
          die ("initialisation of backend '%s' failed", arg);
	if (arguments->l2opts.flags || arguments->l2opts.send_delay)
          die ("You provided options which '%s' does not recognize", arg);
        memset(&arguments->l2opts, 0, sizeof(arguments->l2opts));
        arguments->has_work++;
        break;
      }
    case ARGP_KEY_FINI:
      if (arguments->l2opts.flags)
        die ("You need to use backend flags in front of the affected backend");

#ifdef HAVE_SYSTEMD
      {
        BaseServerPtr s = nullptr;
        const int num_fds = sd_listen_fds(0);

        if( num_fds < 0 )
          die("Error getting fds from systemd.");
        // zero FDs from systemd is not a bug

        for( int fd = SD_LISTEN_FDS_START; fd < SD_LISTEN_FDS_START+num_fds; ++fd )
          {
            if( sd_is_socket(fd, AF_UNSPEC, SOCK_STREAM, 1) <= 0 )
              die("Error: socket not of expected type.");

            s = BaseServerPtr(new SystemdServer(arguments->tracer("systemd"), fd));
            if (!s->init (arguments->l3()))
              die ("initialisation of the systemd socket failed");
            arguments->has_work++;
          }
      }
#endif

	  errno = 0;
      if (arguments->tunnel || arguments->route || arguments->discover || 
          arguments->eibnetname)
        die ("Option '-S' starts the multicast server.\n"
             "-T/-R/-D/-n after or without that option are useless.");
      if (arguments->l2opts.flags)
	die ("You provided L2 flags after specifying an L2 interface.");
      if (arguments->has_work == 0)
        die ("I know about no interface. Nothing to do. Giving up.");
      if (arguments->has_work == 1)
        die ("I only have one interface. Nothing to do. Giving up.");
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/** information for the argument parser*/
static struct argp argp = { options, parse_opt, args_doc, doc };

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

// #define EV_TRACE

static void
signal_cb (EV_P_ ev_signal *w, int revents)
{
  ev_break (EV_A_ EVBREAK_ALL);
}

#ifdef EV_TRACE
static void
timeout_cb (EV_P_ ev_timer *w, int revents)
{
  printf("LIBEV ping\n");
}
#endif

struct _hup {
  struct ev_signal sighup;
  const char *daemon;
  TracePtr t;
} hup;

static void
sighup_cb (EV_P_ ev_signal *w, int revents)
{
  struct _hup *hup = (struct _hup *)w;

  int fd = open (hup->daemon, O_WRONLY | O_APPEND | O_CREAT, FILE_MODE);
  if (fd == -1)
  {
    ERRORPRINTF (hup->t, E_ERROR | 21, 0, "can't open log file %s",
                hup->daemon);
    return;
  }
  close (1);
  close (2);
  dup2 (fd, 1);
  dup2 (fd, 2);
  close (fd);
}

int
main (int ac, char *ag[])
{
  int index;
#ifdef HAVE_PTHSEM
  pth_init ();
#endif
  setlinebuf(stdout);

// set up libev
#if EV_MULTIPLICITY
  typedef struct ev_loop *LOOP_RESULT;
#else
  typedef int LOOP_RESULT;
#endif
#ifdef HAVE_PTHSEM
  LOOP_RESULT loop = ev_default_loop(EVFLAG_AUTO | EVFLAG_NOSIGMASK | EVBACKEND_PTHSEM);
#else
  LOOP_RESULT loop = ev_default_loop(EVFLAG_AUTO | EVFLAG_NOSIGMASK);
#endif
  assert (loop);

#ifdef EV_TRACE
  struct ev_timer timer;
#endif
  struct _hup hup;
  struct ev_signal sigint;
  struct ev_signal sigterm;

#ifdef EV_TRACE
  printf("LIBEV starting up\n");
#endif

#ifdef EV_TRACE
  ev_timer_init (&timer, timeout_cb, 1., 10.);
  ev_timer_again (EV_A_ &timer);
#endif
  if (arg.daemon) {
    hup.t = TracePtr(new Trace(arg.t,"reload"));
    hup.daemon = arg.daemon;
    ev_signal_init (&hup.sighup, sighup_cb, SIGINT);
    ev_signal_start (EV_A_ &hup.sighup);
  }

  ev_signal_init (&sigint, signal_cb, SIGINT);
  ev_signal_start (EV_A_ &sigint);
  ev_signal_init (&sigterm, signal_cb, SIGTERM);
  ev_signal_start (EV_A_ &sigterm);

  arg.errorlevel = LEVEL_WARNING;

  argp_parse (&argp, ac, ag, ARGP_IN_ORDER, &index, &arg);

  // if you ever want this to be fatal, doing it here would be too late
  if (getuid () == 0)
    ERRORPRINTF (&arg.t, E_WARNING | 20, 0, "EIBD should not run as root");

  signal (SIGPIPE, SIG_IGN);

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

  // main loop
#ifdef HAVE_SYSTEMD
  sd_notify(0,"READY=1");
#endif

  // now wait for events
  ev_run (EV_A_ arg.stop_now ? EVRUN_NOWAIT : 0);

#ifdef HAVE_SYSTEMD
  sd_notify(0,"STOPPING=1");
#endif

  ev_break(EV_A_ EVBREAK_ALL);

  arg.free_l3();

  if (arg.pidfile)
    unlink (arg.pidfile);

#ifdef HAVE_PTHSEM
  pth_yield (0);
  pth_yield (0);
  pth_yield (0);
  pth_yield (0);
  pth_exit (0);
#endif
  return 0;
}
