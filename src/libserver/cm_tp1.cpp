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

#include "cm_tp1.h"

/* L_Data */

LDataPtr CM_TP1_to_L_Data (const CArray & c, TracePtr)
{
  LDataPtr l = LDataPtr(new L_Data_PDU ());
  if (c.size() < 6)
    return nullptr;
  if ((c[0] & 0x53) != 0x10)
    return nullptr;
  l->frame_format = (c[0] & 0x80) ? 1 : 0;
  l->repeated = (c[0] & 0x20) ? 0 : 1;
  l->priority = static_cast<EIB_Priority>((c[0] >> 2) & 0x3);
  l->valid_length = 1;
  if (l->frame_format)
    {
      /* Standard frame */
      l->source_address = (c[1] << 8) | (c[2]);
      l->destination_address = (c[3] << 8) | (c[4]);
      l->address_type = (c[5] & 0x80) ? GroupAddress : IndividualAddress;
      l->hop_count = (c[5] >> 4) & 0x07; // @todo this is NPDU
      uint8_t len = (c[5] & 0x0f) + 1;
      if (len + 7 != c.size())
        return nullptr;
      l->lsdu.set (c.data() + 6, len);
    }
  else
    {
      /* extended frame */
      if ((c[1] & 0x0f) != 0)
        return nullptr;
      if (c.size() < 7)
        return nullptr;
      l->address_type = (c[1] & 0x80) ? GroupAddress : IndividualAddress;
      l->hop_count = (c[1] >> 4) & 0x07; // @todo this is NPDU
      l->source_address = (c[2] << 8) | (c[3]);
      l->destination_address = (c[4] << 8) | (c[5]);
      uint8_t len = c[6] + 1;
      if (len + 8 != c.size())
        {
          if (c.size() == 23)
            {
              l->valid_length = 0;
              l->lsdu.set (c.data() + 7, 8);
            }
          else
            return nullptr;
        }
      else
        l->lsdu.set (c.data() + 7, len);
    }

  /* checksum */
  uint8_t checksum = 0;
  for (unsigned i = 0; i < c.size () - 1; i++)
    checksum ^= c[i];
  checksum = ~checksum;
  l->valid_checksum = (c[c.size () - 1] == checksum);

  return l;
}

CArray L_Data_to_CM_TP1 (const LDataPtr & p)
{
  assert (p->lsdu.size() >= 1);
  assert (p->lsdu.size() <= 0xff);
  assert ((p->hop_count & 0xf8) == 0);

  CArray pdu;
  uint8_t len = p->lsdu.size() - 1;
  if (len <= 0x0f)
    {
      /* L_Data_Standard Frame */
      pdu.resize (7 + p->lsdu.size());
      pdu[0] = 0x90 | (p->repeated ? 0x00 : 0x20) | (p->priority << 2);
      pdu[1] = p->source_address >> 8;
      pdu[2] = p->source_address & 0xff;
      pdu[3] = p->destination_address >> 8;
      pdu[4] = p->destination_address & 0xff;
      pdu[5] =
        (p->address_type == GroupAddress ? 0x80 : 0x00) |
        ((p->hop_count & 0x07) << 4) |
        (len & 0x0f);
      pdu.setpart (p->lsdu.data(), 6, 1 + (len & 0x0f));
    }
  else
    {
      /* L_Data_Extended Frame */
      pdu.resize (8 + p->lsdu.size());
      pdu[0] = 0x10 | (p->repeated ? 0x00 : 0x20) | (p->priority << 2);
      pdu[1] =
        (p->address_type == GroupAddress ? 0x80 : 0x00) |
        ((p->hop_count & 0x07) << 4);
      pdu[2] = p->source_address >> 8;
      pdu[3] = p->source_address & 0xff;
      pdu[4] = p->destination_address >> 8;
      pdu[5] = p->destination_address & 0xff;
      pdu[6] = (p->lsdu.size() - 1) & 0xff;
      pdu.setpart (p->lsdu.data(), 7, 1 + (len & 0xff));
    }

  /* checksum */
  uint8_t checksum = 0;
  for (unsigned i = 0; i < pdu.size() - 1; i++)
    checksum ^= pdu[i];
  pdu[pdu.size() - 1] = ~checksum;

  return pdu;
}
