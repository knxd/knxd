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

public const int EINVAL = 1;
public const int ECONNRESET = 2;
public const int EBUSY = 3;
public const int EADDRINUSE = 4;
public const int ETIMEDOUT = 5;
public const int EADDRNOTAVAIL = 6;
public const int EIO = 7;
public const int EPERM = 8;
public const int ENOENT = 9;
public const int ENODEV = 10;

private int errno = 0;

public int
getLastError ()
{
  return errno;
}
