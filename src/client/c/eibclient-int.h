/*
    EIBD client library - internals
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    In addition to the permissions in the GNU General Public License, 
    you may link the compiled version of this file into combinations
    with other programs, and distribute those combinations without any 
    restriction coming from the use of this file. (The General Public 
    License restrictions do apply in other respects; for example, they 
    cover modification of the file, and distribution when not linked into 
    a combine executable.)

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef EIBCLIENT_INT_H
#define EIBCLIENT_INT_H

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "eibclient.h"

#include "eibtypes.h"

/** unsigned char */
typedef uint8_t uchar;

/** EIB Connection internal */
struct _EIBConnection
{
  int (*complete) (EIBConnection *);
  /** file descriptor */
  int fd;
  unsigned readlen;
  /** buffer */
  uchar *buf;
  /** buffer size */
  unsigned buflen;
  /** used buffer */
  unsigned size;
  struct
  {
    int sendlen;
    int len;
    uint8_t *buf;
    int16_t *ptr1;
    uint8_t *ptr2;
    uint8_t *ptr3;
    uint16_t *ptr4;
    eibaddr_t *ptr5;
    eibaddr_t *ptr6;
    uint32_t *ptr7;
  } req;
};

/** extracts TYPE code of an eibd packet */
#define EIBTYPE(con) (((con)->buf[0]<<8)|((con)->buf[1]))
/** sets TYPE code for an eibd packet*/
#define EIBSETTYPE(buf,type) do{(buf)[0]=((type)>>8)&0xff;(buf)[1]=(type)&0xff;}while(0)

/** set EIB address */
#define EIBSETADDR(buf,type) do{(buf)[0]=((type)>>8)&0xff;(buf)[1]=(type)&0xff;}while(0)

int _EIB_SendRequest (EIBConnection * con, unsigned int size, uchar * data);
int _EIB_CheckRequest (EIBConnection * con, int block);
int _EIB_GetRequest (EIBConnection * con);

#define EIBC_LICENSE(text)

#define EIBC_GETREQUEST \
	int i; \
	i = _EIB_GetRequest (con); \
	if (i == -1) \
	     return -1;

#define EIBC_RETURNERROR(msg, error) \
	if (EIBTYPE (con) == msg) \
	  { \
	    errno = error; \
	    return -1; \
	  }

#define EIBC_RETURNERROR_UINT16(offset, error) \
	if (!con->buf[offset] && !con->buf[offset+1]) \
	  { \
	    errno = error; \
	    return -1; \
	  }

#define EIBC_RETURNERROR_SIZE(length, error) \
	if (con->size <= length) \
	  { \
	    errno = error; \
	    return -1; \
	  }

#define EIBC_CHECKRESULT(msg, msgsize) \
	if (EIBTYPE (con) != msg || con->size < msgsize) \
	  { \
	    errno = ECONNRESET; \
	    return -1; \
	  }

#define EIBC_RETURN_BUF(offset) \
	i = con->size - offset; \
	if (i > con->req.len) \
	  i = con->req.len; \
	memcpy (con->req.buf, con->buf + offset, i); \
	return i;

#define EIBC_RETURN_OK \
	return 0;

#define EIBC_RETURN_LEN \
	return con->req.sendlen;

#define EIBC_RETURN_UINT8(offset) \
	return con->buf[offset];

#define EIBC_RETURN_UINT16(offset) \
	return (con->buf[offset] << 8) | (con->buf[offset+1]);

#define EIBC_RETURN_PTR1(offset) \
	if (con->req.ptr1) \
	  *con->req.ptr1 = (con->buf[offset] << 8) | (con->buf[offset+1]);

#define EIBC_RETURN_PTR2(offset) \
	if (con->req.ptr2) \
	  *con->req.ptr2 = con->buf[offset];

#define EIBC_RETURN_PTR3(offset) \
	if (con->req.ptr3) \
	  *con->req.ptr3 = con->buf[offset];

