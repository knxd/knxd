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

FT12LowLevelDriver::FT12LowLevelDriver (const DriverPtr& p, IniSection &s)
    : LowLevelDriver(p,s)
{
}

bool
FT12LowLevelDriver::setup(DriverPtr master)
{
  if(!LowLevelDriver::setup(master))
    return false;
  dev = cfg.value("device","");
  if (!dev.size())
    {
      ERRORPRINTF (t, E_ERROR | 36, "Section '%s' requires a 'device=' entry", cfg.name);
      return false;
    }
  return true;
}

void
FT12LowLevelDriver::start()
{
  struct termios t1;
  TRACEPRINTF (t, 1, "Open");

  fd = open (dev.c_str(), O_RDWR | O_NOCTTY | O_NDELAY | O_SYNC);
  if (fd == -1)
    goto ex;
  set_low_latency (fd, &sold);
  close (fd);

  fd = open (dev.c_str(), O_RDWR | O_NOCTTY);
  if (fd == -1)
    goto ex;
  if (tcgetattr (fd, &old))
    {
      ERRORPRINTF (t, E_ERROR | 34, "%s: getattr_old: %s", cfg.name, strerror(errno));
      goto ex2;
    }

  if (tcgetattr (fd, &t1))
    {
      ERRORPRINTF (t, E_ERROR | 34, "%s: getattr: %s", cfg.name, strerror(errno));
      goto ex2;
    }
  t1.c_cflag = CS8 | PARENB | CLOCAL | CREAD;
  t1.c_iflag = IGNBRK | INPCK | ISIG;
  t1.c_oflag = 0;
  t1.c_lflag = 0;
  t1.c_cc[VTIME] = 1;
  t1.c_cc[VMIN] = 0;
  cfsetospeed (&t1, B19200);
  cfsetispeed (&t1, 0);

  if (tcsetattr (fd, TCSAFLUSH, &t1))
    {
      ERRORPRINTF (t, E_ERROR | 34, "%s: setattr: %s", cfg.name, strerror(errno));
      goto ex2;
    }
  set_low_latency (fd, &sold);
  setup_buffers();

  sendflag = 0;
  recvflag = 0;
  repeatcount = 0;
  TRACEPRINTF (t, 1, "Opened");
  started();
  return;

ex2:
  if (fd > -1)
    restore_low_latency (fd, &sold);
ex:
  if (fd >= -1)
    {
      close(fd);
      fd = -1;
    }
  stopped();
}

void
FT12LowLevelDriver::setup_buffers()
{
  sendbuf.init(fd);
  recvbuf.init(fd);

  recvbuf.on_read.set<FT12LowLevelDriver,&FT12LowLevelDriver::read_cb>(this);
  recvbuf.on_error.set<FT12LowLevelDriver,&FT12LowLevelDriver::error_cb>(this);
  sendbuf.on_error.set<FT12LowLevelDriver,&FT12LowLevelDriver::error_cb>(this);
  timer.set <FT12LowLevelDriver,&FT12LowLevelDriver::timer_cb> (this);
  sendtimer.set <FT12LowLevelDriver,&FT12LowLevelDriver::sendtimer_cb> (this);

  trigger.set<FT12LowLevelDriver,&FT12LowLevelDriver::trigger_cb>(this);
  trigger.start();

  sendbuf.start();
  recvbuf.start();
}

void
FT12LowLevelDriver::error_cb()
{
  TRACEPRINTF (t, 2, "ERROR");
  stop();
}

void 
FT12LowLevelDriver::stop()
{
  // XXX TODO add de-registration callback

  recvbuf.stop();
  sendbuf.stop();
  timer.stop();
  sendtimer.stop();
  trigger.stop();

  TRACEPRINTF (t, 1, "Close");
  if (fd != -1)
    {
      tcsetattr (fd, TCSAFLUSH, &old);
      restore_low_latency (fd, &sold);
      close (fd);
      fd = -1;
    }
}

FT12LowLevelDriver::~FT12LowLevelDriver ()
{
  stop ();
}

