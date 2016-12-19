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

#include "loadimage.h"
#include "image.h"

static void
GenAlloc (CArray & req, uint16_t start, uint16_t len, uint8_t access,
	  uint8_t type, bool check)
{
  const uchar zero[10] = { 0 };
  req.set (zero, 10);
  req[0] = 0x03;
  req[2] = (start >> 8) & 0xff;
  req[3] = (start) & 0xff;
  req[4] = (len >> 8) & 0xff;
  req[5] = (len) & 0xff;
  req[6] = access;
  req[7] = type;
  req[8] = check ? 0x80 : 0x00;
}

typedef struct
{
  uint16_t start;
  uint16_t len;
} Segment;

static int
AddSegmentOverlap (Array < Segment > &s, uint16_t start, uint16_t len)
{
  unsigned int i;
  if (!len)
    return 1;
  for (i = 0; i < s.size(); i++)
    {
      if (start >= s[i].start && start < s[i].start + s[i].len)
	return 0;
      if (s[i].start >= start && s[i].start < start + len)
	return 0;
    }
  Segment s1;
  s1.start = start;
  s1.len = len;
  if (len)
    s.push_back (s1);
  return 1;
}

BCU_LOAD_RESULT
PrepareLoadImage (const CArray & im, BCUImage * &img)
{
  Array < Segment > seg;
  img = 0;
  Image *i = Image::fromArray (im);
  if (!i)
    return IMG_UNRECOG_FORMAT;

  if (!i->isValid ())
    {
      delete i;
      return IMG_INVALID_FORMAT;
    }

  STR_BCUType *b = (STR_BCUType *) i->findStream (S_BCUType);
  if (!b)
    {
      delete i;
      return IMG_NO_BCUTYPE;
    }
  STR_Code *c = (STR_Code *) i->findStream (S_Code);
  if (!c)
    {
      delete i;
      return IMG_NO_CODE;
    }
  if (b->bcutype == 0x0012)
    {
      STR_BCU1Size *s = (STR_BCU1Size *) i->findStream (S_BCU1Size);
      if (!s)
	{
	  delete i;
	  return IMG_NO_SIZE;
	}

      if (s->datasize + s->bsssize + s->stacksize > 18)
	{
	  delete i;
	  return IMG_LODATA_OVERFLOW;
	}
      if (s->textsize > 0xfe)
	{
	  delete i;
	  return IMG_TEXT_OVERFLOW;
	}

      if (s->textsize != c->code.size())
	{
	  delete i;
	  return IMG_WRONG_SIZE;
	}

      if (s->textsize < 0x18)
	{
	  delete i;
	  return IMG_NO_ADDRESS;
	}
      if (c->code[8] < 8 || c->code[8] > c->code.size() + 1)
	{
	  delete i;
	  return IMG_WRONG_CHECKLIM;
	}
      img = new BCUImage;
      img->code = c->code;
      img->BCUType = BCUImage::B_bcu1;
      img->addr = (c->code[0x17] << 8) | (c->code[0x18]);

      return IMG_IMAGE_LOADABLE;
    }
  if (b->bcutype == 0x0020 || b->bcutype == 0x0021)
    {
      STR_BCU2Size *s = (STR_BCU2Size *) i->findStream (S_BCU2Size);
      if (!s)
	{
	  delete i;
	  return IMG_NO_SIZE;
	}
      if (s->lo_datasize + s->lo_bsssize > 18)
	{
	  delete i;
	  return IMG_LODATA_OVERFLOW;
	}
      if (s->hi_datasize + s->hi_bsssize > 24)
	{
	  delete i;
	  return IMG_HIDATA_OVERFLOW;
	}
      if (s->textsize > 0x36f)
	{
	  delete i;
	  return IMG_TEXT_OVERFLOW;
	}

      if (s->textsize != c->code.size())
	{
	  delete i;
	  return IMG_WRONG_SIZE;
	}

      if (s->textsize < 0x18)
	{
	  delete i;
	  return IMG_NO_ADDRESS;
	}
      STR_BCU2Start *s1 = (STR_BCU2Start *) i->findStream (S_BCU2Start);
      if (!s1)
	{
	  delete i;
	  return IMG_NO_START;
	}
      if (s1->addrtab_start != 0x116 || s1->addrtab_size < 4)
	{
	  delete i;
	  return IMG_WRONG_ADDRTAB;
	}
      if (s1->addrtab_size > 0xff)
	{
	  delete i;
	  return IMG_ADDRTAB_OVERFLOW;
	}

      AddSegmentOverlap (seg, s1->addrtab_start, s1->addrtab_size);

      if (!AddSegmentOverlap (seg, s1->assoctab_start, s1->assoctab_size))
	{
	  delete i;
	  return IMG_OVERLAP_ASSOCTAB;
	}
      if (s1->assoctab_size > 0xff)
	{
	  delete i;
	  return IMG_ADDRTAB_OVERFLOW;
	}
      if (s1->readonly_end < s1->readonly_start)
	{
	  delete i;
	  return IMG_NEGATIV_TEXT_SIZE;
	}
      if (!AddSegmentOverlap
	  (seg, s1->readonly_start, s1->readonly_end - s1->readonly_start))
	{
	  delete i;
	  return IMG_OVERLAP_TEXT;
	}
      if (s1->param_end < s1->param_start)
	{
	  delete i;
	  return IMG_NEGATIV_TEXT_SIZE;
	}
      if (s1->eeprom_end < s1->eeprom_start)
	{
	  delete i;
	  return IMG_NEGATIV_TEXT_SIZE;
	}
      if (!AddSegmentOverlap
	  (seg, s1->eeprom_start, s1->eeprom_end - s1->eeprom_start))
	{
	  delete i;
	  return IMG_OVERLAP_EEPROM;
	}
      if (!AddSegmentOverlap
	  (seg, s1->param_start, s1->param_end - s1->param_start))
	{
	  delete i;
	  return IMG_OVERLAP_PARAM;
	}
      if (s1->obj_count > 0xff)
	{
	  delete i;
	  return IMG_OBJTAB_OVERFLOW;
	}
      if (s1->param_end > c->code.size() + 0x100)
	{
	  delete i;
	  return IMG_WRONG_LOADCTL;
	}

      STR_BCU2Key *s2 = (STR_BCU2Key *) i->findStream (S_BCU2Key);
      if (s2 && s2->keys.size() != 3)
	{
	  delete i;
	  return IMG_INVALID_KEY;
	}

      img = new BCUImage;
      img->code = c->code;
      img->BCUType =
	(b->bcutype == 0x0020 ? BCUImage::B_bcu20 : BCUImage::B_bcu21);
      img->addr = (c->code[0x17] << 8) | (c->code[0x18]);

      if (s2)
	{
	  img->installkey = s2->installkey;
	  img->keys = s2->keys;
	}
      else
	{
	  img->installkey = 0xFFFFFFFF;
	  img->keys.resize (3);
	  img->keys[0] = 0xFFFFFFFF;
	  img->keys[1] = 0xFFFFFFFF;
	  img->keys[2] = 0xFFFFFFFF;
	}

      const uchar zero[10] = { 0 };
      EIBLoadRequest r;
      /*unload */
      r.obj = 1;
      r.prop = 5;
      r.start = 1;
      r.memaddr = 0xffff;
      r.req.set (zero, 10);
      r.req[0] = 0x04;
      r.result.resize (1);
      r.result[0] = 0x00;
      r.error = IMG_UNLOAD_ADDR;
      img->load.push_back (r);
      r.obj = 2;
      r.error = IMG_UNLOAD_ASSOC;
      img->load.push_back (r);
      r.obj = 3;
      r.error = IMG_UNLOAD_PROG;
      img->load.push_back (r);

      /* loadaddrtab */
      r.req.set (zero, 10);
      r.obj = 1;
      r.req[0] = 0x01;
      r.result[0] = 0x02;
      r.error = IMG_LOAD_ADDR;
      img->load.push_back (r);

      GenAlloc (r.req, s1->addrtab_start, s1->addrtab_size, 0x31, 0x03, 1);
      r.memaddr = s1->addrtab_start;
      r.len = 1;
      r.error = IMG_WRITE_ADDR;
      img->load.push_back (r);

      r.obj = 0xff;
      r.memaddr += 3;
      r.len = s1->addrtab_size - 4;
      img->load.push_back (r);

      r.obj = 1;
      r.memaddr = 0xffff;
      r.req[0] = 0x03;
      r.req[1] = 0x02;
      r.req[2] = (s1->addrtab_start >> 8) & 0xff;
      r.req[3] = (s1->addrtab_start) & 0xff;
      r.req[4] = c->code[0x09];
      r.req.setpart (c->code.data() + 0x03, 5, 5);
      r.error = IMG_SET_ADDR;
      img->load.push_back (r);

      r.req.set (zero, 10);
      r.req[0] = 0x02;
      r.result[0] = 0x01;
      r.error = IMG_FINISH_ADDR;
      img->load.push_back (r);

      /* loadassoctab */
      r.req.set (zero, 10);
      r.obj = 2;
      r.req[0] = 0x01;
      r.result[0] = 0x02;
      r.error = IMG_LOAD_ASSOC;
      img->load.push_back (r);

      GenAlloc (r.req, s1->assoctab_start, s1->assoctab_size, 0x31, 0x03, 1);
      r.memaddr = s1->assoctab_start;
      r.len = s1->assoctab_size - 1;
      r.error = IMG_WRITE_ASSOC;
      img->load.push_back (r);

      r.memaddr = 0xffff;
      r.req[0] = 0x03;
      r.req[1] = 0x02;
      r.req[2] = (s1->assoctab_start >> 8) & 0xff;
      r.req[3] = (s1->assoctab_start) & 0xff;
      r.req[4] = c->code[0x09];
      r.req.setpart (c->code.data() + 0x03, 5, 5);
      r.error = IMG_SET_ASSOC;
      img->load.push_back (r);

      r.req.set (zero, 10);
      r.req[0] = 0x02;
      r.result[0] = 0x01;
      r.error = IMG_FINISH_ASSOC;
      img->load.push_back (r);

      /* loadproctab */
      r.req.set (zero, 10);
      r.obj = 3;
      r.req[0] = 0x01;
      r.result[0] = 0x02;
      r.error = IMG_LOAD_PROG;
      img->load.push_back (r);

      GenAlloc (r.req, 0x00C8, 0x0018, 0x32, 0x01, 0);
      r.error = IMG_ALLOC_LORAM;
      img->load.push_back (r);

      GenAlloc (r.req, 0x0972, 0x004A, 0x32, 0x02, 0);
      r.error = IMG_ALLOC_HIRAM;
      img->load.push_back (r);

      GenAlloc (r.req, 0x0100, 0x0016, 0x32, 0x03, 0);
      r.error = IMG_ALLOC_INIT;
      r.len = 0x001;
      r.memaddr = 0x100;
      img->load.push_back (r);

      r.obj = 0xff;
      r.memaddr = 0x103;
      r.len = 0x13;
      img->load.push_back (r);
      r.obj = 3;

      GenAlloc (r.req, s1->readonly_start,
		s1->readonly_end - s1->readonly_start, 0x30, 0x03, 1);
      r.error = IMG_ALLOC_RO;
      r.len = s1->readonly_end - s1->readonly_start - 1;
      r.memaddr = s1->readonly_start;
      if (r.len)
	img->load.push_back (r);

      GenAlloc (r.req, s1->eeprom_start, s1->eeprom_end - s1->eeprom_start,
		0x31, 0x03, 0);
      r.error = IMG_ALLOC_EEPROM;
      r.len = s1->eeprom_end - s1->eeprom_start;
      r.memaddr = s1->eeprom_start;
      if (r.len)
	img->load.push_back (r);

      GenAlloc (r.req, s1->param_start, s1->param_end - s1->param_start, 0x32,
		0x03, 1);
      r.error = IMG_ALLOC_PARAM;
      r.len = s1->param_end - s1->param_start;
      r.memaddr = s1->param_start;
      if (r.len > 1)
	img->load.push_back (r);

      r.memaddr = 0xffff;
      r.req[0] = 0x03;
      r.req[1] = 0x02;
      r.req[2] = (s1->runaddr >> 8) & 0xff;
      r.req[3] = (s1->runaddr) & 0xff;
      r.req[4] = c->code[0x09];
      r.req.setpart (c->code.data() + 0x03, 5, 5);
      r.error = IMG_SET_PROG;
      img->load.push_back (r);

      r.req.set (zero, 10);
      r.req[0] = 0x03;
      r.req[1] = 0x03;
      r.req[2] = (s1->initaddr >> 8) & 0xff;
      r.req[3] = (s1->initaddr) & 0xff;
      r.req[4] = (s1->saveaddr >> 8) & 0xff;
      r.req[5] = (s1->saveaddr) & 0xff;
      r.req[6] = (s1->sphandler >> 8) & 0xff;
      r.req[7] = (s1->sphandler) & 0xff;
      r.error = IMG_SET_TASK_PTR;
      img->load.push_back (r);

      r.req.set (zero, 10);
      r.req[0] = 0x03;
      r.req[1] = 0x04;
      r.req[2] = (s1->obj_ptr >> 8) & 0xff;
      r.req[3] = (s1->obj_ptr) & 0xff;
      r.req[4] = (s1->obj_count) & 0xff;
      r.error = IMG_SET_OBJ;
      img->load.push_back (r);

      r.req.set (zero, 10);
      r.req[0] = 0x03;
      r.req[1] = 0x05;
      r.req[2] = (s1->appcallback >> 8) & 0xff;
      r.req[3] = (s1->appcallback) & 0xff;
      r.req[4] = (s1->groupobj_ptr >> 8) & 0xff;
      r.req[5] = (s1->groupobj_ptr) & 0xff;
      r.req[6] = (s1->seg0 >> 8) & 0xff;
      r.req[7] = (s1->seg0) & 0xff;
      r.req[8] = (s1->seg1 >> 8) & 0xff;
      r.req[9] = (s1->seg1) & 0xff;
      r.error = IMG_SET_TASK2;
      img->load.push_back (r);

      r.req.set (zero, 10);
      r.req[0] = 0x02;
      r.result[0] = 0x01;
      r.error = IMG_FINISH_PROC;
      img->load.push_back (r);

      return IMG_IMAGE_LOADABLE;


    }
  delete i;
  return IMG_UNKNOWN_BCUTYPE;
}

