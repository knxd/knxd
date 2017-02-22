/*
    cgi for webserver to read groupcachelastupdates from comet/ajax client

    Parameters:
    url= (ip:host:port or local:/run/knx - local is default)
    p=position
    t=timeout (10000)
    g=GA&g=GA... - groupadresses as x/y/z or integer

    Copyright (C) 2010 Michael Markstaller <mm@elabnet.de>
    Copyright (C) 2005-2010 Martin Koegler <mkoegler@auto.tuwien.ac.at>

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
#include <ctype.h>
#include <time.h>

#include <string.h>
#define MAX_POSTSIZE 10000 /* max post-size */
#define BUFSIZE 512 /* max buffer of cfgfile-line */
#define UINT16 65536

char *eiburl[256];
uint32_t lastpos = 0;
int timeout = 300;
int subscribedGA[UINT16 >>3];
int seenGA[UINT16 >>3];
//int gaDPT[UINT16];

//struct gaconfig_s {
//  int ga;
//  int dpt;
//};



/*
char *decodeDPT(const char *data, const int dpt)
{
    unsigned int ival;
    switch (dpt)
    {
      case 5:
        sscanf(data,"%X",&ival);
        return sprintf("%.1f", ival * 100 / 255);
    }
return;
}
*/

void
cgidie (const char *msg)
{
  printf ("{'error': '%s'}\n", msg);
  exit (1);
}

char *parseRequest ()
{
   unsigned long size;
   char *buffer = NULL;
   char *request = getenv("REQUEST_METHOD");
   char *contentlen;
   char *cgi_str;

   // check METHOD
   if(  NULL == request )
      return NULL;
   else if( strcmp(request, "GET") == 0 )
      {
         cgi_str = getenv("QUERY_STRING");
         if( NULL == cgi_str )
            return NULL;
         else
            {
               buffer =(char *) strdup(cgi_str);
               return buffer;
            }
      }
   else if( strcmp(request, "POST") == 0 )
      {
         contentlen = getenv("CONTENT_LENGTH");
         if( NULL == contentlen )
            return NULL;
         else
            {
               size = (unsigned long) atoi(contentlen);
               if(size <= 0 && size > MAX_POSTSIZE) //avoid insane
                  return NULL;
            }
         buffer =(char *) malloc(size+1);
         if( NULL == buffer )
            return NULL;
         else
            {
               if( NULL == fgets(buffer, size+1, stdin) )
                  {
                     free(buffer);
                     return NULL;
                  }
               else
                  return buffer;
            }
      }
   else /* no GET or POST */
      return NULL;
}

/* 2xHex to char */
char convHtoA(char *hex)
{
   char ascii;
   ascii =
   (hex[0] >= 'A' ? ((hex[0] & 0xdf) - 'A')+10 : (hex[0] - '0'));
   ascii <<= 4;
   ascii +=
   (hex[1] >= 'A' ? ((hex[1] & 0xdf) - 'A')+10 : (hex[1] - '0'));
   return ascii;
}

/* urlized Hex (%xx) to ASCII */
void hex2ascii(char *str)
{
   int x,y;
   for(x=0,y=0; str[y] != '\0'; ++x,++y)
      {
         str[x] = str[y];
         if(str[x] == '%')
            {
               str[x] = convHtoA(&str[y+1]);
               y+=2;
            }
         else if( str[x] == '+')
            str[x]=' ';
      }
   str[x] = '\0';
}

int isNumeric(char *str)
{
  while(*str)
  {
    if(!isdigit(*str))
      return 0;
    str++;
  }
  return 1;
}

#ifdef BETA
void readConfig()
{
  char fname[] = "/etc/wiregate/eibga.conf";
  FILE* fp;       /*Declare file pointer variable*/
  char buf[BUFSIZE], *tok;
  //char *buf, *tok;
  int hg,mg,ga;
  int currentga=0,ptr;
  if ((fp = fopen(fname,"r")) == NULL)
  {
    printf("Error openening cfg\n");
    return;
  }
  while(fgets(buf, BUFSIZE, fp) != NULL)
  {
    for(tok = strtok(buf,"\n");tok;tok=strtok(0,"\n"))
    {
      if (sscanf(tok,"[%d/%d/%d]",&hg,&mg,&ga) == 3)
      {
        currentga=((hg & 0x01f) << 11) | ((mg & 0x07) << 8) | ((ga & 0xff));
      }
//      else if (currentga && (ptr=strstr(tok,"DPTId")) )
//      {
//          sscanf(tok,"%*s = %d",&gaDPT[currentga]);
//      }
    }
  }/*until EOF*/
  fclose(fp);
}
#endif


// read parameters
void readParseCGI()
{
  char *param,*nextp;
  char *value;
  char *cgistr;
  cgistr = parseRequest();
  if(cgistr == NULL)
    cgidie ("No data");
  hex2ascii(cgistr);
  nextp = cgistr;
  while (nextp != NULL) 
  {
      param = nextp;
      nextp = strchr(param, '&');
      if (nextp)
        *nextp++ = 0;

      value=strchr(param,'=');
      if (!value)
        continue;
      *value++ = 0;

      if (strcmp (param,"url") == 0)
      {
#ifdef BETA
        if ( value != NULL )
              *eiburl = (char *) strdup(value);
/* FIXME: Security - define url from ENV - Parameter only for devel */
#endif
      }
      else if (strcmp (param,"i") == 0)
      {
        lastpos=atoi(value);
      }
      else if (strcmp (param,"t") == 0)
      {
        timeout=atoi(value);
        if (timeout==0) {
          lastpos=0;
          timeout=1;
        }
      }
      else if (strcmp (param,"a") == 0)
      {
        if (value==NULL)
            break;
        if (isNumeric(value))
        {
          int v = atoi(value);
          subscribedGA[v>>3] |= 1<<(v&7);
        }
        else if (strcmp (value,"all") == 0)
        {
          subscribedGA[0] |= 1;
        } else {
          int ga = readgaddr(value);
          if (ga > 0)
            subscribedGA[ga >> 3] |= 1<<(ga&7);
        }
      }
      //FIXME: Session,filter?
      else
      {
        //printf ("Unknown param %s\n",value); //debug
      }
  }
}


