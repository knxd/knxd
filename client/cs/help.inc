/*
    EIBD client library
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

public class EIBBuffer
{
  public byte[] data;

  public EIBBuffer (byte[]d)
  {
    data = d;
  }
  public EIBBuffer ()
  {
    data = null;
  }
}

public class EIBAddr
{
  public ushort addr;

  public EIBAddr ()
  {
    addr = 0;
  }

  public EIBAddr (ushort val)
  {
    addr = val;
  }
}

public class UInt8
{
  public byte data;

  public UInt8 ()
  {
    data = 0;
  }

  public UInt8 (byte value)
  {
    data = value;
  }
}

public class Int16
{
  public short data;

  public Int16 ()
  {
    data = 0;
  }

  public Int16 (short value)
  {
    data = value;
  }
}

public class UInt16
{
  public ushort data;

  public UInt16 ()
  {
    data = 0;
  }

  public UInt16 (ushort value)
  {
    data = value;
  }
}

public class UInt32
{
  public uint data;

  public UInt32 ()
  {
    data = 0;
  }

  public UInt32 (ushort value)
  {
    data = value;
  }
}
