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
#include "router.h"
#include "version.h"
#include "link.h"

#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

// option values
bool stop_now = false;
bool do_list = false;
bool background = false;
bool stopping = false;
const char *pidfile = NULL;
const char *logfile = NULL;
const char *cfgfile = NULL;
const char *mainsection = NULL;
char *const *argv;

LOOP_RESULT loop;

/** aborts program with a printf like message */
void die (const char *msg, ...);

void usage()
{
  die("Usage: knxd configfile [main-section]");
}

// The NOQUEUE options are deprecated
#define OPT_STOP_NOW 1

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

  if (pidfile)
    unlink (pidfile);

  exit (1);
}

/** version */
const char *argp_program_version = "knxd " REAL_VERSION;
/** documentation */
static char doc[] =
  "knxd -- a commonication stack for EIB/KNX\n"
  "(C) 2005-2015 Martin Koegler <mkoegler@auto.tuwien.ac.at> et al.\n"
  "(C) 2016-2017 Matthias Urlichs <matthias@urlichs.de>\n"
  "\n"
  "Usage: knxd configfile [main-section]\n"
  "\n"
  "Please read the documentation for details.\n"
  ;

/** option list */
static struct argp_option options[] = {
  {"stop", OPT_STOP_NOW, 0, OPTION_HIDDEN,
   "immediately stops the server after a successful start"},
  {"list", 'l', 0, 0,
   "list known drivers, filters, or servers"},
  {0}
};

void fork_args_helper()
{
  int fifo[2];
  if (pipe(fifo) == -1)
    die("pipe");
  pid_t pid = fork();
  if (pid == -1)
    die("could not fork");
  if (pid == 0)
    {
      close(fifo[0]);
      dup2(fifo[1],1);
      close(fifo[1]);

      char *s = (char *)malloc(strlen(argv[0])+7);
      sprintf(s,"%s_args",argv[0]);
      execvp(s, argv);

      execv(LIBEXECDIR "/knxd_args", argv);

      die("could not exec knxd_args helper");
      exit(1);
    }
  close(fifo[1]);
  dup2(fifo[0],0);
  close(fifo[0]);
  cfgfile = "-";
  mainsection = "main";
}

/** parses and stores an option */
static error_t
parse_opt (int key, char *arg, struct argp_state *state UNUSED)
{
  switch (key)
    {
    case ARGP_KEY_INIT:
      break;
    case OPT_STOP_NOW:
      stop_now = true;
      break;

    case 'l':
      do_list = true;
      break;

    case ARGP_KEY_ARG:
      if (cfgfile == NULL)
        cfgfile = arg;
      else if (mainsection == NULL)
        mainsection = arg;
      else
        usage();
      break;

    case ARGP_KEY_SUCCESS:
      return 0;

    case ARGP_KEY_NO_ARGS:
      if (!do_list)
        usage();
      return 0;

    case ARGP_KEY_ARGS:
    case ARGP_KEY_FINI:
    case ARGP_KEY_END:
      if (cfgfile != NULL)
        break;
    case ARGP_KEY_ERROR:
    default:
      if (!do_list)
        fork_args_helper();
      return 1;
    }
  return 0;
}

const char args_doc[] = "config-file [main-section]";

/** information for the argument parser*/
static struct argp argp = { options, parse_opt, args_doc, doc };

// #define EV_TRACE

static void
signal_cb (EV_P_ ev_signal *w UNUSED, int revents UNUSED)
{
  ev_break (EV_A_ EVBREAK_ALL);
  stopping = true;
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
  const char *logfile;
  TracePtr t;
} hup;

static void
sighup_cb (EV_P_ ev_signal *w, int revents UNUSED)
{
  struct _hup *hup = (struct _hup *)w;

  if(hup->logfile && hup->logfile[0])
    {
      int fd = open (hup->logfile, O_WRONLY | O_APPEND | O_CREAT, 0660);
      if (fd == -1)
      {
        ERRORPRINTF (hup->t, E_ERROR | 21, "can't open log file %s", hup->logfile);
        return;
      }
      close (1);
      close (2);
      dup2 (fd, 1);
      dup2 (fd, 2);
      close (fd);
    }
}