#define EIBC_RETURN_PTR4(offset) \
	if (con->req.ptr4) \
	  *con->req.ptr4 = (con->buf[offset] << 8) | (con->buf[offset+1]);

#define EIBC_RETURN_PTR5(offset) \
	if (con->req.ptr5) \
	  *con->req.ptr5 = (con->buf[offset] << 8) | (con->buf[offset+1]);

#define EIBC_RETURN_PTR6(offset) \
	if (con->req.ptr6) \
	  *con->req.ptr6 = (con->buf[offset] << 8) | (con->buf[offset+1]);

#define EIBC_RETURN_PTR7(offset) \
	if (con->req.ptr7) \
	  *con->req.ptr7 = (con->buf[offset] << 24) | (con->buf[offset+1] << 16) | (con->buf[offset+2] << 8) | (con->buf[offset+3]);

#define EIBC_COMPLETE(name, body) \
	static int \
	name ## _complete (EIBConnection * con) \
	{ \
          con->complete = 0; \
	  body \
	}

#define EIBC_INIT_COMPLETE(name) \
	con->complete = name ## _complete; \
	return 0;

#define EIBC_INIT_SEND(length) \
	uchar head[length]; \
	uchar *ibuf = head; \
	unsigned int ilen __attribute__((unused)) = length; \
	int dyn = 0; \
	int i __attribute__((unused)); \
	if (!con) \
	  { \
	    errno = EINVAL; \
	    return -1; \
	  }

#define EIBC_SEND_BUF(name) EIBC_SEND_BUF_LEN (name, 0)

