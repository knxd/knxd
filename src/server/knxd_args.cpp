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
#include <iostream>
#include <stdarg.h>
#include <unordered_map>

#include "inifile.h"
#include "types.h"
#include "trace.h"

/** aborts program with a printf like message */
void die (const char *msg, ...);

typedef struct {
  unsigned int flags;
  unsigned int send_delay;
  TracePtr t;
} L2options;

#define FLAG_B_TUNNEL_NOQUEUE (1<<0)
#define FLAG_B_TPUARTS_ACKGROUP (1<<1)
#define FLAG_B_TPUARTS_ACKINDIVIDUAL (1<<2)
#define FLAG_B_TPUARTS_DISCH_RESET (1<<3)
#define FLAG_B_EMI_NOQUEUE (1<<4)
#define FLAG_B_NO_MONITOR (1<<5)

// The NOQUEUE options are deprecated
#define OPT_BACK_TUNNEL_NOQUEUE 1
#define OPT_BACK_TPUARTS_ACKGROUP 2
#define OPT_BACK_TPUARTS_ACKINDIVIDUAL 3
#define OPT_BACK_TPUARTS_DISCH_RESET 4
#define OPT_BACK_EMI_NOQUEUE 5
#define OPT_STOP_NOW 6
#define OPT_FORCE_BROADCAST 7
#define OPT_BACK_SEND_DELAY 8
#define OPT_SINGLE_PORT 9
#define OPT_MULTI_PORT 10
#define OPT_NO_TIMESTAMP 11

#define OPT_ARG(_arg,_state,_default) (arg ? arg : \
        (state->argv[state->next] && state->argv[state->next][0] && (state->argv[state->next][0] != '-')) ?  \
            state->argv[state->next++] : _default)

#define ADD(a,b) do { \
    if (a.size()) a.push_back(','); \
    a.append(b); \
} while(0)

IniData ini;
char link[99] = "@.";
void link_to(const char *arg)
{
  char *p;
  ++*link;
  strcpy(link+2,arg);
  p = strchr(link+2,':');
  if (p)
    *p = 0;
}

/** structure to store the arguments */
class arguments
{
public:
  /** -D -T -R has been used */
  bool want_server;
  /** port to listen */
  int port;
  /** path for unix domain socket */
  const char *name;
  /** path to pid file */
  const char *pidfile;
  /** path to trace log file */
  const char *daemon;
  /** trace level */
  int tracelevel = -1;
  int errorlevel = -1;
  bool no_timestamps = false;
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
  bool single_port = true;
  const char *intf = nullptr;

  L2options l2opts;
  const char *serverip;
  std::string servername = "knxd";

  bool stop_now;
  bool force_broadcast;

public:
  /** The current tracer */
  Array < std::string > filters;

  arguments () { }
  virtual ~arguments () { }

  /** get the current tracer.
   * Call with 'true' when you want to change the tracer
   * and with 'false' when you want to use it.
   *
   * If the current tracer has been used, it's not modified; instead, it is
   * passed to Router (which will deallocate it when it ends) and copied to
   * a new instance.
   */

  std::unordered_map<std::string,std::string> more_args;
  void add_arg(char *arg)
    {
      if (!*arg)
        return;

      char *val = strchr(arg,'=');
      if (val)
        {
          *val++ = 0;
          more_args.emplace(arg,val);
        }
      else if (*arg == '!' && arg[1])
        more_args.emplace(arg+1,"false");
      else if (*arg == '!')
        return;
      else
        more_args.emplace(arg,"true");
    }

  void do_filter(const char *name)
    {
      if (more_args.size() > 0)
        {
          link_to(name);
          ITER(i, more_args)
            (*ini[link])[i->first] = i->second;
          (*ini[link])["filter"] = name;
          more_args.clear();
          filters.push_back(link);
        }
      else
        filters.push_back(name);
    }