void
FT12LowLevelDriver::Send_Packet (CArray l)
{
  CArray pdu;
  uchar c;
  unsigned i;
  t->TracePacket (1, "Send", l);

  assert (l.size() <= 32);
  pdu.resize (l.size() + 7);
  pdu[0] = 0x68;
  pdu[1] = l.size() + 1;
  pdu[2] = l.size() + 1;
  pdu[3] = 0x68;
  if (sendflag)
    pdu[4] = 0x53;
  else
    pdu[4] = 0x73;
  sendflag = !sendflag;

  pdu.setpart (l.data(), 5, l.size());
  c = pdu[4];
  for (i = 0; i < l.size(); i++)
    c += l[i];
  pdu[pdu.size() - 2] = c;
  pdu[pdu.size() - 1] = 0x16;

  send_q.push (pdu);
  if (!send_wait)
    trigger.send();
}

void
FT12LowLevelDriver::SendReset ()
{
  CArray pdu;
  TRACEPRINTF (t, 1, "SendReset");
  pdu.resize (4);
  pdu[0] = 0x10;
  pdu[1] = 0x40;
  pdu[2] = 0x40;
  pdu[3] = 0x16;
  sendflag = 0;
  recvflag = 0;
  send_q.push (pdu);
  if (!send_wait)
    trigger.send();
}

size_t
FT12LowLevelDriver::read_cb(uint8_t *buf, size_t len)
{
  t->TracePacket (0, "Read", len, buf);
  akt.setpart (buf, akt.size(), len);
  process_read(false);
  return len;
}

void
FT12LowLevelDriver::timer_cb(ev::timer &w, int revents)
{
  process_read(true);
}

void
FT12LowLevelDriver::process_read(bool is_timeout)
{
  while (akt.size() > 0)
    {
      if (akt[0] == 0xE5 && send_wait)
        {
          send_q.get ();
          akt.deletepart (0, 1);
          timer.stop();
          send_wait = false;
          if (!send_q.isempty())
            trigger.send();
          repeatcount = 0;
        }
      else if (akt[0] == 0x10)
        {
          if (akt.size() < 4)
            break;
          if (akt[1] == akt[2] && akt[3] == 0x16)
            {
              uchar c1 = 0xE5;
              t->TracePacket (0, "Send Ack", 1, &c1);
              sendbuf.write(&c1,1);
              if ((akt[1] == 0xF3 && !recvflag) ||
                  (akt[1] == 0xD3 && recvflag))
                {
                  //correct sequence number
                  recvflag = !recvflag;
                }
              if ((akt[1] & 0x0f) == 0)
                {
                  const uchar reset[1] = { 0xA0 };
                  CArray *c = new CArray (reset, sizeof (reset));
                  t->TracePacket (0, "RecvReset", *c);
                  on_recv (c);
                }
            }
          akt.deletepart (0, 4);
        }
      else if (akt[0] == 0x68)
        {
          int len;
          uchar c1;
          if (akt.size() < 7)
            break;
          if (akt[1] != akt[2] || akt[3] != 0x68)
            {
              //receive error, try to resume
              goto err_out;
            }
          if (akt.size() < akt[1] + 6U)
            break;

          c1 = 0;
          for (unsigned int i = 4; i < akt[1] + 4U; i++)
            c1 += akt[i];
          if (akt[akt[1] + 4] != c1 || akt[akt[1] + 5] != 0x16)
            {
              len = akt[1] + 6;
              //Forget wrong short frame
              akt.deletepart (0, len);
              continue;
            }

          c1 = 0xE5;
          t->TracePacket (0, "Send Ack", 1, &c1);
          sendbuf.write (&c1, 1);

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
              len = akt[1] + 6;
              c->setpart (akt.data() + 5, 0, len - 7);
              last = *c;
              on_recv (c);
            }
          // XXX TODO otherwise set 'len' to what? Or continue?
          akt.deletepart (0, len);
        }
      else
        /* if timeout OR an unknown byte, drop it. */
        if (false)
          {
    err_out:
            if (!is_timeout)
              break;
          }
        akt.deletepart (0, 1);
    }

  if (akt.size())
    timer.start(0.15,0);
}

void
FT12LowLevelDriver::sendtimer_cb(ev::timer &w, int revents)
{
  send_wait = false;
  trigger.send();
}

void
FT12LowLevelDriver::trigger_cb (ev::async &w, int revents)
{
  if (send_wait)
    return;
  if (send_q.isempty())
    return;

  const CArray &c = send_q.front ();
  t->TracePacket (0, "Send", c);
  repeatcount++;
  sendbuf.write(c.data(), c.size());
  send_wait = true;
  timer.start(0.2, 0);
}

EMIVer FT12LowLevelDriver::getEMIVer ()
{
  return vEMI2;
}
