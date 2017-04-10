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
#include "router.h"

#define NO_MAP
#include "nat.h"
#include "llserial.h"
#include "lltcp.h"
#include "log.h"

TPUART::~TPUART() {}

class TPUARTserial : public LLserial
{
public:
  TPUARTserial(LowLevelIface* a, IniSectionPtr& b) : LLserial(a,b) { t->setAuxName("TPU_ser"); }
  virtual ~TPUARTserial();
protected:
  void termios_settings (struct termios &t1)
    {
      t1.c_cflag = CS8 | CLOCAL | CREAD | PARENB;
      t1.c_iflag = IGNBRK | INPCK | ISIG;
      t1.c_oflag = 0;
      t1.c_lflag = 0;
      t1.c_cc[VTIME] = 1;
      t1.c_cc[VMIN] = 0;
    }
  unsigned int default_baudrate() { return 19200; }
};

LowLevelFilter *
TPUART::create_wrapper(LowLevelIface* parent, IniSectionPtr& s, LowLevelDriver* i)
{
  return new TPUARTwrap(parent,s, i);
}

FDdriver *
TPUARTwrap::create_serial(LowLevelIface* parent, IniSectionPtr& s)
{
  return new TPUARTserial(parent,s);
}


static const char* SN(enum TSTATE s)
{
  static int x = 0;
  static char buf[2][10];
  switch(s)
    {
    case T_new:            return "new";
    case T_error:          return "error";
    case T_start:          return "start";
    case T_in_reset:       return "in_reset";
    case T_in_setaddr:     return "in_setaddr";
    case T_in_getstate:    return "in_getstate";
    case T_is_online:      return "is_online";
    case T_wait:           return "wait";
    case T_wait_more:      return "wait_more";
    case T_wait_keepalive: return "wait_keepalive";
    case T_busmonitor:     return "busmonitor";
    default:
      x = 1-x;
      sprintf(buf[x],"? %d",s);
      return buf[x];
    }
}

bool
TPUART::setup()
{
  iface = create_wrapper(this, cfg);

  if (t->ShowPrint(0))
    iface = new LLlog (this,cfg, iface);

  if (!LowLevelAdapter::setup())
    return false;

  return true;
ex1:
  delete iface;
  iface = nullptr;
  return false;
}

bool
TPUARTwrap::setup()
{
  ackallgroup = cfg->value("ack-group",false);
  ackallindividual = cfg->value("ack-individual",false);
  monitor = cfg->value("monitor",false);

  if (cfg->value("device","").length() > 0)
    {
      if (cfg->value("ip-address","").length() > 0 ||
          cfg->value("port",-1) != -1)
        {
          ERRORPRINTF (t, E_ERROR, "Don't specify both device and IP options!");
          return false;
        }
      iface = create_serial(this, cfg);
    }
  else
    {
      if (cfg->value("baudrate",-1) != -1)
        {
          ERRORPRINTF (t, E_ERROR, "Don't specify both device and IP options!");
          return false;
        }
      iface = new LLtcp(this, cfg);
    }

  if (t->ShowPrint(0))
    iface = new LLlog (this,cfg, iface);

  FilterPtr single = findFilter("single");
  if (single != nullptr)
    {
      std::shared_ptr<NatL2Filter> f = std::dynamic_pointer_cast<NatL2Filter>(single);
      if (f)
        my_addr = f->addr;
    }
  
  if (!LowLevelFilter::setup())
    return false;

  return true;
}

TPUARTwrap::~TPUARTwrap ()
{
  TRACEPRINTF (t, 2, "Close");

  timer.stop();
  sendtimer.stop();
}

TPUARTserial::~TPUARTserial() {}

void
TPUARTwrap::send_L_Data (LDataPtr l)
{
  assert(out.size() == 0);
  out = l->ToPacket ();

  send_again();
}

/* ignore low level send_Next -- just assume that this works */
void
TPUARTwrap::do_send_Next()
{
  next_free = true;
  if (send_wait)
    {
      send_wait = false;
      send_again();
    }
}

void
TPUARTwrap::do__send_Next()
{
  out.clear();
  send_retry = 0;
  sendtimer.stop();
  LowLevelFilter::do_send_Next();
}

void
TPUARTwrap::send_again()
{
  if (out.size() > 0 && state > T_is_online && state < T_busmonitor)
    {
      if (!next_free)
        {
          send_wait = true;
          return;
        }

      CArray w;
      unsigned i;
      unsigned z = out.size();

      w.resize (z * 2);
      for (i = 0; i < z; i++)
        {
          w[2 * i] = 0x80 | (i & 0x3f);
          w[2 * i + 1] = out[i];
        }
      z = (z - 1) * 2;
      w[z] = (w[z] & 0x3f) | 0x40;
      LowLevelFilter::send_Data(w);
      sendtimer.start(2,0);

      // clear retry flag. for later comparison
      out[0] &=~ 0x20;
    }
}

void
TPUARTwrap::started()
{
  setstate(T_new);
  setstate(T_start);
}