  void stack(const std::string section, bool clear = true)
    {
      if (l2opts.send_delay)
        {
          char buf[15];
          snprintf(buf,sizeof(buf),"delay=%d",l2opts.send_delay);
          add_arg(buf);
          do_filter("pace");
        }
      ITER(i,filters)
        ADD((*ini[section])["filters"],(*i));
      if (l2opts.flags & FLAG_B_TPUARTS_ACKGROUP)
        (*ini[section])["ack-group"] = "true";
      if (l2opts.flags & FLAG_B_TPUARTS_ACKINDIVIDUAL)
        (*ini[section])["ack-individual"] = "true";
      if (l2opts.flags & FLAG_B_TPUARTS_DISCH_RESET)
        (*ini[section])["reset"] = "true";
      if (l2opts.flags & FLAG_B_NO_MONITOR)
        (*ini[section])["monitor"] = "false";
      ITER(i, more_args)
        (*ini[section])[i->first] = i->second;
      more_args.clear();
      if (tracelevel >= 0 || errorlevel >= 0 || no_timestamps) {
          char b1[10],b2[50];
          snprintf(b2,sizeof(b2),"debug-%s",section.c_str());
          (*ini[section])["debug"] = b2;
          if (tracelevel >= 0)
            {
              snprintf(b1,sizeof(b1),"0x%x",tracelevel);
              (*ini[b2])["trace-mask"] = b1;
            }
          if (errorlevel >= 0)
            {
              snprintf(b1,sizeof(b1),"0x%x",errorlevel);
              (*ini[b2])["error-level"] = b1;
            }
          if (no_timestamps)
            {
              (*ini[b2])["timestamps"] = "false";
            }
      }
      if (clear)
        reset();
    }
  void reset()
    {
        filters.clear();
        l2opts = L2options();
        tracelevel = -1;
        errorlevel = -1;
        no_timestamps = false;
    }
};

void driver_argsv(const char *arg, char *ap, ...);
void driver_argsv(const char *arg, char *ap, ...)
{
  va_list apl;
  va_start(apl, ap);
  (*ini[link])["driver"] = arg;

  while(ap)
    {
      char *p2 = strchr(ap,':');
      if (p2)
        *p2++ = '\0';
      char *pa = va_arg(apl, char *);
      if (!pa)
        {
          if (!*ap && !p2)
            break;
          die ("Too many arguments for %s!", arg);
        }
      if (*pa == '!') // required-argument flag
        pa++;
      if (*ap) // skip empty arguments
        (*ini[link])[pa] = ap;
      ap = p2;
    }
    char *pa = va_arg(apl, char *);
    if (pa && *pa == '!')
      die("%s requires an argument (%s)", arg,pa+1);
    va_end(apl);
}

void driver_args(const char *arg, char *ap)
{
  if(!strcmp(arg,"ip"))
    driver_argsv(arg,ap, "multicast-address","port","interface", NULL);
  else if(!strcmp(arg,"tpuarttcp") || !strcmp(arg,"ft12tcp") || !strcmp(arg,"ncn5120tcp") || !strcmp(arg,"ft12cemitcp"))
    {
      char cut[20];
      strcpy(cut,arg);
      cut[strlen(cut)-3] = 0; // drop the "tcp" at the end
      driver_argsv(cut,ap, "!ip-address","!dest-port", NULL);
    }
  else if(!strcmp(arg,"usb"))
    driver_argsv(arg,ap, "bus","device","config","interface", NULL);
  else if(!strcmp(arg,"ipt"))
    driver_argsv(arg,ap, "!ip-address","dest-port","src-port", NULL);
  else if(!strcmp(arg,"iptn"))
    {
      driver_argsv("ipt",ap, "!ip-address","dest-port","src-port","nat-ip","data-port", NULL);
      (*ini[link])["nat"] = "true";
    }
  else if(!strcmp(arg,"ft12") || !strcmp(arg,"ncn5120") || !strcmp(arg,"tpuarts") || !strcmp(arg,"ft12cemi"))
    {
      if (!strcmp(arg,"tpuarts"))
        arg = "tpuart";
      driver_argsv(arg,ap, "!device","baudrate", NULL);
    }
  else if(!strcmp(arg,"dummy"))
    driver_argsv(arg,ap, NULL);
  else
    die ("I don't know of options for %s",arg);
}

/** storage for the arguments*/
arguments arg;

/** number of file descriptors passed in by systemd */
#ifdef HAVE_SYSTEMD
int num_fds;
#else
const int num_fds = 0;
#endif

/** aborts program with a printf like message */
void
die (const char *msg, ...)
{
  va_list ap;
  int err = errno;

  va_start (ap, msg);
  vfprintf (stderr, msg, ap);
  if (err)
    fprintf (stderr, ": %s\n", strerror(err));
  else
    fprintf (stderr, "\n");
  va_end (ap);

  exit (1);
}

/** parses an EIB individual address */
void
readaddr (const char *addr)
{
  int a, b, c;
  if (sscanf (addr, "%d.%d.%d", &a, &b, &c) != 3)
    die ("Address needs to look like X.X.X");
  (*ini["main"])["addr"] = addr;
}

