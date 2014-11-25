# 
#   EIBD client library
#   Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>
# 
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
# 
#   In addition to the permissions in the GNU General Public License, 
#   you may link the compiled version of this file into combinations
#   with other programs, and distribute those combinations without any 
#   restriction coming from the use of this file. (The General Public 
#   License restrictions do apply in other respects; for example, they 
#   cover modification of the file, and distribution when not linked into 
#   a combine executable.)
# 
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
# 
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
# 

import errno;
import socket;

class EIBBuffer:
  def __init__(self, buf = []):
    self.buffer = buf

class EIBAddr:
  def __init__(self, value = 0):
    self.data = value

class EIBInt8:
  def __init__(self, value = 0):
    self.data = value

class EIBInt16:
  def __init__(self, value = 0):
    self.data = value

class EIBInt32:
  def __init__(self, value = 0):
    self.data = value

class EIBConnection:
  def __init__(self):
    self.data = []
    self.readlen = 0
    self.datalen = 0
    self.fd = None
    self.errno = 0
    self.__complete = None

  def EIBSocketLocal(self, path):
    if self.fd != None:
      self.errno = errno.EUSERS
      return -1
    fd = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    fd.connect(path)
    self.data = []
    self.readlen = 0
    self.fd = fd
    return 0

  def EIBSocketRemote(self, host, port = 6720):
    if self.fd != None:
      self.errno = errno.EUSERS
      return -1
    fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    fd.connect((host, port))
    self.data = []
    self.readlen = 0
    self.fd = fd
    return 0

  def EIBSocketURL(self, url):
    if url[0:6] == 'local:':
      return self.EIBSocketLocal(url[6:])
    if url[0:3] == 'ip:':
      parts=url.split(':')
      if (len(parts) == 2):
        parts.append(6720)
      return self.EIBSocketRemote(parts[1], int(parts[2]))
    self.errno = errno.EINVAL
    return -1

  def EIBComplete(self):
    if self.__complete == None:
      self.errno = errno.EINVAL
      return -1
    return self.__complete()

  def EIBClose(self):
    if self.fd == None:
      self.errno = errno.EINVAL
      return -1
    self.fd.close()
    self.fd = None

  def EIBClose_sync(self):
    self.EIBReset()
    return self.EIBClose()

  def __EIB_SendRequest(self, data):
    if self.fd == None:
      self.errno = errno.ECONNRESET
      return -1
    if len(data) < 2 or len(data) > 0xffff:
      self.errno = errno.EINVAL
      return -1
    data = [ (len(data)>>8)&0xff, (len(data))&0xff ] + data
    result = ''
    for i in data:
      result += chr(i)
    self.fd.send(result)
    return 0

  def EIB_Poll_FD(self):
    if self.fd == None:
      self.errno = errno.EINVAL
      return -1
    return self.fd

  def EIB_Poll_Complete(self):
    if self.__EIB_CheckRequest(False) == -1:
      return -1
    if self.readlen < 2 or (self.readlen >= 2 and self.readlen < self.datalen + 2):
      return 0
    return 1

  def __EIB_GetRequest(self):
     while True:
      if self.__EIB_CheckRequest(True) == -1:
        return -1
      if self.readlen >= 2 and self.readlen >= self.datalen + 2:
        self.readlen = 0
        return 0

  def __EIB_CheckRequest(self, block):
    if self.fd == None:
      self.errno = errno.ECONNRESET
      return -1
    if self.readlen == 0:
      self.head = []
      self.data = []
    if self.readlen < 2:
      self.fd.setblocking(block)
      result = self.fd.recv (2-self.readlen)
      for a in result:
        self.head.append(ord(a))
      self.readlen += len(result)
    if self.readlen < 2:
      return 0
    self.datalen = (self.head[0] << 8) | self.head[1]
    if self.readlen < self.datalen + 2:
      self.fd.setblocking(block)
      result = self.fd.recv (self.datalen + 2 -self.readlen)
      for a in result:
        self.data.append(ord(a))
      self.readlen += len(result)
    return 0      