void
TPUARTwrap::stopped()
{
  setstate(T_new);

  LowLevelFilter::stopped();
}

void
TPUARTwrap::RecvLPDU (const uchar * data, int len)
{
  t->TracePacket (1, "RecvLP", len, data);
  if (state == T_busmonitor)
    {
      LBusmonPtr l = LBusmonPtr(new L_Busmonitor_PDU ());
      l->pdu.set (data, len);
      recv_L_Busmonitor (std::move(l));
    }
  else if (state > T_start)
    {
      LPDUPtr l = LPDU::fromPacket (CArray (data, len), t);
      if (l->getType () != L_Data)
        TRACEPRINTF (t, 1, "dropping packet: type %d", l->getType ());
      else
        {
          if (((L_Data_PDU *)(&*l))->valid_checksum)
            recv_L_Data (dynamic_unique_cast<L_Data_PDU>(std::move(l)));
          else
            TRACEPRINTF (t, 1, "dropping packet: invalid");
        }
    }
}

TPUARTwrap::TPUARTwrap(LowLevelIface* parent, IniSectionPtr& s, LowLevelDriver* i) : LowLevelFilter(parent,s,i)
{
  timer.set <TPUARTwrap,&TPUARTwrap::timer_cb> (this);
  sendtimer.set <TPUARTwrap,&TPUARTwrap::sendtimer_cb> (this);
}

void
TPUARTwrap::sendtimer_cb(ev::timer &w UNUSED, int revents UNUSED)
{
  if (send_retry++ > 3)
    {
      ERRORPRINTF (t, E_ERROR, "send timeout: too many retries");
      setstate(T_error);
      return;
    } // TODO error
  TRACEPRINTF (t, 8, "send timeout: retry");
  send_again();
}

void
TPUARTwrap::timer_cb(ev::timer &w UNUSED, int revents UNUSED)
{
  switch(state)
    {
    case T_error:
      stop();
      break;
    case T_new:
      break;
    case T_in_reset:
      if (retry < 3)
        {
          setstate(T_in_reset);
          return;
        }
      setstate(T_error);
      break;

    case T_in_getstate:
      if (retry > 5)
        {
          errored();
          setstate(T_new);
          return;
        }
      setstate(state);
      break;

    case T_in_setaddr:
      {
        uint8_t addrbuf[2] = { (uint8_t)((my_addr>>8)&0xFF), (uint8_t)(my_addr&0xFF) };
        TRACEPRINTF (t, 0, "SendAddr %02X%02X", addrbuf[0],addrbuf[1]);
        LowLevelIface::send_Data(CArray(addrbuf, sizeof(addrbuf)));
        setstate(T_in_getstate);
      }
      break;

    case T_wait:
      setstate(T_wait_keepalive);
      break;
    case T_wait_more:
      t->TracePacket (8, "Incomplete packet", in);
      in.clear();
      setstate(T_wait);
      break;
    case T_wait_keepalive:
      if (retry < 3)
        {
          setstate(T_in_reset);
          return;
        }
      setstate(T_wait_keepalive);
      break;
    default:
      TRACEPRINTF (t, 8, "Timeout in state %s",SN(state));
      break;
    }
}

void
TPUARTwrap::in_check()
{
  bool ext = !(in[0] & 0x80);

  if (in.size () >= 6u+ext)
    {

      if (!acked && !recvecho && my_addr == 0 && state >= T_is_online && state < T_busmonitor)
        {
          if (out.size() >= 6u+ext && !((in[0]^out[0])&~0x20) && !memcmp(in.data()+1,out.data()+1,5+ext))
            recvecho = true;
          else
            {
              uchar c = 0x10;
              if ((in[ext ? 1 : 5] & 0x80) == 0)
                {
                  if (ackallindividual || checkSysAddress ((in[3+ext] << 8) | in[4+ext]))
                    c |= 0x1;
                }
              else
                {
                  if (ackallgroup || checkSysGroupAddress ((in[3+ext] << 8) | in[4+ext]))
                    c |= 0x1;
                }
              TRACEPRINTF (t, 0, "SendAck %02X", c);
              LowLevelIface::send_Data(c);
              acked = true;
            }
        }

      unsigned len = ext ? in[6] : (in[5] & 0x0f);
      len += 6 + ext + 2;

      if (in.size() > len)
        TRACEPRINTF (t, 8, "Datalen %d has len %d?", len, in.size());

      if (in.size() >= len)
        {
          if (!recvecho)
            RecvLPDU (in.data(), in.size());
          in.clear();
        }
    }

  if (state > T_is_online && state < T_busmonitor)
    {
      if (in.size() == 0)
        setstate(T_wait);
      else
        setstate(T_wait_more);
    }
}