void
readaddrblock (const char *addr)
{
  int a, b, c, d;
  if (sscanf (addr, "%d.%d.%d:%d", &a, &b, &c, &d) != 4)
    die ("Address block needs to look like X.X.X:X");
  (*ini["main"])["client-addrs"] = addr;
}

/** version */
static struct argp_option options[] = {
  {"listen-tcp", 'i', "PORT", OPTION_ARG_OPTIONAL,
   "listen at TCP port PORT (default 6720)"},
  {"listen-local", 'u', "FILE", OPTION_ARG_OPTIONAL,
   "listen at Unix domain socket FILE (default /run/knx)"},
  {"no-timestamp", OPT_NO_TIMESTAMP, 0, 0,
   "don't print timestamps when logging"},
  {"trace", 't', "MASK", 0,
   "set trace flags (bitmask)"},
  {"error", 'f', "LEVEL", 0,
   "set error level (default 3: warnings)"},
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
  {"Interface", 'I', "intf", 0,
   "Interface to use"},
  {"Name", 'n', "SERVERNAME", 0,
   "name of the EIBnet/IP server (default is 'knxd')"},
  {"single-port", OPT_SINGLE_PORT, 0, 0,
   "Use one common port for multicast. This is an ETS4/ETS5 bug workaround."},
  {"multi-port", OPT_MULTI_PORT, 0, 0,
   "Use two ports for multicast. This lets you run multiple KNX processes."},
#endif
  {"layer2", 'b', "driver:[arg]", 0,
   "a Layer-2 driver to use (knxd supports more than one)"},
  {"filter", 'B', "filter", 0,
   "a Layer-2 filter to use in front of the next driver"},
  {"arg", 'A', "name=value", 0,
   "an additional configuration item for the next filter or driver"},
#ifdef HAVE_GROUPCACHE
  {"GroupCache", 'c', "SIZE", OPTION_ARG_OPTIONAL,
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
   "the next Driver interface may not enter monitor mode"},
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
    case 'T':
      arguments->stack("tunnel");
      (*ini["server"])["tunnel"] = "tunnel";
      arguments->want_server = true;
      break;
    case 'R':
      arguments->stack("router");
      (*ini["server"])["router"] = "router";
      arguments->want_server = true;
      // (*ini["router"])["driver"] = "ets-multicast";
      break;
    case 'D':
      arguments->stack("server"); // to allow for -t255 -DTRS
      arguments->want_server = true;
      (*ini["server"])["discover"] = "true";
      break;
    case OPT_SINGLE_PORT:
      (*ini["server"])["multi-port"] = "false";
      break;
    case OPT_MULTI_PORT:
      (*ini["server"])["multi-port"] = "true";
      break;
    case 'I':
      (*ini["server"])["interface"] = arg;
      break;
    case 'S':
      {
        if (arguments->filters.size())
          die("Use filters in front of -R or -T, not -S");
        ADD((*ini["main"])["connections"], "server");
        (*ini["server"])["server"] = "ets_router";
        arguments->want_server = false;
        // (*ini["server"])["driver"] = "ets-link";

        const char *serverip;
        const char *name = arguments->servername.c_str();
        std::string tracename;

        int port = 0;
        char *a = strdup (OPT_ARG(arg, state, ""));
        char *b = strchr (a, ':');
        if (b)
          {
            *b++ = 0;
            if (atoi (b) > 0)
              (*ini["server"])["port"] = b;
          }
        if (*a) 
          (*ini["server"])["multicast-address"] = a;

        if (!name || !*name) {
            name = "knxd";
            tracename = "mcast";
        } else {
            tracename = "mcast:";
            tracename += name;
        }
        (*ini["debug-server"])["name"] = tracename;
        (*ini["server"])["debug"] = "debug-server";
        arguments->stack("server");
        break;
      }

    case 'n':
      if (*arg == '=')
	arg++;
      if(strlen(arg) >= 30)
        die("Server name must be shorter than 30 bytes");
      (*ini["main"])["name"] = arg;
      break;

    case 'u':
      {
        if (arguments->want_server)
          die("You need -S after -D/-T/-R");
        link_to("unix");
        ADD((*ini["main"])["connections"], link);
        (*ini[link])["server"] = "knxd_unix";
        // (*ini[link])["driver"] = "knx-link";
        const char *name = OPT_ARG(arg,state,NULL);
        if (name)
          {
            (*ini[link])["path"] = name;
            (*ini[link])["systemd-ignore"] = "false";
          }
        else
          (*ini[link])["systemd-ignore"] = "true";
        arguments->stack(link);
      }
      break;