#define EIBC_SEND_BUF_LEN(name, length) \
	if (!name || name ## _len < length) \
	  { \
	    errno = EINVAL; \
	    return -1; \
	  } \
	con->req.sendlen = name ## _len; \
	dyn = 1; \
	ibuf = (uchar *) malloc (ilen + name ## _len); \
	if (!ibuf) \
	  { \
	    errno = ENOMEM; \
	    return -1; \
	  } \
	memcpy (ibuf, head, ilen); \
	memcpy (ibuf + ilen, name, name ## _len); \
	ilen = ilen + name ## _len;

#define EIBC_SEND_LEN(name) (name ## _len)

#define EIBC_SEND(msg) \
	EIBSETTYPE (ibuf, msg); \
	i = _EIB_SendRequest (con, ilen, ibuf); \
	if (dyn) \
	  free (ibuf); \
	if (i == -1) \
	  return -1;

#define EIBC_READ_BUF(buffer) \
	if (!buffer || buffer ## _maxlen < 0) \
	  { \
	    if (dyn) \
	      free (ibuf); \
	    errno = EINVAL; \
	    return -1; \
	  } \
	con->req.buf = buffer; \
	con->req.len = buffer ## _maxlen;

#define EIBC_READ_LEN(buffer) (buffer ## _maxlen)

#define EIBC_PTR1(name) \
	con->req.ptr1 = name;

#define EIBC_PTR2(name) \
	con->req.ptr2 = name;

#define EIBC_PTR3(name) \
	con->req.ptr3 = name;

#define EIBC_PTR4(name) \
	con->req.ptr4 = name;

#define EIBC_PTR5(name) \
	con->req.ptr5 = name;

#define EIBC_PTR6(name) \
	con->req.ptr6 = name;

#define EIBC_PTR7(name) \
	con->req.ptr7 = name;

#define EIBC_SETADDR(name, offset) \
	EIBSETADDR (ibuf + offset, name);

#define EIBC_SETUINT8(value, offset) \
	ibuf[offset] = value;

#define EIBC_UINT8(value, offset) \
	ibuf[offset] = value;

#define EIBC_SETUINT16(value, offset) \
	ibuf[offset] = ((value) >> 8) & 0xff; \
	ibuf[offset + 1] = ((value)) & 0xff;

#define EIBC_SETUINT32(value, offset) \
	ibuf[offset] = ((value) >> 24) & 0xff; \
	ibuf[offset + 1] = ((value) >> 16) & 0xff; \
	ibuf[offset + 2] = ((value) >> 8) & 0xff; \
	ibuf[offset + 3] = ((value)) & 0xff;

#define EIBC_SETLEN(value, offset) \
	ibuf[offset] = ((value) >> 8) & 0xff; \
	ibuf[offset + 1] = ((value)) & 0xff;

#define EIBC_SETBOOL(value, offset) \
	ibuf[offset] = ((value) ? 0xff : 0);

#define EIBC_SETKEY(value, offset) \
	memcpy (ibuf + offset, value, 4);

#define EIBC_ASYNC(name, args, body) \
	int \
	name ##_async (EIBConnection * con KAG ## args) \
	{ \
	  body \
	} \
	 \
	int \
	name (EIBConnection * con KAG ## args) \
	{ \
	  if (name ## _async (con KAL ## args) == -1) \
	    return -1; \
	  return EIBComplete (con); \
	}

#define EIBC_SYNC(name, args, body) \
	int \
	name (EIBConnection * con KAG ## args) \
	{ \
	  body \
	}

#define AGARG_NONE
#define AGARG_BOOL(name, args) int name KAG ## args
#define AGARG_UINT8(name, args) uint8_t name KAG ## args
#define AGARG_UINT8a(name, args) uint8_t name KAG ## args
#define AGARG_UINT8b(name, args) uint8_t name KAG ## args
#define AGARG_UINT16(name, args) uint16_t name KAG ## args
#define AGARG_UINT32(name, args) uint32_t name KAG ## args
#define AGARG_OUTUINT8(name, args) uint8_t *name KAG ## args
#define AGARG_OUTUINT8a(name, args) uint8_t *name KAG ## args
#define AGARG_OUTUINT16(name, args) uint16_t *name KAG ## args
#define AGARG_OUTINT16(name, args) int16_t *name KAG ## args
#define AGARG_OUTUINT32(name, args) uint32_t *name KAG ## args
#define AGARG_ADDR(name, args) eibaddr_t name KAG ## args
#define AGARG_OUTADDR(name, args) eibaddr_t *name KAG ## args
#define AGARG_OUTADDRa(name, args) eibaddr_t *name KAG ## args
#define AGARG_INBUF(name, args) int name##_len, const uint8_t *name KAG ## args
#define AGARG_OUTBUF(name, args) int name##_maxlen, uint8_t *name KAG ## args
#define AGARG_OUTBUF_LEN(name, args) int name##_maxlen, uint8_t *name KAG ## args
#define AGARG_KEY(name, args) uint8_t name[4] KAG ## args

#define ALARG_NONE
#define ALARG_BOOL(name, args) name KAL ## args
#define ALARG_UINT8(name, args) name KAL ## args
#define ALARG_UINT8a(name, args) name KAL ## args
#define ALARG_UINT8b(name, args) name KAL ## args
#define ALARG_UINT16(name, args) name KAL ## args
#define ALARG_UINT32(name, args) name KAL ## args
#define ALARG_OUTUINT8(name, args) name KAL ## args
#define ALARG_OUTUINT8a(name, args) name KAL ## args
#define ALARG_OUTUINT16(name, args) name KAL ## args
#define ALARG_OUTINT16(name, args) name KAL ## args
#define ALARG_OUTUINT32(name, args) name KAL ## args
#define ALARG_ADDR(name, args) name KAL ## args
#define ALARG_OUTADDR(name, args) name KAL ## args
#define ALARG_OUTADDRa(name, args) name KAL ## args
#define ALARG_INBUF(name, args) name##_len, name KAL ## args
#define ALARG_OUTBUF(name, args) name##_maxlen, name KAL ## args
#define ALARG_OUTBUF_LEN(name, args) name##_maxlen, name KAL ## args
#define ALARG_KEY(name, args) name KAL ## args

#include "def/karg.def"

#endif
