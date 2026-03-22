/* SystemTera eib bus access driver
    Copyright (C) 2014 Patrik Pfaffenbauer <patrik.pfaffenbauer@p3.co.at>

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
#include <fcntl.h>
#include <sys/ioctl.h>
#include "ncn5120.h"
#include "llserial.h"

class NCN5120wrap : public TPUARTwrap
{
public:
  NCN5120wrap (LowLevelIface* parent, IniSectionPtr& s, LowLevelDriver* i = nullptr) : TPUARTwrap(parent,s,i) {}
  virtual ~NCN5120wrap() = default;

protected:
  void termios_settings(struct termios &t);
  unsigned int default_baudrate();
  void setstate(enum TSTATE state);

  void RecvLPDU (const uint8_t * data, int len);
  void encode_frame(const CArray& frame, CArray& uart_buf) override;
  virtual LLserial * create_serial(LowLevelIface* parent, IniSectionPtr& s);
};

class NCN5120serial : public LLserial
{
public:
  NCN5120serial(LowLevelIface* a, IniSectionPtr& b) : LLserial(a,b)
  {
    t->setAuxName("NCN_ser");
  }
  virtual ~NCN5120serial () = default;

protected:
  unsigned int default_baudrate()
  {
    return 38400;
  }
  void termios_settings(struct termios &t1)
  {
    t1.c_cflag = CS8 | CLOCAL | CREAD;
    t1.c_iflag = IGNBRK | ISIG;
    t1.c_oflag = 0;
    t1.c_lflag = 0;
    t1.c_cc[VTIME] = 1;
    t1.c_cc[VMIN] = 0;
  }
};

LowLevelFilter *
NCN5120::create_wrapper(LowLevelIface* parent, IniSectionPtr& s, LowLevelDriver* i)
{
  return new NCN5120wrap(parent,s,i);
}


LLserial *
NCN5120wrap::create_serial(LowLevelIface* parent, IniSectionPtr& s)
{
  return new NCN5120serial(parent, s);
}

void NCN5120wrap::RecvLPDU (const uint8_t * data, int len)
{
  skip_char = true;
  TPUARTwrap::RecvLPDU (data, len);
}

void
NCN5120wrap::setstate(enum TSTATE new_state)
{
  if (new_state == T_in_setaddr && my_addr != 0)
    {
      uint8_t addrbuf[4] = { 0xF1, (uint8_t)((my_addr>>8)&0xFF), (uint8_t)(my_addr&0xFF), 0x00 };
      TRACEPRINTF (t, 0, "SendAddr %02X%02X%02X", addrbuf[0],addrbuf[1],addrbuf[2]);
      LowLevelIface::send_Data(addrbuf, sizeof(addrbuf));
      new_state = T_in_getstate;
    }
  TPUARTwrap::setstate(new_state);
}

void
NCN5120wrap::encode_frame(const CArray& frame, CArray& uart_buf)
{
  unsigned z = frame.size();

  // For short frames, use standard TPUART encoding
  if (z <= 63)
    {
      TPUARTwrap::encode_frame(frame, uart_buf);
      return;
    }

  // NCN5120/NCN5121: insert U_L_DataOffset.req (0x08 | offset) at each
  // 64-byte boundary to set upper 3 bits of 9-bit data index.
  // Offsets 5-7 are forbidden per datasheet; max frame = 263 bytes.
  unsigned extra = 0;
  for (unsigned i = 1; i < z; i++)
    if ((i >> 6) != ((i - 1) >> 6))
      extra++;
  uart_buf.resize(z * 2 + extra);

  unsigned wi = 0;
  unsigned cur_offset = 0;
  for (unsigned i = 0; i < z; i++)
    {
      unsigned new_offset = i >> 6;
      if (new_offset != cur_offset)
        {
          uart_buf[wi++] = 0x08 | (new_offset & 0x07);
          cur_offset = new_offset;
        }
      uart_buf[wi] = 0x80 | (i & 0x3f);
      uart_buf[wi + 1] = frame[i];
      wi += 2;
    }
  // Mark last as U_L_DataEnd
  wi -= 2;
  uart_buf[wi] = (uart_buf[wi] & 0x3f) | 0x40;
}
