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

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "tpuart.h"
#include "layer3.h"

TPUART_Base::TPUART_Base (L2options *opt)
	: sendbuf(), recvbuf(), Layer2 (opt)
{
  int reuse = 1;
  int nodelay = 1;
  struct sockaddr_in addr;

  TRACEPRINTF (t, 2, this, "Open");

  ackallgroup = opt ? (opt->flags & FLAG_B_TPUARTS_ACKGROUP) : 0;
  ackallindividual = opt ? (opt->flags & FLAG_B_TPUARTS_ACKINDIVIDUAL) : 0;

  if (opt)
	opt->flags &=~ (FLAG_B_TPUARTS_ACKGROUP |
	                FLAG_B_TPUARTS_ACKINDIVIDUAL);
  // Start(); // XXX needs to be called by derived class
}

void
TPUART_Base::setup_buffers()
{
  sendbuf.init(fd);
  recvbuf.init(fd);
  recvbuf.low_latency();

  recvbuf.on_recv_cb.set<TPUART_Base,&TPUART_Base::read_cb>(this);
  recvbuf.on_error_cb.set<TPUART_Base,&TPUART_Base::error_cb>(this);
  sendbuf.on_error_cb.set<TPUART_Base,&TPUART_Base::error_cb>(this);
  timer.set <TPUART_Base,&TPUART_Base::timer_cb> (this);
  sendtimer.set <TPUART_Base,&TPUART_Base::sendtimer_cb> (this);
  watchdogtimer.set <TPUART_Base,&TPUART_Base::watchdogtimer_cb> (this);

  trigger.set<TPUART_Base,&TPUART_Base::trigger_cb>(this);
  trigger.start();

  sendbuf.start();
  recvbuf.start();
}

void
TPUART_Base::error_cb()
{
  TRACEPRINTF (t, 2, this, "ERROR");
  stop();
}

TPUART_Base::~TPUART_Base ()
{
  TRACEPRINTF (t, 2, this, "Close");

  sendbuf.stop();
  recvbuf.stop();
  timer.stop();
  sendtimer.stop();
  watchdogtimer.stop();

  if (fd != -1)
    close (fd);
}

bool TPUART_Base::init (Layer3 *l3)
{
  if (fd == -1)
    return false;
  if (! addGroupAddress(0))
    return false;
  if (! Layer2::init (l3))
    return false;

  sendbuf.start();
  recvbuf.start();
  return true;
}

void
TPUART_Base::send_L_Data (LDataPtr l)
{
  TRACEPRINTF (t, 2, this, "Send %s", l->Decode ().c_str());
  send_q.push(std::move(l));
  if (!sendtimer.is_active())
    trigger.send();
}

//Open

bool
TPUART_Base::enterBusmonitor ()
{
  if (!Layer2::enterBusmonitor ())
    return false;
  uchar c = 0x05;
  t->TracePacket (2, this, "openBusmonitor", 1, &c);
  if (write (fd, &c, 1) != 1)
    return 0;
  return 1;
}

bool
TPUART_Base::leaveBusmonitor ()
{
  if (!Layer2::leaveBusmonitor ())
    return false;
  TRACEPRINTF (t, 2, this, "leaveBusmonitor");

  send_reset();
  return 1;
}

bool
TPUART_Base::Open ()
{
  if (!Layer2::Open ())
    return false;
  TRACEPRINTF (t, 2, this, "open");
  send_reset();
  return 1;
}

void
TPUART_Base::RecvLPDU (const uchar * data, int len)
{
  t->TracePacket (1, this, "RecvLP", len, data);
  if (mode & BUSMODE_MONITOR)
    {
      LBusmonPtr l = LBusmonPtr(new L_Busmonitor_PDU (shared_from_this()));
      l->pdu.set (data, len);
      l3->recv_L_Busmonitor (std::move(l));
    }
  if (mode == BUSMODE_UP)
    {
      LPDUPtr l = LPDU::fromPacket (CArray (data, len), shared_from_this());
      if (l->getType () != L_Data)
        TRACEPRINTF (t, 1, this, "dropping packet: type %d", l->getType ());
      else
        {
          if (((L_Data_PDU *)(&*l))->valid_checksum)
            l3->recv_L_Data (dynamic_unique_cast<L_Data_PDU>(std::move(l)));
          else
            TRACEPRINTF (t, 1, this, "dropping packet: invalid");
        }
    }
}

size_t
TPUART_Base::read_cb(uint8_t *buf, size_t len)
{
  t->TracePacket (0, this, "Read", len, buf);
  if (watch < 4)
    in.setpart (buf, in.size(), len);
  process_read(false);
  return len;
}

void
TPUART_Base::timer_cb(ev::timer &w, int revents)
{
  process_read(true);
}