int
main ()
{
  int len,len_gread;
  EIBConnection *con;
  int i,j;
  eibaddr_t dest;
  eibaddr_t src;
  uchar buf[300];
  uchar buf_gread[200];
  char outbuf[10000];
  char *outptr = outbuf;
#define OPL ({ int opl = sizeof(outbuf)-(outptr-outbuf); if (opl < 3) opl=0; opl;})
  time_t tstart;
  char seen = 0;
  tstart = time(NULL);
  //strcat(outbuf,"Content-Type: application/json\n\n");
  //printf("Content-Type: application/json\n\n");
  printf("Content-Type: text/plain\r\n\r\n"); //workaround for uhttpd

  readParseCGI();
  //readConfig();
  if (*eiburl == NULL)
    *eiburl = "local:/run/knx";

  con = EIBSocketURL (*eiburl);
  if (!con)
    cgidie ("Open failed");

  strcpy(outptr,"{\"d\": {");
  outptr += strlen(outptr);

  if (lastpos==0 ) //initial read
  {
    for (i = 1; i < UINT16; i++) // skip all-zero GA
    {
      if (subscribedGA[i>>3]&(1<<(i&7)))
      {
        dest = i;
        len_gread = EIB_Cache_Read_Sync (con, dest, &src, sizeof (buf_gread), buf_gread, 0);
        //printf("%d/%d/%d",(dest >> 11) & 0x1f, (dest >> 8) & 0x07, dest & 0xff); //debug
        //printf(" %d len %d %c",dest,len_gread,buf_gread[1]); //debug
        if (len_gread >= 0)
        {
          if (buf_gread[1] & 0xC0)
          {
            if (seen)
              *outptr++ = ',';
            else
              seen = 1;

            if (len_gread == 2)
            {
              outptr += snprintf (outptr,OPL, "\"%d/%d/%d\":\"%02X", (dest >> 11) & 0x1f, (dest >> 8) & 0x07, dest & 0xff, buf_gread[1] & 0x3F);
            }
            else
            {
              outptr += snprintf(outptr,OPL, "\"%d/%d/%d\":\"", (dest >> 11) & 0x1f, (dest >> 8) & 0x07, dest & 0xff);
              for (j = 0; j < len_gread-2; j++)
              {
                outptr += snprintf (outptr,OPL, "%02X", buf_gread[j+2]);
              }
            }
            *outptr++ = '"';
          }
          seenGA[i>>3] |= 1<<(i&7);
        } else {
          //printf ("read failed!\n");
        }
      }
    }
  }

  memset(seenGA,0,sizeof(seenGA));

  while ((!seen || lastpos <1) && difftime(time(NULL), tstart) < timeout) {
    len = EIB_Cache_LastUpdates2 (con, lastpos, timeout, sizeof (buf), buf, &lastpos);
    if (len == -1)
      cgidie ("Read failed");

    for (i = 0; i < len; i += 2)
      {
        dest = (buf[i] << 8) | buf[i + 1];
        // and output only one if changed multiple times to save the planet
        if (seenGA[dest>>3]&(1<<((dest&7))))
          continue;
        seenGA[dest>>3] |= 1<<((dest&7));

        if ((subscribedGA[dest>>3]&(1<<((dest&7)))) || (subscribedGA[0] | 1))
        {
          // read value from cache
          len_gread = EIB_Cache_Read (con, dest, &src, sizeof(buf_gread), buf_gread);
          if (len_gread != -1)
          {
            if (buf_gread[1] & 0xC0)
            {
              if (seen)
                *outptr++ = ',';
              else
                seen=1;

                //sprintf (tmpbuf,"%d,\"%s",dest, decodeDPT(buf_gread[j+2],gaDPT[dest]));
                //sprintf (tmpbuf,"%d,\"YES %02X", dest, buf_gread[1] & 0x3F);
              if (len_gread == 2)
              {
                outptr += snprintf (outptr,OPL, "\"%d/%d/%d\":\"%02X", (dest >> 11) & 0x1f, (dest >> 8) & 0x07, dest & 0xff, buf_gread[1] & 0x3F);
              }
              else
              {
                outptr += snprintf (outptr,OPL, "\"%d/%d/%d\":\"", (dest >> 11) & 0x1f, (dest >> 8) & 0x07, dest & 0xff);
                for (j = 0; j < len_gread-2; j++)
                {
                  outptr += snprintf (outptr,OPL, "%02X", buf_gread[j+2]);
                }
                //printHex (len_gread - 2, buf_gread + 2);
              }
              *outptr++ = '"';
            }
          }
        }
      }
    if (outptr != outbuf)
      printf ("%s},\"i\":%d}\n",outbuf,lastpos);
  }
  EIBClose (con);
  return 0;
}