    case 'i':
      {
        if (arguments->want_server)
          die("You need -S after -D/-T/-R");
        link_to("tcp");
        ADD((*ini["main"])["connections"], link);
        (*ini[link])["server"] = "knxd_tcp";
        // (*ini[link])["driver"] = "knx-link";
        const char *port = OPT_ARG(arg,state,"");
        if (*port && atoi(port) > 0)
          {
            (*ini[link])["port"] = port;
            (*ini[link])["systemd-ignore"] = "false";
          }
        else
          (*ini[link])["systemd-ignore"] = "true";

        arguments->stack(link);
      }
      break;

    case 't':
      if (arg)
        {
          char *x;
          unsigned long level = strtoul(arg, &x, 0);
          if (*x)
            die ("Trace level: '%s' is not a number", arg);
          arguments->tracelevel = level;
        }
      else
        arguments->tracelevel = -1;
      break;
    case OPT_NO_TIMESTAMP:
      arguments->no_timestamps = true;
      break;
    case 'f':
      arguments->errorlevel = (arg ? atoi (arg) : 0);
      break;
    case 'e':
      if (arguments->l2opts.flags || arguments->l2opts.send_delay)
        die("You cannot use flags globally.");
      arguments->stack("main");
      readaddr (arg);
      break;
    case 'E':
      readaddrblock (arg);
      break;
    case 'p':
      (*ini["main"])["pidfile"] = arg;
      break;
    case 'd':
      {
        const char *arg = OPT_ARG(arg,state,NULL);
        (*ini["main"])["background"] = "true";
        if (arg)
          (*ini["main"])["logfile"] = arg;
      }
      break;
    case 'c':
      if (arguments->l2opts.flags || arguments->l2opts.send_delay)
        die("You cannot apply flags to the group cache.");

      link_to("cache");
      (*ini["main"])["cache"] = link;
      arguments->stack(link);
      break;
    case OPT_FORCE_BROADCAST:
      (*ini["main"])["force-broadcast"] = "true";
      break;
    case OPT_STOP_NOW:
      (*ini["main"])["stop-after-setup"] = "true";
      break;
    case OPT_BACK_TUNNEL_NOQUEUE: // obsolete
      fprintf(stderr,"The option '--no-tunnel-client-queuing' is obsolete.\n");
      fprintf(stderr,"Please use '--send-delay=30'.");
      arguments->l2opts.send_delay = 30; // msec
      break;
    case OPT_BACK_EMI_NOQUEUE: // obsolete
      fprintf(stderr,"The option '--no-emi-send-queuing' is obsolete.\n");
      fprintf(stderr,"Please use '--send-delay=500'.");
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
        if (arguments->want_server)
          die("You need -S after -D/-T/-R");
        link_to(arg);
        ADD((*ini["main"])["connections"], link);
        char *ap = strchr(arg,':');
        if (ap)
          *ap++ = '\0';
        driver_args(arg,ap);
        arguments->stack(link);
        break;
      }
    case 'B':
      arguments->do_filter(arg);
      break;
    case 'A':
      arguments->add_arg(arg);
      break;
    case ARGP_KEY_FINI:

      if (arguments->want_server)
        die("You need -S after -D/-T/-R");
#ifdef HAVE_SYSTEMD
      {
        (*ini["main"])["systemd"] = "systemd";
        // (*ini["systemd"])["server"] = "knxd_systemd";
        // (*ini["systemd"])["driver"] = "knx-link";
        arguments->stack("systemd");
      }
#endif
      if (arguments->filters.size() || arguments->more_args.size())
        die ("You need to use filters and arguments in front of the affected backend");
      if (arguments->l2opts.flags || arguments->l2opts.send_delay)
	die ("You provided flags after specifying an interface.");
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/** information for the argument parser*/
static struct argp argp = { options, parse_opt, "URL",
"knxd -- a stack for EIB/KNX\n"
"\n"
"Please read the manpage for detailed options.\n"
};

int
main (int ac, char *ag[])
{
  int index;
  setlinebuf(stdout);

  argp_parse (&argp, ac, ag, ARGP_IN_ORDER, &index, &arg);
  ini.write(std::cout);

  return 0;
}