void
TPUART_Base::process_read(bool timed_out)
{
  if (watch > 4)
    return;

  while (in.size() > 0)
    {
      if (skip_char)
        {
          in.deletepart (0, 1);
          skip_char = false;
          continue;
        }
      uint8_t c = in[0];

      if (c == 0x8B) // L_DataConfirm positive
        {
          if (sendtimer.is_active())
            {
              sendtimer.stop();
              l.reset();
              retry = 0;
            }
          in.deletepart (0, 1);
        }
      else if (c == 0x0B) // L_DataConfirm negative
        {
          if (sendtimer.is_active())
            {
              retry++;
              sendtimer.stop();
              if (retry > 3)
                {
                  TRACEPRINTF (t, 0, this, "NACK: dropping packet");
                  l.reset();
                  retry = 0;
                }
              else
                {
                  trigger.send();
                  TRACEPRINTF (t, 0, this, "NACK");
                }
            }
          in.deletepart (0, 1);
        }
      else if ((c & 0x07) == 0x07) // state indication
        {
          TRACEPRINTF (t, 0, this, "RecvWatchdog: %02X", c);
          watch = 2;
          watchdogtimer.start(10,0);
          in.deletepart (0, 1);
        }
      /*
        * 0xCC acknowledge frame
        * 0x0C NotAcknowledge frame
        * 0xC0 Busy Frame
        */
      else if (c == 0xCC || c == 0xC0 || c == 0x0C)
        {
          RecvLPDU (in.data(), 1);
          in.deletepart (0, 1);
        }
      else if ((c & 0x50) == 0x10) // Matches KNX control byte L_Data_Standard/Extended Frame
        {
          bool ext = !(c & 0x80);

          if (in.size () < 6+ext)
            goto do_timer;

          unsigned len = ext ? in[6] : (in[5] & 0x0f);
          len += 6 + ext + 2;

          if (!recvecho && sendtimer.is_active())
            {
              CArray recvheader;
              recvheader.set(in.data(),6+ext);
              recvheader[0] &=~ 0x20; // repeat flag
              if (recvheader == sendheader)
                {
                  TRACEPRINTF (t, 0, this, "Ignoring this %d-byte telegram. We sent it.",len-2);
                  recvecho = true;
                }
            }
/*
 * Tell the TPUART whether to ack the packet
 * assuming that the checksum ultimately matches
 * (the chip verifies it itself)
 */
          if (!acked && adr_check && !recvecho)
            {
              uchar c = 0x10;
              if ((in[ext ? 1 : 5] & 0x80) == 0)
                {
                  if (ackallindividual || l3->hasAddress ((in[3+ext] << 8) | in[4+ext], shared_from_this()))
                    c |= 0x1;
                }
              else
                {
                  if (ackallgroup || l3->hasGroupAddress ((in[3+ext] << 8) | in[4+ext], shared_from_this()))
                    c |= 0x1;
                }
              TRACEPRINTF (t, 0, this, "SendAck %02X", c);
              sendbuf.write(&c,1);
              acked = true;
            }
          if (in.size() < len)
            goto do_timer;
          if (!recvecho)
            {
              acked = false;
              RecvLPDU (in.data(), len);
            }
          else
            {
              if (l && !(in[0]&0x20)) // this is a repeat frame
                {
                  CArray d = l->ToPacket ();
                  // ignore repeat flag when comparing
                  if (!((in[0]^d[0])&~0x20) && !memcmp(in.data()+1,d.data()+1,d.size()-2))
                    {
                      TRACEPRINTF (t, 0, this, "Skip retrying");
                      retry = 9; // The TPUART handles retransmitting in hardware. There is no need to do it again.
                    }
                }
              recvecho = false;
            }
          in.deletepart (0, len);
        }
      else
        {
          acked = false;
          TRACEPRINTF (t, 0, this, "unknown %02X", c);
          in.deletepart (0, 1);
        }
      timer.stop();
      continue;

    do_timer:
      if (!timed_out)
        break;
      TRACEPRINTF (t, 0, this, "timeout %02X", c);
      in.deletepart (0, 1);
      continue;
    }
  if (in.size())
    timer.start(0.3,0);
  else if (!sendtimer.is_active())
    trigger.send();
}

void
TPUART_Base::trigger_cb(ev::async &w, int revents)
{
  if (sendtimer.is_active() || in.size() || watch > 4)
    return;
  if (!l && !send_q.isempty ())
    l = send_q.get ();
  if (l)
    {
      CArray d = l->ToPacket ();
      CArray *w = new CArray();
      unsigned i;

      bool ext = !(d[0] & 0x80);
      sendheader.set(d.data(), 6+ext);
      sendheader[0] &=~ 0x20;
      w->resize (d.size() * 2);
      for (i = 0; i < d.size(); i++)
        {
          (*w)[2 * i] = 0x80 | (i & 0x3f);
          (*w)[2 * i + 1] = d[i];
        }
      (*w)[(d.size() * 2) - 2] = ((*w)[(d.size() * 2) - 2] & 0x3f) | 0x40;
      t->TracePacket (0, this, "Write", *w);
      sendbuf.write(w);
      sendtimer.start(0.6,0);
    }
  else if (!watch && mode != BUSMODE_MONITOR && !timer.is_active())
    {
      watchdogtimer.start(10,0);
      watch = 1;
      uchar c = 0x02;
      t->TracePacket (2, this, "Watchdog Status", 1, &c);
      sendbuf.write(&c, 1);
    }
}

void
TPUART_Base::send_reset()
{
  uchar c = 0x01;
  t->TracePacket (2, this, "Watchdog Reset", 1, &c);
  sendbuf.write(&c,1);
  adr_check = true;

  if (l3->findFilter("single"))
    {
      eibaddr_t adr = l3->getDefaultAddr();
      if (adr != 0)
        {
          uint8_t adrbuf[3] = {0x28, (uint8_t)((adr>>8)&0xFF), (uint8_t)(adr&0xFF) };
          sendbuf.write(adrbuf, sizeof(adrbuf));
          adr_check = false;
        }
    }

  watch = 1;
  watchdogtimer.start(10,0);
  trigger.send();
}

void
TPUART_Base::watchdogtimer_cb(ev::timer &w, int revents)
{
  if (watch == 1 && mode != BUSMODE_MONITOR)
    send_reset();
  if (watch == 1 && mode == BUSMODE_MONITOR)
    watch = 0;
  if (watch == 2)
    watch = 0;
}

void
TPUART_Base::sendtimer_cb(ev::timer &w, int revents)
{
  retry++;
  if (retry >= 3)
    {
      TRACEPRINTF (t, 0, this, "Drop Send");
      l.reset();
    }
  trigger.send();
}

