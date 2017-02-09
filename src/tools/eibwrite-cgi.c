/*
    cgi for webserver to write from comet/ajax client

    Parameters:
    url= (ip:host:port or local:/run/knx - local is default)
    t=timeout (10000)
    g=GA - groupadress as x/y/z or integer
    v=cleartext value
    d=DPT (number)

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

#include <string.h>
#define MAX_POSTSIZE 4096 /* max post-size */
#define MAX_GA 1024 /* max number of GAs */

char *eiburl[256];
char data[50];
int gadest,dpt=0;

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

/* convert 2xHex to char */
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
/* FIXME: better handling non-hex chars ? */

/* conv urlized Hex (%xx) to ASCII */
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
/* FIXME: Buffer overflow */
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

// read parameters
void readParseCGI()
{
  char* params;
  char* value;
  char *valuepairs[MAX_GA];
  char *cgistr;
  int i=0,j=0;
  cgistr = parseRequest();
  if(cgistr == NULL)
    cgidie ("No data");
  hex2ascii(cgistr);
  params=strtok(cgistr,"&");
  while( params != NULL && i < MAX_GA)
  {
      valuepairs[i] = (char *)malloc(strlen(params)+1);
      if(valuepairs[i] == NULL)
         return;
      valuepairs[i] = params;
      params=strtok(NULL,"&");
      i++;
  }
  while (i > j)
  {
      value=strtok(valuepairs[j],"=");
      if ( value != NULL )
      {
          if (strcmp (value,"url") == 0)
          {
            value=strtok(NULL,"=");
#ifdef BETA
            if ( value != NULL )
                 *eiburl = (char *) strdup(value);
/* FIXME: Security - define url from ENV - Parameter only for devel */
#endif
          }
          else if (strcmp (value,"a") == 0)
          {
            value=strtok(NULL,"=");
            if (value==NULL)
        	break;
            if (isNumeric(value))
              gadest=atoi(value);
            else
              gadest=readgaddr(value);
          }
          else if (strcmp (value,"d") == 0)
          {
            value=strtok(NULL,"=");
            if (isNumeric(value))
            {
              dpt=atoi(value);
            }
          }
          else if (strcmp (value,"v") == 0)
          {
            value=strtok(NULL,"=");
            if ( value != NULL && strlen(value)<50 )
              strcpy(data,value);
          }
          else
          {
            //printf ("Unknown param %s\n",value); //debug
          }

      }
      j++;
  }
}


int
main ()
{
  int len=0;
  EIBConnection *con;
  unsigned int i,j;
  double fval;
  int sign=0,exp=0,mant=0;
  eibaddr_t dest;
  uchar buf[255] = { 0, 0x80 };
  char tmpbuf[255];
  printf("Content-Type: text/plain\r\n\r\n");

  readParseCGI();
  if (*eiburl == NULL)
    *eiburl = "local:/run/knx";

  con = EIBSocketURL (*eiburl);
  if (!con)
    cgidie ("Open failed");

  dest=gadest;
  if (EIBOpenT_Group (con, dest, 1) == -1)
    cgidie ("Connect failed");

  if (!gadest || !(strlen(data)>0))
    cgidie ("Need ga(g),value(v)");
  switch (dpt)
  {
    case 0:
      len=1;
      for(i = 0; i < strlen(data)/2; i++)
      {
        tmpbuf[0] = data[i*2];
        tmpbuf[1] = data[(i*2)+1];
        sscanf(tmpbuf, "%x", &j);
        buf[i+1] = j;
        len++;
      }
      // only allow A_GroupValue_Write
      if ((buf[1] &0x80) != 0x80)
            cgidie ("Only A_GroupValue_Write allowed");
      break;
    case 1:
      buf[1] |= atoi(data) & 0x3f;
      len=2;
      break;
    case 3:
      // EIS2/DPT3 4bit dim
      buf[1] |= atoi(data) & 0x3f;
      len=2;
      break;
    case 5:
      buf[2] = atoi(data)*255/100;
      len=3;
      break;
    case 51:
    case 5001:
      buf[2] = atoi(data);
      len=3;
      break;
    case 9:
    	fval=atof(data);
      if (fval<0)
        sign = 0x8000;
      mant = (int)(fval * 100.0);
      while (abs(mant) > 2047) {
          mant = mant >> 1;
          exp++;
      }
      i = sign | (exp << 11) | (mant & 0x07ff);
      buf[2] = i >> 8;
      buf[3] = i & 0xff;
      //return $data >> 8, $data & 0xff;
      len=4;
      break;
    case 16:
      len=2;
      for (i=0;i<strlen(data); i++)
      {
        buf[i+2] = (int)(data[i]);
        len++;
      }
  }

  len = EIBSendAPDU (con, len, buf);
  if (len == -1)
    cgidie ("Request failed");
  printf ("{'success':%d}\n",len-1); //don't confuse client with leading 0x00

  //printf("size %d %d\n",sizeof(buf),strlen(buf));
  //printf("buf 0x%02X 0x%02X 0x%02X 0x%02X v:%s l:%d\n" ,buf[1],buf[2],buf[3],buf[4],data,strlen(data));
  EIBClose (con);
  return 0;
}