int
main (int ac, char *ag[])
{
  int index;
  setlinebuf(stdout);

  argv = ag;

// set up libev
  loop = ev_default_loop(EVFLAG_AUTO | EVFLAG_NOSIGMASK | EVBACKEND_SELECT);
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

#ifdef HAVE_SYSTEMD
  num_fds = sd_listen_fds(0);
  if( num_fds < 0 )
    die("Error getting fds from systemd.");
#endif
#ifdef EV_TRACE
  ev_timer_init (&timer, timeout_cb, 1., 10.);
  ev_timer_again (EV_A_ &timer);
#endif

  std::string arg_str = "";
  for (index=0; index<ac; index++)
    {
      arg_str += ' ';
      arg_str += ag[index];
    }

  argp_parse (&argp, ac, ag, ARGP_NO_EXIT | ARGP_NO_ERRS | ARGP_IN_ORDER, &index, NULL);

  if (do_list)
    {
      static Factory<Server> _servers;
      static Factory<Driver> _drivers;
      static Factory<Filter> _filters;

      if (cfgfile == NULL)
        {
        x1:
          printf(""
            "Requires type of structure to list.\n"
            "Either 'driver', 'filter' or 'server'.\n"
            );
        }
      else // if (mainsection == NULL)
        {
          if (!strcmp(cfgfile, "driver"))
            {
              for(auto &m : _drivers.Instance().map())
                printf("%s\n",m.first.c_str());
            }
          else if (!strcmp(cfgfile, "filter"))
            {
              for(auto &m : _filters.Instance().map())
                printf("%s\n",m.first.c_str());
            }
          else if (!strcmp(cfgfile, "server"))
            {
              for(auto &m : _servers.Instance().map())
                printf("%s\n",m.first.c_str());
            }
          else
            goto x1;
        }
      // else
      //   {
      //     printf("Detail printing is not implemented yet.\n");
      //   }
      return 0;
    }
  if (cfgfile == NULL)
    usage();
  if (mainsection == NULL)
    mainsection = "main";

  IniData i;
  int errl = i.parse(cfgfile);
  if (errl)
    die("Parse error of '%s' in line %d", cfgfile, errl);
  IniSectionPtr main = i[mainsection];

  pidfile = main->value("pidfile","").c_str();
  if (num_fds)
    pidfile = "";

  logfile = main->value("logfile","").c_str();
  if (num_fds)
    logfile = NULL;

  background = main->value("background",false);
  if (num_fds)
    background = false;

  if (!stop_now)
    stop_now = main->value("stop-after-setup",false);

  if (logfile && *logfile)
    {
      int fd = open (logfile, O_WRONLY | O_APPEND | O_CREAT, 0660);
      if (fd == -1)
        die ("Can not open file %s", logfile);
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

  Router *r = new Router(i,mainsection);

  ERRORPRINTF (r->t, E_INFO | 0, "%s:%s", REAL_VERSION, arg_str);

  if (!r->setup())
    {
      ERRORPRINTF(r->t, E_FATAL,"Error setting up the KNX router.");
      exit(2);
    }
  if (!strcmp(cfgfile, "-"))
    ERRORPRINTF(r->t, E_WARNING,"Consider using a config file.");

  if (background) {
    hup.t = TracePtr(new Trace(*r->t));
    hup.t->setAuxName("reload");
    hup.logfile = logfile;
    ev_signal_init (&hup.sighup, sighup_cb, SIGHUP);
    ev_signal_start (EV_A_ &hup.sighup);
  }

  signal (SIGPIPE, SIG_IGN);

  if (getuid () == 0)
    ERRORPRINTF (r->t, E_WARNING | 20, "knxd should not run as root");

  if (!stop_now)
    {
      r->start();

      ev_signal_init (&sigint, signal_cb, SIGINT);
      ev_signal_start (EV_A_ &sigint);
      ev_signal_init (&sigterm, signal_cb, SIGTERM);
      ev_signal_start (EV_A_ &sigterm);
    }

  FILE *pidf;
  if (pidfile && *pidfile)
    if ((pidf = fopen (pidfile, "w")) != NULL)
      {
        fprintf (pidf, "%d", getpid ());
        fclose (pidf);
      }

  // main loop
#ifdef HAVE_SYSTEMD
  sd_notify(0,"READY=1");
#endif

  // now wait for events
  ev_run (EV_A_ stop_now ? EVRUN_NOWAIT : 0);

#ifdef HAVE_SYSTEMD
  sd_notify(0,"STOPPING=1");
#endif
  ERRORPRINTF(r->t, E_NOTICE,"Shutting down.");

  stopping = false; // re-set by a second signal
  r->stop();
  while (!r->isIdle() && !stopping)
    ev_run (EV_A_ stop_now ? EVRUN_NOWAIT : EVRUN_ONCE);

  int exitcode = r->exitcode;
  delete r;

  if (pidfile && *pidfile)
    unlink (pidfile);

  return exitcode;
}