void
TPUARTwrap::recv_Data(CArray &c)
{
  uint8_t *buf = c.data();
  size_t len = c.size();

  if (state < T_start)
    {
      t->TracePacket (0, "ReadDrop", len, buf);
      return; // discard
    }

  while(len--)
    {
      uint8_t c = *buf++;
      if (in.size() > 0)
        {
          in.setpart (&c, in.size(), 1);
          in_check();
          continue;
        }
      if (skip_char)
        {
          skip_char = false;
          continue;
        }

      if (c == 0x03) // RESET
        {
          if (state == T_in_reset)
            {
              TRACEPRINTF (t, 8, "RESET_ACK");
              setstate(T_in_setaddr);
            }
          else 
            TRACEPRINTF (t, 8, "spurious RESET_ACK");
        }
      else if (c == 0x8B) // L_DataConfirm positive
        {
          if (out.size() == 0 || state < T_is_online)
            {
              TRACEPRINTF (t, 8, "ACK: but not sending");
              continue;
            }
          do__send_Next();
          continue;
        }
      else if (c == 0xCB) // frame end, NCN5120
        { }
      else if (c == 0x0B) // L_DataConfirm negative
        {
          if (out.size() == 0 || state < T_is_online)
            {
              TRACEPRINTF (t, 8, "NACK: but not sending");
              continue;
            }
          do__send_Next();
          continue;
        }
      else if ((c & 0x17) == 0x13) // frame state indication, NCN5120
        { }
      else if ((c & 0x07) == 0x07) // state indication
        {
          TRACEPRINTF (t, 8, "State: %02X", c);
          if (c != 0x07)
            ERRORPRINTF (t, E_WARNING | 63, "TPUART error state x%02X", c);

          switch(state)
            {
            case T_wait_keepalive:
              setstate(T_wait);
              break;
            case T_in_reset:
              // setstate(T_in_reset); // do not immediately retry
              break;
            case T_in_setaddr:
              if (c == 0x47)
                {
                  ERRORPRINTF (t, E_ERROR | 62, "TPUART detected. Hardware ACK not supported.");
                  my_addr = 0;
                }
              setstate(T_in_getstate);
              break;
            case T_in_getstate:
              setstate(T_is_online);
              break;

            default:
              ERRORPRINTF (t, E_WARNING | 63, "TPUART state %s should not happen", SN(state));
              break;
            }
        }
      /*
        * 0xCC acknowledge frame
        * 0x0C NotAcknowledge frame
        * 0xC0 Busy Frame
        */
      else if (c == 0xCC || c == 0xC0 || c == 0x0C)
        {
          RecvLPDU (in.data(), 1);
        }
      else if ((c & 0x50) == 0x10) // Matches KNX control byte L_Data_Standard/Extended Frame
        {
          assert(!in.size());
          in.setpart (&c, in.size(), 1);
        }
      else
        {
          acked = false;
          TRACEPRINTF (t, 0, "unknown %02X", c);
        }
    }
  return;
}

void
TPUARTwrap::setstate(enum TSTATE new_state)
{
  if (state != new_state)
    TRACEPRINTF (t, 8, "state: %s > %s", SN(state),SN(new_state));

  if (state < T_is_online && new_state >= T_is_online)
    {
      LowLevelFilter::started();
      if (monitor)
        new_state = T_busmonitor;
      else if (new_state < T_busmonitor)
        send_again();
    }

  switch(new_state)
    {
    case T_start:
      new_state = T_in_reset;
      /* fall thru */
    case T_in_reset:
      if (state == T_in_reset)
        retry++;
      else
        retry = 1;
      {
        uchar c = 0x01;
        TRACEPRINTF (t, 0, "SendReset %02X", c);
        LowLevelIface::send_Data(c);
      }
      timer.start(0.5,0);
      break;

    case T_in_setaddr:
      if (my_addr)
        {
          uchar c = 0x28;
          TRACEPRINTF (t, 0, "SendAddr %02X", c);
          LowLevelIface::send_Data(c);
          timer.start(0.2,0);
          break;
        }
      new_state = T_in_getstate;
      TRACEPRINTF (t, 8, "addr zero: %s > %s", SN(state),SN(new_state));
      // FALL THRU
    case T_in_getstate:
      {
        uchar c = 0x02;
        TRACEPRINTF (t, 0, "Send GetState %02X", c);
        LowLevelIface::send_Data(c);
        timer.start(0.5,0);
      }
      break;

    case T_busmonitor:
      {
        uchar c = 0x05;
        TRACEPRINTF (t, 0, "Send openBusmonitor %02X", c);
        LowLevelIface::send_Data(c);
      }
      break;

    case T_is_online:
      new_state = T_wait;
      // fall thru
    case T_wait:
      timer.start(10,0);
      acked = false;
      recvecho = false;
      break;

    case T_wait_more:
      timer.start(1,0);
      break;

    case T_wait_keepalive:
      {
        uchar c = 0x02;
        TRACEPRINTF (t, 0, "Send GetState %02X", c);
        LowLevelIface::send_Data(c);
        timer.start(0.5,0);
        break;
      }

    case T_error:
      timer.start(1,0);
      break;

    default:
      break;
    }
  state = new_state;
}


