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

private Socket sock = null;

public
EIBConnection (String host):
this (host, 6720)
{
}

public
EIBConnection (String host, int port)
{
  IPAddress ip = Dns.GetHostEntry (host).AddressList[0];
  IPEndPoint endpoint = new IPEndPoint (ip, port);

  sock = new Socket (AddressFamily.InterNetwork,
		     SocketType.Stream, ProtocolType.Tcp);
  sock.Connect (endpoint);
  sock.SetSocketOption (SocketOptionLevel.Tcp, SocketOptionName.NoDelay, 1);
}


protected int
_EIB_SendRequest (byte[]data)
{
  if (sock == null)
    throw new Exception ("connection closed");
  byte[]len = new byte[2];
  if (data.Length > 0xffff || data.Length < 2)
    {
      errno = EINVAL;
      return -1;
    }
  len[0] = (byte) ((data.Length >> 8) & 0xff);
  len[1] = (byte) ((data.Length) & 0xff);
  sock.Send (len);
  sock.Send (data);
  return 0;
}

protected int
_EIB_CheckRequest (bool block)
{
  int res;
  if (sock == null)
    throw new Exception ("connection closed");

  if (readlen == 0)
    head = new byte[2];
  if (!block && sock.Available < 1)
    return 0;
  if (readlen < 2)
    {
      res = sock.Receive (head, readlen, 2 - readlen, SocketFlags.None);
      if (res <= 0)
	{
	  throw new Exception ("connection closed");
	}
      readlen += res;
      if (readlen < 2)
	return 0;
      res = SHORT2INT (head[0], head[1]);
      data = new byte[res];
    }
  if (!block && sock.Available < 1)
    return 0;
  res = sock.Receive (data, readlen - 2, data.Length + 2 - readlen,
		      SocketFlags.None);
  if (res <= 0)
    {
      throw new Exception ("connection closed");
    }
  readlen += res;
  return 0;
}

protected int
_EIB_GetRequest ()
{
  do
    {
      if (_EIB_CheckRequest (true) == -1)
	return -1;
    }
  while (readlen < 2 || (readlen >= 2 && readlen < data.Length + 2));
  readlen = 0;
  return 0;
}

public int
EIB_Poll_Complete ()
{
  if (_EIB_CheckRequest (false) == -1)
    return -1;
  if (readlen < 2 || (readlen >= 2 && readlen < data.Length + 2))
    return 0;
  return 1;
}

public int
EIBClose ()
{
  if (sock == null)
    throw new Exception ("connection closed");
  try
  {
    sock.Shutdown (SocketShutdown.Both);
    sock.Close ();
  }
  finally
  {
    sock = null;
  }
  return 0;
}

public int
EIBClose_sync ()
{
  try
  {
    EIBReset ();
  }
  catch (Exception e)
  {
  }
  return EIBClose ();
}
