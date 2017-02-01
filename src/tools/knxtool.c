/*
    knxtool - multi function eibd client
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>
    
    First version by Jean-Francois Meessen <jef2000@ouaye.net>
    More applets ported by Marc Leeman

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
#include "path.h"
#include <time.h>
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

static char *prog;

static EIBConnection *
open_con (const char *uri)
{
  EIBConnection *con;
  if (!uri || !*uri)
    die ("usage: %s url [args]", prog);

  /* Open the Socket */
  con = EIBSocketURL (uri);
  if (!con)
    die ("Open failed");

  return con;
}

int
main (int ac, char *ag[])
{
  uchar buf[255];
  int len;
  EIBConnection *con;
  eibaddr_t dest;
  eibaddr_t src;

  /* check if the application is called by its original name
   * (and an applet name afterwards), or with a symbolic link */
  prog = strrchr (ag[0], '/');
  if (prog)
    prog += 1;
  else
    prog = ag[0];
  if (strcmp(prog, "knxtool") == 0 || strcmp(prog, "lt-knxtool") == 0)
    {
      if (ac < 2)
	die ("usage: %s applet [args] -- say '%s list' to get a list of applets", prog,prog);
      ac--;
      ag++;
      prog = ag[0];
    }
  if (strncmp (prog, "knx", 3) == 0)
    prog += 3;
  if (strcmp (prog , "list") == 0)
    {
      printf("on off write swrite read if readtemp dimup log \n\
busmonitor1 busmonitor2 readindividual progmodeon progmodeoff progmodetoggle progmodestatus maskver \n\
writeaddress vbusmonitor1 vbusmonitor2 mprogmodeon mprogmodeoff mprogmodetoggle mprogmodestatus mmaskver \n\
mpeitype madcread mread mwrite mpropread mpropwrite mpropdesc mpropscan groupread groupswrite groupwrite \n\
msetkey grouplisten groupresponse groupsresponse groupsocketlisten groupsocketread mpropscanpoll \n\
vbusmonitor1poll groupreadresponse groupcacheenable groupcachedisable groupcacheclear groupcacheremove \n\
groupcachereadsync groupcacheread mwriteplain mrestart groupsocketwrite groupsocketswrite \n\
xpropread xpropwrite groupcachelastupdates busmonitor3 vbusmonitor3 eibread-cgi eibwrite-cgi \n\
vbusmonitor1time\n");
	  return 0;
    }

  /* Buffering stdout is almost never what we want */
  setvbuf(stdout, NULL, _IOLBF, 0);

  if (strcmp (prog, "on") == 0)
    {
      if (ac != 3)
	die ("usage: %s url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);
      buf[0] = 0;
      buf[1] = 0x81;

      if (EIBOpenT_Group (con, dest, 1) == -1)
	die ("Connect failed");

      len = EIBSendAPDU (con, 2, buf);
      if (len == -1)
	die ("Request failed");
      printf ("Send request\n");
    }
  else if (strcmp (prog, "off") == 0)
    {
      if (ac != 3)
	die ("usage: %s url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);
      buf[0] = 0;
      buf[1] = 0x80;

      if (EIBOpenT_Group (con, dest, 1) == -1)
	die ("Connect failed");

      len = EIBSendAPDU (con, 2, buf);
      if (len == -1)
	die ("Request failed");
      printf ("Send request\n");
    }
  else if (strcmp (prog, "write") == 0)
    {
      if (ac != 4)
	die ("usage: %s url eibaddr val", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);
      buf[0] = 0;
      buf[1] = 0x80;
      buf[2] = readHex (ag[3]);

      if (EIBOpenT_Group (con, dest, 1) == -1)
	die ("Connect failed");

      len = EIBSendAPDU (con, 3, buf);
      if (len == -1)
	die ("Request failed");
      printf ("Send request\n");
    }
  else if (strcmp (prog, "swrite") == 0)
    {
      if (ac != 4)
	die ("usage: %s url eibaddr val", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);
      buf[0] = 0;
      buf[1] = 0x80 | (readHex (ag[3]) & 0x3f);

      if (EIBOpenT_Group (con, dest, 1) == -1)
	die ("Connect failed");

      len = EIBSendAPDU (con, 2, buf);
      if (len == -1)
	die ("Request failed");
      printf ("Send request\n");
    }
  else if (strcmp (prog, "read") == 0)
    {

      if (ac != 3)
	die ("usage: %s url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);

      if (EIBOpenT_Group (con, dest, 0) == -1)
	die ("Connect failed");

      buf[0] = 0;
      buf[1] = 0;
      len = EIBSendAPDU (con, 2, buf);
      if (len == -1)
	die ("Request failed");

      while (1)
	{
	  len = EIBGetAPDU_Src (con, sizeof (buf), buf, &src);
	  if (len == -1)
	    die ("Read failed");
	  if (len < 2)
	    die ("Invalid Packet");
	  if (buf[0] & 0x3 || (buf[1] & 0xC0) == 0xC0)
	    {
	      printf ("Error: Unknown APDU: ");
	      printHex (len, buf);
	      printf ("\n");
	    }
	  else if ((buf[1] & 0xC0) == 0x40)
	    {
	      if (len == 2)
		printf ("%02X", buf[1] & 0x3F);
	      else
		printHex (len - 2, buf + 2);
	      printf ("\n");
	      fflush (stdout);
	      break;
	    }
	}

    }
  else if (strcmp (prog, "if") == 0)
    {
      if (ac != 4 && ac != 5)
	die ("usage: %s url eibaddr text1 [text2]", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);

      if (EIBOpenT_Group (con, dest, 0) == -1)
	die ("Connect failed");

      buf[0] = 0;
      buf[1] = 0;
      len = EIBSendAPDU (con, 2, buf);
      if (len == -1)
	die ("Request failed");

      while (1)
	{
	  len = EIBGetAPDU_Src (con, sizeof (buf), buf, &src);
	  if (len < 2 || buf[0] & 0x3 || (buf[1] & 0xC0) == 0xC0)
	    {
	      printf ("ERR");
	      break;
	    }
	  else if ((buf[1] & 0xC0) == 0x40)
	    {
	      if (len == 2)
		{
		  if (buf[1] & 0x3F)
		    printf ("%s", ag[3]);
		  else if (ac == 5)
		    printf ("%s", ag[4]);
		  printf ("\n");
		  fflush (stdout);
		}
	      else
		printf ("ERR\n");
	      break;
	    }
	}

    }
  else if (strcmp (prog, "readtemp") == 0)
    {
      if (ac != 3)
	die ("usage: %s url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);

      if (EIBOpenT_Group (con, dest, 0) == -1)
	die ("Connect failed");

      buf[0] = 0;
      buf[1] = 0;
      len = EIBSendAPDU (con, 2, buf);
      if (len == -1)
	die ("Request failed");

      while (1)
	{
	  len = EIBGetAPDU_Src (con, sizeof (buf), buf, &src);
	  if (len == -1)
	    die ("Read failed");
	  if (len < 2)
	    die ("Invalid Packet");
	  if (buf[0] & 0x3 || (buf[1] & 0xC0) == 0xC0)
	    {
	      printf ("Error: Unknown APDU: ");
	      printHex (len, buf);
	      printf ("\n");
	    }
	  else if ((buf[1] & 0xC0) == 0x40)
	    {
	      if (len == 2)
		printf ("%02X", buf[1] & 0x3F);
	      else
		{

		  if (len == 4)
		    {
		      int d1 =
			((unsigned char) buf[2]) * 256 +
			(unsigned char) buf[3];
		      int m = d1 & 0x7ff;
		      int ex = (d1 & 0x7800) >> 11;
		      float temp = ((float) m * (1 << ex) / 100);
		      // printf ("d1=%d;m=%d;ex=%d;temp=%f\n", d1, m, ex, temp);
		      printf ("%2.1f", temp);
		    }
		  else
		    printHex (len - 2, buf + 2);
		}
	      printf ("\n");
	      break;
	    }
	}
    }
  else if (strcmp (prog, "dimup") == 0)
    {
      if (ac != 4)
	die ("usage: %s url eibaddr time", prog);
      con = open_con(ag[1]);

      dest = readgaddr (ag[2]);

      if (EIBOpenT_Group (con, dest, 0) == -1)
	die ("Connect failed");

      buf[0] = 0;
      buf[1] = 0x80;
      int time = atoi(ag[3]);
      if (time <= 0)
	die ("Invalid param: time");

      unsigned long step = time * 8333;
      int idx;
      int fd = EIB_Poll_FD (con);
      struct timeval tv;
      fd_set fds;
      tv.tv_sec = 0;
      tv.tv_usec = 0;
      for (idx = 0; idx < 120; idx++)
	{
	  buf[2] = idx * 2;
	  len = EIBSendAPDU (con, 3, buf);
	  if (len == -1)
	    die ("Request failed");
	  usleep (step);
	  FD_ZERO (&fds);
	  FD_SET (fd, &fds);
	  while (select (fd + 1, &fds, NULL, NULL, &tv) > 0)
	    {
	      if (EIB_Poll_Complete (con) == 1)
		{
		  len = EIBGetAPDU_Src (con, sizeof (buf), buf, &src);
		  if (len == 3 && (buf[1] & 0xC0) == 0x80)
		    {
		      printf ("Read value %02X\n", buf[2]);
		      if (buf[2] < idx * 2)
			{
			  printf ("Abort dim\n");
			  goto out;
			}
		    }

		}

	    }
	}
      printf ("Dimmed up\n");
    }
  else if (strcmp (prog, "log") == 0)
    {
      int prev_day = -1;
      FILE *log_fd = stderr;

      if (ac != 2)
	die ("usage: %s url", prog);
      con = open_con(ag[1]);

      if (EIBOpenVBusmonitorText (con) == -1)
	die ("Open Busmonitor failed");

      while (1)
	{
	  time_t clock;
	  struct tm *loctim;
	  len = EIBGetBusmonitorPacket (con, sizeof (buf), buf);
	  if (len == -1)
	    die ("Read failed");
	  clock = time (NULL);
	  loctim = localtime (&clock);

	  if (prev_day != loctim->tm_yday)
	    {
	      char logfile[32];
	      FILE *new_fd;
	      prev_day = loctim->tm_yday;
	      snprintf (logfile, 31, "knx-%d-%d-%d.log",
			loctim->tm_year + 1900,
			loctim->tm_mon + 1, loctim->tm_mday);

	      if (NULL == (new_fd = fopen (logfile, "a")))
		{
		  fprintf (log_fd, "Unable to create logfile: %s\n", logfile);
		}
	      else
		{
		  if (log_fd != stderr)
		    {
		      fprintf (log_fd, "Logfile closed\n");
		      fclose (log_fd);
		    }
		  log_fd = new_fd;
		  fprintf (log_fd, "Logfile opened\n");
		}
	    }
	  fprintf (log_fd, "%d-%02d-%02d %d:%d:%d : %s\n",
		   loctim->tm_year + 1900,
		   loctim->tm_mon + 1,
		   loctim->tm_mday,
		   loctim->tm_hour, loctim->tm_min, loctim->tm_sec, buf);
	  fflush (log_fd);
	}
      if (log_fd != stderr)
	{
	  fprintf (log_fd, "Logfile closed\n");
	  fclose (log_fd);
	}
    }
  else if (strcmp (prog, "busmonitor1") == 0)
    {
      if (ac != 2)
	die ("usage: %s url", prog);
      con = open_con(ag[1]);

      if (EIBOpenBusmonitorText (con) == -1)
	die ("Open Busmonitor failed");
      while (1)
	{
	  len = EIBGetBusmonitorPacket (con, sizeof (buf), buf);
	  if (len == -1)
	    die ("Read failed");
	  buf[len] = 0;
	  printf ("%s\n", buf);
	  fflush (stdout);
	}
    }
  else if (strcmp (prog, "busmonitor2") == 0)
    {
      if (ac != 2)
	die ("usage: %s url", prog);
      con = open_con(ag[1]);

      if (EIBOpenBusmonitor (con) == -1)
	die ("Open Busmonitor failed");
      while (1)
	{
	  len = EIBGetBusmonitorPacket (con, sizeof (buf), buf);
	  if (len == -1)
	    die ("Read failed");
	  printHex (len, buf);
	  printf ("\n");
	  fflush (stdout);
	}
    }
  else if (strcmp (prog, "busmonitor3") == 0)
    {
      uint32_t ts;
      uint8_t status;
      if (ac != 2)
	die ("usage: %s url", prog);
      con = open_con(ag[1]);

      if (EIBOpenBusmonitorTS (con, &ts) == -1)
	die ("Open Busmonitor failed");

      printf ("TS-Base: %08x\n", ts);

      while (1)
	{
	  len = EIBGetBusmonitorPacketTS (con, &status, &ts, sizeof (buf), buf);
	  if (len == -1)
	    die ("Read failed");
	  printf ("(%d, %08x) ", status, ts);
	  printHex (len, buf);
	  printf ("\n");
	  fflush (stdout);
	}
    }

  else if (strcmp (prog, "groupcacheclear") == 0)
    {
      if (ac != 2)
	die ("usage: %s url", prog);
      con = open_con(ag[1]);

      len = EIB_Cache_Clear (con);
      if (len == -1)
	die ("Clear failed");
    }
  else if (strcmp (prog, "groupcachedisable") == 0)
    {
      if (ac != 2)
	die ("usage: %s url", prog);
      con = open_con(ag[1]);

      len = EIB_Cache_Disable (con);
      if (len == -1)
	die ("Disable failed");
    }
  else if (strcmp (prog, "groupcacheenable") == 0)
    {
      if (ac != 2)
	die ("usage: %s url", prog);
      con = open_con(ag[1]);
      len = EIB_Cache_Enable (con);
      if (len == -1)
	die ("Enable failed");

    }
  else if (strcmp (prog, "groupcachelastupdates") == 0)
    {
      int i;
      int start;
      int timeout;
      uint32_t end;

      if (ac != 4)
	die ("usage: %s url start-position timeout", prog);
      con = open_con(ag[1]);
      start = atoi (ag[2]);
      timeout = atoi (ag[3]);

      len = EIB_Cache_LastUpdates2 (con, start, timeout, sizeof (buf), buf, &end);
      if (len == -1)
	die ("Read failed");

      printf ("new position: %d\n", end);
      for (i = 0; i < len; i += 2)
	{
	  eibaddr_t a = (buf[i] << 8) | buf[i + 1];
	  printGroup (a);
	  printf ("\n");
	}
      printf ("\n");
    }
  else if (strcmp (prog, "groupcacheread") == 0)
    {
      if (ac != 3)
	die ("usage: %s url groupaddr", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);

      len = EIB_Cache_Read (con, dest, &src, sizeof (buf), buf);
      if (len == -1)
	die ("Read failed");

      switch (buf[1] & 0xC0)
	{
	case 0x40:
	  printf ("Response");
	  break;
	case 0x80:
	  printf ("Write");
	  break;
	}
      printf (" from ");
      printIndividual (src);
      if (buf[1] & 0xC0)
	{
	  printf (": ");
	  if (len == 2)
	    printf ("%02X", buf[1] & 0x3F);
	  else
	    printHex (len - 2, buf + 2);
	}
      printf ("\n");
    }
  else if (strcmp (prog, "groupcachereadsync") == 0)
    {
      uint16_t age = 0;

      if (ac != 3 && ac != 4)
	die ("usage: %s url eibaddr [age]", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);
      if (ac == 4)
	age = atoi (ag[3]);

      len = EIB_Cache_Read_Sync (con, dest, &src, sizeof (buf), buf, age);
      if (len == -1)
	die ("Read failed");

      switch (buf[1] & 0xC0)
	{
	case 0x40:
	  printf ("Response");
	  break;
	case 0x80:
	  printf ("Write");
	  break;
	}
      printf (" from ");
      printIndividual (src);
      if (buf[1] & 0xC0)
	{
	  printf (": ");
	  if (len == 2)
	    printf ("%02X", buf[1] & 0x3F);
	  else
	    printHex (len - 2, buf + 2);
	}
      printf ("\n");
    }
  else if (strcmp (prog, "groupcacheremove") == 0)
    {
      if (ac != 3)
	die ("usage: %s url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);

      len = EIB_Cache_Remove (con, dest);
      if (len == -1)
	die ("Remove failed");
    }
  else if (strcmp (prog, "grouplisten") == 0)
    {
      if (ac != 3)
	die ("usage: %s url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);

      if (EIBOpenT_Group (con, dest, 0) == -1)
	die ("Connect failed");

      while (1)
	{
	  len = EIBGetAPDU_Src (con, sizeof (buf), buf, &src);
	  if (len == -1)
	    die ("Read failed");
	  if (len < 2)
	    die ("Invalid Packet");
	  if (buf[0] & 0x3 || (buf[1] & 0xC0) == 0xC0)
	    {
	      printf ("Unknown APDU from ");
	      printIndividual (src);
	      printf (": ");
	      printHex (len, buf);
	      printf ("\n");
	    }
	  else
	    {
	      switch (buf[1] & 0xC0)
		{
		case 0x00:
		  printf ("Read");
		  break;
		case 0x40:
		  printf ("Response");
		  break;
		case 0x80:
		  printf ("Write");
		  break;
		}
	      printf (" from ");
	      printIndividual (src);
	      if (buf[1] & 0xC0)
		{
		  printf (": ");
		  if (len == 2)
		    printf ("%02X", buf[1] & 0x3F);
		  else
		    printHex (len - 2, buf + 2);
		}
	      printf ("\n");
	    }
	}
    }
  else if (strcmp (prog, "groupread") == 0)
    {
      buf[0] = 0x0;
      buf[1] = 0x0;

      if (ac != 3)
	die ("usage: %s url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);

      if (EIBOpenT_Group (con, dest, 1) == -1)
	die ("Connect failed");

      len = EIBSendAPDU (con, 2, buf);
      if (len == -1)
	die ("Request failed");
      printf ("Send request\n");
    }
  else if (strcmp (prog, "groupreadresponse") == 0)
    {
      uchar req_buf[2] = { 0, 0 };
      fd_set read;
      struct timeval tv;

      if (ac != 3)
	die ("usage: %s url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);

      if (EIBOpenT_Group (con, dest, 0) == -1)
	die ("Connect failed");

      len = EIBSendAPDU (con, 2, req_buf);
      if (len == -1)
	die ("Request failed");
      printf ("Send request\n");

      while (1)
	{
	  tv.tv_usec = 0;
	  tv.tv_sec = 10;
	lp:
	  FD_ZERO (&read);
	  FD_SET (EIB_Poll_FD (con), &read);

	  if (select (EIB_Poll_FD (con) + 1, &read, 0, 0, &tv) == -1)
	    die ("select failed");

	  len = EIB_Poll_Complete (con);
	  if (len == -1)
	    die ("Read failed");
	  if (len == 0)
	    {
	      if (tv.tv_sec == 0 && tv.tv_usec == 0)
		goto end;
	      goto lp;
	    }

	  len = EIBGetAPDU_Src (con, sizeof (buf), buf, &src);
	  if (len == -1)
	    die ("Read failed");
	  if (len < 2)
	    die ("Invalid Packet");
	  if (buf[0] & 0x3 || (buf[1] & 0xC0) == 0xC0)
	    {
	      printf ("Unknown APDU from ");
	      printIndividual (src);
	      printf (": ");
	      printHex (len, buf);
	      printf ("\n");
	    }
	  else
	    {
	      switch (buf[1] & 0xC0)
		{
		case 0x00:
		  printf ("Read");
		  break;
		case 0x40:
		  printf ("Response");
		  break;
		case 0x80:
		  printf ("Write");
		  break;
		}
	      printf (" from ");
	      printIndividual (src);
	      if (buf[1] & 0xC0)
		{
		  printf (": ");
		  if (len == 2)
		    printf ("%02X", buf[1] & 0x3F);
		  else
		    printHex (len - 2, buf + 2);
		}
	      printf ("\n");
	      switch (buf[1] & 0xC0)
		{
		case 0x80:
		case 0x40:
		  goto end;
		}
	    }
	}
    end:
      fprintf (stderr, "Ending %s.\n", prog);
    }
  else if (strcmp (prog, "groupresponse") == 0)
    {
      uchar lbuf[255] = { 0, 0x40 };

      if (ac < 4)
	die ("usage: %s url eibaddr val val ...", prog);
      con = open_con(ag[1]);
      die ("Open failed");
      dest = readgaddr (ag[2]);
      len = readBlock (lbuf + 2, sizeof (lbuf) - 2, ac - 3, ag + 3);
      if (len < 0)
	die ("Invalid hex bytes");

      if (EIBOpenT_Group (con, dest, 1) == -1)
	die ("Connect failed");

      len = EIBSendAPDU (con, 2 + len, lbuf);
      if (len == -1)
	die ("Request failed");
      printf ("Send request\n");
    }
  else if (strcmp (prog, "groupsocketlisten") == 0)
    {
      if (ac != 2)
	die ("usage: %s url", prog);
      con = open_con(ag[1]);

      if (EIBOpen_GroupSocket (con, 0) == -1)
	die ("Connect failed");

      while (1)
	{
	  len = EIBGetGroup_Src (con, sizeof (buf), buf, &src, &dest);
	  if (len == -1)
	    die ("Read failed");
	  if (len < 2)
	    die ("Invalid Packet");
	  if (buf[0] & 0x3 || (buf[1] & 0xC0) == 0xC0)
	    {
	      printf ("Unknown APDU from ");
	      printIndividual (src);
	      printf (" to ");
	      printGroup (dest);
	      printf (": ");
	      printHex (len, buf);
	      printf ("\n");
	    }
	  else
	    {
	      switch (buf[1] & 0xC0)
		{
		case 0x00:
		  printf ("Read");
		  break;
		case 0x40:
		  printf ("Response");
		  break;
		case 0x80:
		  printf ("Write");
		  break;
		}
	      printf (" from ");
	      printIndividual (src);
	      printf (" to ");
	      printGroup (dest);
	      if (buf[1] & 0xC0)
		{
		  printf (": ");
		  if (len == 2)
		    printf ("%02X", buf[1] & 0x3F);
		  else
		    printHex (len - 2, buf + 2);
		}
	      printf ("\n");
	    }
	}
    }
  else if (strcmp (prog, "groupsocketread") == 0)
    {
      buf[0] = 0x0;
      buf[1] = 0x0;

      if (ac != 3)
	die ("usage: %s url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);

      if (EIBOpen_GroupSocket (con, 1) == -1)
	die ("Connect failed");

      len = EIBSendGroup (con, dest, 2, buf);
      if (len == -1)
	die ("Request failed");
      printf ("Send request\n");
    }
  else if (strcmp (prog, "groupsresponse") == 0)
    {
      uchar lbuf[3] = { 0x0, 0x40 };

      if (ac != 4)
	die ("usage: %s url eibaddr val", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);
      lbuf[1] |= readHex (ag[3]) & 0x3f;

      if (EIBOpenT_Group (con, dest, 1) == -1)
	die ("Connect failed");

      len = EIBSendAPDU (con, 2, lbuf);
      if (len == -1)
	die ("Request failed");
      printf ("Send request\n");
    }
  else if (strcmp (prog, "groupswrite") == 0)
    {
      uchar lbuf[3] = { 0x0, 0x80 };

      if (ac != 4)
	die ("usage: %s url eibaddr val", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);
      lbuf[1] |= readHex (ag[3]) & 0x3f;

      if (EIBOpenT_Group (con, dest, 1) == -1)
	die ("Connect failed");

      len = EIBSendAPDU (con, 2, lbuf);
      if (len == -1)
	die ("Request failed");
      printf ("Send request\n");
    }
  else if (strcmp (prog, "groupwrite") == 0)
    {
      uchar lbuf[255] = { 0x0, 0x80 };

      if (ac < 4)
	die ("usage: %s url eibaddr val val ...", prog);
      con = open_con(ag[1]);
      dest = readgaddr (ag[2]);
      len = readBlock (lbuf + 2, sizeof (lbuf) - 2, ac - 3, ag + 3);
      if (len < 0)
	die ("Invalid hex bytes");

      if (EIBOpenT_Group (con, dest, 1) == -1)
	die ("Connect failed");

      len = EIBSendAPDU (con, 2 + len, lbuf);
      if (len == -1)
	die ("Request failed");
      printf ("Send request\n");

    }
  else if (strcmp (prog, "madcread") == 0)
    {
      int channel;
      int16_t val;

      parseKey (&ac, &ag);
      if (ac != 5)
	die ("usage: %s [-k key] url eibaddr channel count", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);
      channel = atoi (ag[3]);
      len = atoi (ag[4]);

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      len = EIB_MC_ReadADC (con, channel, len, &val);
      if (len == -1)
	die ("Read failed");
      printf ("Value: %d\n", val);
    }
  else if (strcmp (prog, "maskver") == 0)
    {
      if (ac != 3)
	die ("usage: %s url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);

      len = EIB_M_GetMaskVersion (con, dest);
      if (len == -1)
	die ("Read failed");
      printf ("Mask: %04X\n", len);
    }
  else if (strcmp (prog, "mmaskver") == 0)
    {
      parseKey (&ac, &ag);
      if (ac != 3)
	die ("usage: %s [-k key] url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      len = EIB_MC_GetMaskVersion (con);
      if (len == -1)
	die ("Read failed");
      printf ("Mask: %04X\n", len);
    }
  else if (strcmp (prog, "mpeitype") == 0)
    {
      parseKey (&ac, &ag);
      if (ac != 3)
	die ("usage: %s [-k key] url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      len = EIB_MC_GetPEIType (con);
      if (len == -1)
	die ("Read failed");
      printf ("PEI: %d\n", len);
    }
  else if (strcmp (prog, "mprogmodeoff") == 0)
    {
      parseKey (&ac, &ag);
      if (ac != 3)
	die ("usage: %s [-k key] url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      len = EIB_MC_Progmode_Off (con);
      if (len == -1)
	die ("Set failed");
    }
  else if (strcmp (prog, "mprogmodeon") == 0)
    {
      parseKey (&ac, &ag);
      if (ac != 3)
	die ("usage: %s [-k key] url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      len = EIB_MC_Progmode_On (con);
      if (len == -1)
	die ("Set failed");
    }
  else if (strcmp (prog, "mprogmodestatus") == 0)
    {
      parseKey (&ac, &ag);
      if (ac != 3)
	die ("usage: %s [-k key] url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      len = EIB_MC_Progmode_Status (con);
      if (len == -1)
	die ("Set failed");
      if (len)
	printf ("in programming mode\n");
      else
	printf ("not in programming mode\n");

    }
  else if (strcmp (prog, "mprogmodetoggle") == 0)
    {
      parseKey (&ac, &ag);
      if (ac != 3)
	die ("usage: %s [-k key] url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      len = EIB_MC_Progmode_Toggle (con);
      if (len == -1)
	die ("Set failed");
    }
  else if (strcmp (prog, "mpropdesc") == 0)
    {
      int obj, prop;
      uchar type, access;
      uint16_t count;

      parseKey (&ac, &ag);
      if (ac != 5)
	die ("usage: %s [-k key] url eibaddr obj prop", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);
      obj = atoi (ag[3]);
      prop = atoi (ag[4]);

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      len = EIB_MC_PropertyDesc (con, obj, prop, &type, &count, &access);
      if (len == -1)
	die ("Read failed");
      printf ("Property: type:%d count:%d access:%02X\n", type, count,
	      access);
    }
  else if (strcmp (prog, "mpropread") == 0)
    {
      int obj, prop, start, nr_of_elem;

      parseKey (&ac, &ag);
      if (ac != 7)
	die ("usage: %s [-k key] url eibaddr obj prop start nr_of_elem",
	     prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);
      obj = atoi (ag[3]);
      prop = atoi (ag[4]);
      start = atoi (ag[5]);
      nr_of_elem = atoi (ag[6]);

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      len =
	EIB_MC_PropertyRead (con, obj, prop, start, nr_of_elem, sizeof (buf),
			     buf);
      if (len == -1)
	die ("Read failed");
      printHex (len, buf);

    }
  else if (strcmp (prog, "mpropscan") == 0)
    {
      int i;

      parseKey (&ac, &ag);
      if (ac != 3)
	die ("usage: %s [-k key] url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      len = EIB_MC_PropertyScan (con, sizeof (buf), buf);
      if (len == -1)
	die ("Read failed");
      for (i = 0; i < len; i += 6)
	if (buf[i + 1] == 1 && buf[i + 2] == 4)
	  printf ("Obj: %d Property: %d Type: %d Objtype:%d Access:%02X\n",
		  buf[i + 0], buf[i + 1], buf[i + 2],
		  (buf[i + 3] << 8) | buf[i + 4], buf[i + 5]);
	else
	  printf ("Obj: %d Property: %d Type: %d Count:%d Access:%02X\n",
		  buf[i + 0], buf[i + 1], buf[i + 2],
		  (buf[i + 3] << 8) | buf[i + 4], buf[i + 5]);

    }
  else if (strcmp (prog, "mpropscanpoll") == 0)
    {
      fd_set read;
      int i;

      parseKey (&ac, &ag);
      if (ac != 3)
	die ("usage: %s [-k key] url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      len = EIB_MC_PropertyScan_async (con, sizeof (buf), buf);
      if (len == -1)
	die ("Read failed");

    lp0:
      FD_ZERO (&read);
      FD_SET (EIB_Poll_FD (con), &read);
      printf ("Waiting\n");
      if (select (EIB_Poll_FD (con) + 1, &read, 0, 0, 0) == -1)
	die ("select failed");
      printf ("Data available\n");
      len = EIB_Poll_Complete (con);
      if (len == -1)
	die ("Read failed");
      if (len == 0)
	goto lp0;
      printf ("Completed\n");

      len = EIBComplete (con);

      for (i = 0; i < len; i += 6)
	if (buf[i + 1] == 1 && buf[i + 2] == 4)
	  printf ("Obj: %d Property: %d Type: %d Objtype:%d Access:%02X\n",
		  buf[i + 0], buf[i + 1], buf[i + 2],
		  (buf[i + 3] << 8) | buf[i + 4], buf[i + 5]);
	else
	  printf ("Obj: %d Property: %d Type: %d Count:%d Access:%02X\n",
		  buf[i + 0], buf[i + 1], buf[i + 2],
		  (buf[i + 3] << 8) | buf[i + 4], buf[i + 5]);

    }
  else if (strcmp (prog, "mpropwrite") == 0)
    {
      int obj, prop, start, nr_of_elem;
      uchar res[255];

      parseKey (&ac, &ag);
      if (ac < 7)
	die
	  ("usage: %s [-k key] url eibaddr obj prop start nr_of_elem [xx xx ..]",
	   prog);
        con = open_con(ag[1]);
      dest = readaddr (ag[2]);
      obj = atoi (ag[3]);
      prop = atoi (ag[4]);
      start = atoi (ag[5]);
      nr_of_elem = atoi (ag[6]);
      len = readBlock (buf, sizeof (buf), ac - 7, ag + 7);
      if (len < 0)
	die ("Invalid hex bytes");

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      printf ("Write: ");
      printHex (len, buf);
      printf ("\n");
      len =
	EIB_MC_PropertyWrite (con, obj, prop, start, nr_of_elem, len, buf,
			      sizeof (res), res);
      if (len == -1)
	die ("Write failed");
      printHex (len, res);
    }
  else if (strcmp (prog, "mread") == 0)
    {
      int addr;

      parseKey (&ac, &ag);
      if (ac != 5)
	die ("usage: %s url [-k key] eibaddr addr count", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);
      addr = readHex (ag[3]);
      len = atoi (ag[4]);

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      len = EIB_MC_Read (con, addr, len, buf);
      if (len == -1)
	die ("Read failed");
      printHex (len, buf);
    }
  else if (strcmp (prog, "mrestart") == 0)
    {
      parseKey (&ac, &ag);
      if (ac != 3)
	die ("usage: %s [-k key] url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      len = EIB_MC_Restart (con);
      if (len == -1)
	die ("Restart failed");

    }
  else if (strcmp (prog, "msetkey") == 0)
    {
      int level, k;
      uint8_t key[4];

      parseKey (&ac, &ag);
      if (ac != 5)
	die ("usage: %s [-k key] url eibaddr level key", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);
      level = atoi (ag[3]);
      sscanf (ag[4], "%x", &k);
      key[0] = (k >> 24) & 0xff;
      key[1] = (k >> 16) & 0xff;
      key[2] = (k >> 8) & 0xff;
      key[3] = (k >> 0) & 0xff;

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      k = EIB_MC_SetKey (con, key, level);
      if (k == -1)
	die ("SetKey failed");

    }
  else if (strcmp (prog, "mwrite") == 0)
    {
      int addr;

      parseKey (&ac, &ag);
      if (ac < 4)
	die ("usage: %s [-k key] url eibaddr addr [xx xx xx ..]", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);
      addr = readHex (ag[3]);
      len = readBlock (buf, sizeof (buf), ac - 4, ag + 4);
      if (len < 0)
	die ("Invalid hex bytes");

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      printf ("Write: ");
      printHex (len, buf);
      printf ("\n");
      len = EIB_MC_Write (con, addr, len, buf);
      if (len == -1)
	die ("Write failed");

    }
  else if (strcmp (prog, "mwriteplain") == 0)
    {
      int addr;

      parseKey (&ac, &ag);
      if (ac < 4)
	die ("usage: %s [-k key] url eibaddr addr [xx xx xx ..]", prog);
      con = open_con(ag[1]);

      dest = readaddr (ag[2]);
      addr = readHex (ag[3]);
      len = readBlock (buf, sizeof (buf), ac - 4, ag + 4);
      if (len < 0)
	die ("Invalid hex bytes");

      if (EIB_MC_Connect (con, dest) == -1)
	die ("Connect failed");
      auth (con);

      printf ("Write: ");
      printHex (len, buf);
      printf ("\n");
      len = EIB_MC_Write_Plain (con, addr, len, buf);
      if (len == -1)
	die ("Write failed");
    }
  else if (strcmp (prog, "progmodeoff") == 0)
    {
      if (ac != 3)
	die ("usage: %s url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);

      len = EIB_M_Progmode_Off (con, dest);
      if (len == -1)
	die ("Set failed");
    }
  else if (strcmp (prog, "progmodeon") == 0)
    {
      if (ac != 3)
	die ("usage: %s url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);

      len = EIB_M_Progmode_On (con, dest);
      if (len == -1)
	die ("Set failed");

    }
  else if (strcmp (prog, "progmodestatus") == 0)
    {
      if (ac != 3)
	die ("usage: %s url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);

      len = EIB_M_Progmode_Status (con, dest);
      if (len == -1)
	die ("Set failed");
      if (len)
	printf ("in programming mode\n");
      else
	printf ("not in programming mode\n");
    }
  else if (strcmp (prog, "progmodetoggle") == 0)
    {
      if (ac != 3)
	die ("usage: %s url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);

      len = EIB_M_Progmode_Toggle (con, dest);
      if (len == -1)
	die ("Set failed");
    }
  else if (strcmp (prog, "readindividual") == 0)
    {
      int i;

      if (ac != 2)
	die ("usage: %s url", prog);
      con = open_con(ag[1]);

      len = EIB_M_ReadIndividualAddresses (con, sizeof (buf), buf);
      if (len == -1)
	die ("Read failed");
      for (i = 0; i < len; i += 2)
	{
	  printf ("Addr: ");
	  printIndividual ((buf[i] << 8) | buf[i + 1]);
	  printf ("\n");
	}
    }
  else if (strcmp (prog, "vbusmonitor1") == 0)
    {
      if (ac != 2)
	die ("usage: %s url", prog);
      con = open_con(ag[1]);

      if (EIBOpenVBusmonitorText (con) == -1)
	die ("Open Busmonitor failed");

      while (1)
	{
	  len = EIBGetBusmonitorPacket (con, sizeof (buf), buf);
	  if (len == -1)
	    die ("Read failed");
	  buf[len] = 0;
	  printf ("%s\n", buf);
	}
    }
  else if (strcmp (prog, "vbusmonitor1poll") == 0)
    {
      fd_set read;

      if (ac != 2)
	die ("usage: %s url", prog);
      con = open_con(ag[1]);

      if (EIBOpenVBusmonitorText (con) == -1)
	die ("Open Busmonitor failed");

      while (1)
	{
	lp1:
	  FD_ZERO (&read);
	  FD_SET (EIB_Poll_FD (con), &read);
	  printf ("Waiting\n");
	  if (select (EIB_Poll_FD (con) + 1, &read, 0, 0, 0) == -1)
	    die ("select failed");
	  printf ("Data available\n");
	  len = EIB_Poll_Complete (con);
	  if (len == -1)
	    die ("Read failed");
	  if (len == 0)
	    goto lp1;
	  printf ("Completed\n");

	  len = EIBGetBusmonitorPacket (con, sizeof (buf), buf);
	  if (len == -1)
	    die ("Read failed");
	  printf ("%s\n", buf);
	}

    }
  else if (strcmp (prog, "vbusmonitor1time") == 0)
    {
      // hires-time
      struct timeval tv; 
      struct tm* ptm; 
      char time_string[40]; 
      long milliseconds; 
      /* Obtain the time of day, and convert it to a tm struct. */ 
      gettimeofday (&tv, NULL); 
      ptm = localtime (&tv.tv_sec); 
      /* Format the date and time, down to a single second. */ 
      strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm); 
      /* Compute milliseconds from microseconds. */ 
      milliseconds = tv.tv_usec / 1000; 
      /* Print the formatted time, in seconds, followed by a decimal point and the milliseconds. */ 
      printf ("%s.%03ld\n", time_string, milliseconds); 

      if (ac != 2)
	die ("usage: %s url", prog);
      con = open_con(ag[1]);

      if (EIBOpenVBusmonitorText (con) == -1)
	die ("Open Busmonitor failed");

      while (1)
	{
	  len = EIBGetBusmonitorPacket (con, sizeof (buf), buf);
	  if (len == -1)
	    die ("Read failed");
	  buf[len] = 0;
		      gettimeofday (&tv, NULL); 
		      ptm = localtime (&tv.tv_sec); 
		      strftime (time_string, sizeof (time_string), "%H:%M:%S", ptm); 
		      milliseconds = tv.tv_usec / 1000; 
	  printf ("%s.%03ld %s\n", time_string, milliseconds, buf);
	  fflush (stdout);
	}

    }
  else if (strcmp (prog, "vbusmonitor2") == 0)
    {
      if (ac != 2)
	die ("usage: %s url", prog);
      con = open_con(ag[1]);

      if (EIBOpenVBusmonitor (con) == -1)
	die ("Open Busmonitor failed");

      while (1)
	{
	  len = EIBGetBusmonitorPacket (con, sizeof (buf), buf);
	  if (len == -1)
	    die ("Read failed");
	  printHex (len, buf);
	  printf ("\n");
	}
    }
  else if (strcmp (prog, "vbusmonitor3") == 0)
    {
      uint32_t ts;
      uint8_t status;
      if (ac != 2)
	die ("usage: %s url", prog);
      con = open_con(ag[1]);

      if (EIBOpenVBusmonitorTS (con, &ts) == -1)
	die ("Open Busmonitor failed");

      printf ("TS-Base: %08x\n", ts);

      while (1)
	{
	  len = EIBGetBusmonitorPacketTS (con, &status, &ts, sizeof (buf), buf);
	  if (len == -1)
	    die ("Read failed");
	  printf ("(%d, %08x) ", status, ts);
	  printHex (len, buf);
	  printf ("\n");
	  fflush (stdout);
	}

    }
  else if (strcmp (prog, "writeaddress") == 0)
    {
      if (ac != 3)
	die ("usage: %s url eibaddr", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);

      len = EIB_M_WriteIndividualAddress (con, dest);
      if (len == -1)
	die ("Set failed");

    }
  else if (strcmp (prog, "xpropread") == 0)
    {
      int len, obj, prop, start, nr_of_elem;

      if (ac != 7)
	die ("usage: %s url eibaddr obj prop start nr_of_elem", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);
      obj = atoi (ag[3]);
      prop = atoi (ag[4]);
      start = atoi (ag[5]);
      nr_of_elem = atoi (ag[6]);

      if (EIB_MC_Individual_Open (con, dest) == -1)
	die ("Connect failed");

      len =
	EIB_MC_PropertyRead (con, obj, prop, start, nr_of_elem, sizeof (buf),
			    buf);
      if (len == -1)
	die ("Read failed");
      printHex (len, buf);
    }
  else if (strcmp (prog, "xpropwrite") == 0)
    {
      int len, obj, prop, start, nr_of_elem;
      uchar res[255];

      if (ac < 7)
	die ("usage: %s url eibaddr obj prop start nr_of_elem [xx xx ..]", prog);
      con = open_con(ag[1]);
      dest = readaddr (ag[2]);
      obj = atoi (ag[3]);
      prop = atoi (ag[4]);
      start = atoi (ag[5]);
      nr_of_elem = atoi (ag[6]);
      len = readBlock (buf, sizeof (buf), ac - 7, ag + 7);
      if (len < 0)
	die ("Invalid hex bytes");

      if (EIB_MC_Individual_Open (con, dest) == -1)
	die ("Connect failed");

      printf ("Write: ");
      printHex (len, buf);
      printf ("\n");
      len =
	EIB_MC_PropertyWrite (con, obj, prop, start, nr_of_elem, len, buf,
			      sizeof (res), res);
      if (len == -1)
	die ("Write failed");
      printHex (len, res);
    }
  else
    {
      char *path = malloc (strlen (PKGLIBDIR) + strlen (prog) + 2);
      sprintf (path,"%s/%s", PKGLIBDIR, prog);
      execv (path, ag);
      die ("No such applet: %s", prog);
    }

out:
  EIBClose (con);
  return 0;
}
