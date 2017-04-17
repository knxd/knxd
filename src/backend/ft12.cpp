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

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "ft12.h"

#include "emi.h"
#include "emi1.h"
#include "emi2.h"
#include "cemi.h"

#include "llserial.h"
#include "lltcp.h"
#include "log.h"

FT12Driver::~FT12Driver() {}
FT12cemiDriver::~FT12cemiDriver() {}

class FT12serial : public LLserial
{
public:
  FT12serial(LowLevelIface* a, IniSectionPtr& b) : LLserial(a,b) { t->setAuxName("FT12_ser"); }
  virtual ~FT12serial() {}
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

bool
FT12Driver::make_EMI()
{
  switch (getVersion())
    {
    case vEMI1:
      iface = new EMI1Driver (this,cfg, iface);
      break;
    case vEMI2:
      iface = new EMI2Driver (this,cfg, iface);
      break;
    case vCEMI:
      iface = new FT12CEMIDriver (this,cfg, iface);
      break;
    case vRaw:
      break;
    case vUnknown:
      return false;
    default: 
      TRACEPRINTF (t, 2, "Unsupported EMI");
      return false;
    }

  if (t->ShowPrint(0))
    iface = new LLlog (this,cfg, iface);
  return iface->setup();
}

bool
FT12Driver::setup()
{
  FDdriver *fdd;

  iface = new FT12wrap(this, cfg);

  if (t->ShowPrint(0))
    iface = new LLlog (this,cfg, iface);

  if(!LowLevelAdapter::setup())
    goto ex1;

  if (!make_EMI())
    goto ex1;

  return true;

ex1:
  return false;
}

FT12wrap::FT12wrap (LowLevelIface* c, IniSectionPtr& s, LowLevelDriver *i) : LowLevelFilter(c,s,i)
{
  t->setAuxName("ft12wrap");
}

bool
FT12wrap::setup()
{
  if (cfg->value("device","").length() > 0)
    {
      if (cfg->value("ip-address","").length() > 0 ||
          cfg->value("port",-1) != -1)
        {
          ERRORPRINTF (t, E_ERROR, "Don't specify both device and IP options!");
          return false;
        }
      iface = new FT12serial(this, cfg);
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

  if (!iface->setup())
    return false;

  return true;
}


void
FT12wrap::start()
{
  TRACEPRINTF (t, 1, "Open");

  setup_buffers();

  sendflag = false;
  recvflag = false;
  next_free = true;
  send_wait = false;
  in_reader = false;

  repeatcount = 0;
  TRACEPRINTF (t, 1, "Opened");
  LowLevelFilter::start();
  return;
}

void
FT12wrap::setup_buffers()
{
  timer.set <FT12wrap,&FT12wrap::timer_cb> (this);
  sendtimer.set <FT12wrap,&FT12wrap::sendtimer_cb> (this);

  trigger.set<FT12wrap,&FT12wrap::trigger_cb>(this);
  trigger.start();
}

void 
FT12wrap::stop_()
{
  // XXX TODO add de-registration callback

  timer.stop();
  sendtimer.stop();
  trigger.stop();
}

void 
FT12wrap::stop()
{
  TRACEPRINTF (t, 1, "Close");

  stop_();
  LowLevelFilter::stop();
}

FT12wrap::~FT12wrap ()
{
  stop_ ();
}

void
FT12wrap::send_Data (CArray& l)
{
  do_send_Local (l, 0);
}

void
FT12wrap::do_send_Local (CArray& l, int raw)
{
  if(out.size() > 0)
    {
      ERRORPRINTF (t, E_ERROR | 36, "Send while data");
      return;
    }
  if (!raw)
    {
      uchar c;
      unsigned i;

      out.resize (l.size() + 7);
      out[0] = 0x68;
      out[1] = l.size() + 1;
      out[2] = l.size() + 1;
      out[3] = 0x68;
      if (sendflag)
        out[4] = 0x53;
      else
        out[4] = 0x73;
      sendflag = !sendflag;

      out.setpart (l.data(), 5, l.size());
      c = out[4];
      for (i = 0; i < l.size(); i++)
        c += l[i];
      out[out.size() - 2] = c;
      out[out.size() - 1] = 0x16;
    }
  else
    {
      assert (raw == 1);
      out = l;
    }

  if (!send_wait)
    trigger.send();
}

void
FT12wrap::recv_Data(CArray &c)
{
  uint8_t *buf = c.data();
  size_t len = c.size();

  akt.setpart (buf, akt.size(), len);
  process_read(false);
}

void
FT12wrap::timer_cb(ev::timer &w UNUSED, int revents UNUSED)
{
  process_read(true);
}

void
FT12wrap::do_send_Next()
{
  next_free = true;
  if (akt.size() > 0)
    {
      process_read(false);
      if (!next_free)
        return;
    }
  if (send_wait)
    {
      send_wait = false;
      trigger.send();
    }
}

void
FT12wrap::do__send_Next()
{
  out.clear();
  sendtimer.stop();
  LowLevelFilter::do_send_Next();
}

void
FT12wrap::process_read(bool is_timeout)
{
  if (in_reader)
    return;
  in_reader = true;
  timer.stop();

  t->TracePacket (1, "Processing", akt);
  while (akt.size() > 0 && next_free)
    {
      if (akt[0] == 0xE5 && send_wait)
        {
          akt.deletepart (0, 1);
          send_wait = false;
          do__send_Next();
          repeatcount = 0;
        }
      else if (akt[0] == 0xE5)
        {
          TRACEPRINTF (t, 0, "Spurious ACK");
          akt.deletepart (0, 1);
        }
      else if (akt[0] == 0x10)
        {
          if (akt.size() < 4)
            break;
          if (akt[1] == akt[2] && akt[3] == 0x16)
            {
              uchar c1 = 0xE5;
              iface->send_Data(c1);
              if ((akt[1] == 0xF3 && !recvflag) ||
                  (akt[1] == 0xD3 && recvflag))
                {
                  //correct sequence number
                  recvflag = !recvflag;
                }
              if ((akt[1] & 0x0f) == 0)
                {
                  const uchar reset[1] = { 0xA0 };
                  CArray c = CArray (reset, sizeof (reset));
                  t->TracePacket (0, "RecvReset", c);
                  LowLevelFilter::recv_Data (c);
                }
              akt.deletepart (0, 4);
            }
          else
            {
              akt.deletepart (0, 1);
            }
        }
      else if (akt[0] == 0x68)
        {
          int len;
          uchar c1;
          if (akt.size() < 7)
            break;
          if (akt[1] != akt[2] || akt[3] != 0x68)
            {
              akt.deletepart (0, 1);
              //receive error, try to resume
              goto err_out;
            }
          if (akt.size() < akt[1] + 6U)
            break;

          c1 = 0;
          for (unsigned int i = 4; i < akt[1] + 4U; i++)
            c1 += akt[i];
          len = akt[1] + 6;
          if (akt[akt[1] + 4] != c1 || akt[akt[1] + 5] != 0x16)
            {
              //Forget wrong short frame
              akt.deletepart (0, len);
              continue;
            }

          c1 = 0xE5;
          iface->send_Data (c1);

          if (akt[4] == (recvflag ? 0xF3 : 0xD3))
            { // repeat packet?
              if (CArray (akt.data() + 5, akt[1] - 1) != last)
                {
                  TRACEPRINTF (t, 0, "Sequence jump");
                  recvflag = !recvflag;
                }
              else
                TRACEPRINTF (t, 0, "Wrong Sequence");
            }
          else if (akt[4] == (recvflag ? 0xD3 : 0xF3))
            {
              recvflag = !recvflag;
              CArray *c = new CArray;
              c->setpart (akt.data() + 5, 0, len - 7);
              last = *c;
              LowLevelFilter::recv_Data (*c);
            }
          akt.deletepart (0, len);
        }
      else
        {
          /* if timeout OR an unknown byte, drop it. */
          if (false)
            {
      err_out:
              if (!is_timeout)
                break;
            }
          akt.deletepart (0, 1);
        }
    }

  if (akt.size())
    {
      t->TracePacket (1, "Processing: left", akt);
      timer.start(0.15,0);
    }
  in_reader = false;
}

void
FT12wrap::sendtimer_cb(ev::timer &w UNUSED, int revents UNUSED)
{
  send_wait = false;
  trigger.send();
}

void
FT12wrap::trigger_cb (ev::async &w UNUSED, int revents UNUSED)
{
  if (send_wait || !next_free)
    return;
  if (out.size() == 0)
    return;

  repeatcount++;
  iface->send_Data(out.data(), out.size());
  send_wait = true;
  timer.start(0.2, 0);
}

void FT12wrap::sendReset()
{
  const uchar t1[] = { 0x10, 0x40, 0x40, 0x16 };
  CArray l (t1, sizeof (t1));
  do_send_Local (l, 1);
}


FT12CEMIDriver::~FT12CEMIDriver()
{
}

void
FT12CEMIDriver::cmdOpen()
{
  sendLocal_done_next = N_up;
  const uchar t1[] = { 0xF6, 0x00, 0x08, 0x01, 0x34, 0x10, 0x01, 0x00 };
  send_Local (CArray (t1, sizeof (t1)),1);
}