#define _(A) (A)

String
decodeBCULoadResult (BCU_LOAD_RESULT r)
{
  switch (r)
    {
    case IMG_UNKNOWN_ERROR:
      return _("unknown error");
      break;
    case IMG_UNRECOG_FORMAT:
      return _("data not regcognized as image");
      break;
    case IMG_INVALID_FORMAT:
      return _("invalid streams in the image");
      break;
    case IMG_NO_BCUTYPE:
      return _("no bcu type specified");
      break;
    case IMG_UNKNOWN_BCUTYPE:
      return _("don't know how to load the bcutype");
      break;
    case IMG_NO_CODE:
      return _("no text segment found");
      break;
    case IMG_NO_SIZE:
      return _("size information not found");
      break;
    case IMG_LODATA_OVERFLOW:
      return _("too many data for low-ram");
      break;
    case IMG_HIDATA_OVERFLOW:
      return _("too many data for hi-ram");
      break;
    case IMG_TEXT_OVERFLOW:
      return _("too many data for eeprom");
      break;
    case IMG_IMAGE_LOADABLE:
      return _("Image is loadable");
      break;
    case IMG_NO_ADDRESS:
      return _("no address found in the image");
      break;
    case IMG_WRONG_SIZE:
      return _("unexpected size of the text segment");
      break;
    case IMG_NO_DEVICE_CONNECTION:
      return _("connection to the device failed");
      break;
    case IMG_MASK_READ_FAILED:
      return _("read of mask version failed");
      break;
    case IMG_WRONG_MASK_VERSION:
      return _("incompatible mask version");
      break;
    case IMG_CLEAR_ERROR:
      return _("reseting of RunFlags failed");
      break;
    case IMG_RESET_ADDR_TAB:
      return _("reseting of the address table failed");
      break;
    case IMG_LOAD_HEADER:
      return _("loading of the header failed");
      break;
    case IMG_LOAD_MAIN:
      return _("loading of the code in the eeprom failed");
      break;
    case IMG_ZERO_RAM:
      return _("cleaning the ram failed");
      break;
    case IMG_FINALIZE_ADDR_TAB:
      return _("finalizing the address table failed");
      break;
    case IMG_PREPARE_RUN:
      return _("setting the RunFlags failed");
      break;
    case IMG_RESTART:
      return _("restart failed");
      break;
    case IMG_LOADED:
      return _("image successful loaded");
      break;
    case IMG_NO_START:
      return _("no BCU2 load control information present");
      break;
    case IMG_WRONG_ADDRTAB:
      return _("wrong start address of the address table");
      break;
    case IMG_ADDRTAB_OVERFLOW:
      return _("address table too big");
      break;
    case IMG_OVERLAP_ASSOCTAB:
      return _("association table overlaps with an other segment");
      break;
    case IMG_OVERLAP_TEXT:
      return _("text segement overlaps with an other segment");
      break;
    case IMG_NEGATIV_TEXT_SIZE:
      return _("segment end < text segment");
      break;
    case IMG_OVERLAP_PARAM:
      return _("param segment overlaps with an other segment");
      break;
    case IMG_OVERLAP_EEPROM:
      return _("eeprom segment overlaps with an other segment");
      break;
    case IMG_OBJTAB_OVERFLOW:
      return _("too many objects");
      break;
    case IMG_WRONG_LOADCTL:
      return _("param end not in the text segment");
      break;
    case IMG_UNLOAD_ADDR:
      return _("error unloading address table");
      break;
    case IMG_UNLOAD_ASSOC:
      return _("error unloading assocation table");
      break;
    case IMG_UNLOAD_PROG:
      return _("error unloading user programm");
      break;
    case IMG_LOAD_ADDR:
      return _("error start loading address table");
      break;
    case IMG_WRITE_ADDR:
      return _("error allocation address table");
      break;
    case IMG_SET_ADDR:
      return _("error setting address table start");
      break;
    case IMG_FINISH_ADDR:
      return _("error finishing address table");
      break;
    case IMG_LOAD_ASSOC:
      return _("error start loading association table");
      break;
    case IMG_WRITE_ASSOC:
      return _("error allocation assocation table");
      break;
    case IMG_SET_ASSOC:
      return _("error setting assocation table start");
      break;
    case IMG_FINISH_ASSOC:
      return _("error finishing assocation table");
      break;
    case IMG_LOAD_PROG:
      return _("error start loading programm");
      break;
    case IMG_ALLOC_LORAM:
      return _("error allocation low ram");
      break;
    case IMG_ALLOC_HIRAM:
      return _("error allocation high ram");
      break;
    case IMG_ALLOC_INIT:
      return _("error allocation config section");
      break;
    case IMG_ALLOC_RO:
      return _("error loading text segment");
      break;
    case IMG_ALLOC_EEPROM:
      return _("error loading eeprom segment");
      break;
    case IMG_ALLOC_PARAM:
      return _("error loading parameter segement");
      break;
    case IMG_SET_PROG:
      return _("error setting programm entry");
      break;
    case IMG_SET_TASK_PTR:
      return _("error setting task pointer");
      break;
    case IMG_SET_OBJ:
      return _("error setting object pointer");
      break;
    case IMG_SET_TASK2:
      return _("error setting group object pointer");
      break;
    case IMG_FINISH_PROC:
      return _("error finishing application programm");
      break;
    case IMG_WRONG_CHECKLIM:
      return _("wrong check limit");
      break;
    case IMG_AUTHORIZATION_FAILED:
      return _("authorization failed");
      break;
    case IMG_INVALID_KEY:
      return _("invalid key information");
      break;
    case IMG_KEY_WRITE:
      return _("key write failed");
      break;

    default:
      return _("errorcode not defined");
    }
}
