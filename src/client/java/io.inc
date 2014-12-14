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

  private int readlen = 0;
  private byte[] head = null;
  private byte[] data = null;

  private Socket con;
  private InputStream is;
  private OutputStream os;

  public EIBConnection (String host) throws IOException
  {
    this (host, 6720);
  }

  public EIBConnection (String host, int port) throws IOException
  {
    con = new Socket (host, port);
    is = con.getInputStream ();
    os = con.getOutputStream ();
    con.setTcpNoDelay (true);
  }

  protected int _EIB_SendRequest (byte[]data) throws IOException
  {
    if (is == null)
      throw new IOException ("connection closed");
    byte len[] = new byte[2];
    if (data.length > 0xffff || data.length < 2)
      {
	errno = EINVAL;
	return -1;
      }
    len[0] = (byte) ((data.length >> 8) & 0xff);
    len[1] = (byte) ((data.length) & 0xff);
    os.write (len);
    os.write (data);
    return 0;
  }

  protected int _EIB_CheckRequest (boolean block) throws IOException
  {
    int res;
    if (os == null)
      throw new IOException ("connection closed");

    if (readlen == 0)
      head = new byte[2];
    if (!block && is.available () < 1)
        return 0;
    if (readlen < 2)
      {
	res = is.read (head, readlen, 2 - readlen);
	if (res == -1)
	  {
	    throw new IOException ("connection closed");
	  }
	readlen += res;
	if (readlen < 2)
	  return 0;
	res = SHORT2INT (head[0], head[1]);
	data = new byte[res];
      }
    if (!block && is.available () < 1)
      return 0;
    res = is.read (data, readlen - 2, data.length + 2 - readlen);
    if (res == -1)
      {
	throw new IOException ("connection closed");
      }
    readlen += res;
    return 0;
  }

  protected int _EIB_GetRequest () throws IOException
  {
    do
      {
	if (_EIB_CheckRequest (true) == -1)
	  return -1;
      }
    while (readlen < 2 || (readlen >= 2 && readlen < data.length + 2));
    readlen = 0;
    return 0;
  }

  public int EIB_Poll_Complete () throws IOException
  {
    if (_EIB_CheckRequest (false) == -1)
      return -1;
    if (readlen < 2 || (readlen >= 2 && readlen < data.length + 2))
      return 0;
    return 1;
  }

  public int EIBClose () throws IOException
  {
    if (con == null)
      throw new IOException ("connection closed");
      try
    {
      con.close ();
    }
    finally
    {
      is = null;
      os = null;
      con = null;
    }
    return 0;
  }

  public int EIBClose_sync () throws IOException
  {
    try
    {
      EIBReset ();
    } catch (Exception e)
    {
    }
    return EIBClose ();
  }
