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


require 'hexdump'
require 'socket'
include Socket::Constants

class EIBBuffer
  attr_accessor :buffer
  def initialize(buf = [])
    @buffer = buf
  end
end

class EIBAddr
  attr_accessor :data
  def initialize(value = 0)
    @data = value
  end
end

class EIBInt8
  attr_accessor :data
  def initialize(value = 0)
    @data = value
  end
end

class EIBInt16
  attr_accessor :data
  def initialize(value = 0)
    @data = value
  end
end

class EIBInt32
  attr_accessor :data
  def initialize(value = 0)
    @data = value
  end
end

class EIBConnection
  def initialize()
    @data = []
    @readlen = 0
    @datalen = 0
    @fd = nil
    @errno = 0
    @__complete = nil
  end
  
  def EIBSocketLocal(path)
    puts "EIBSocketLocal(path=#{path})" if $DEBUG
    if @fd != nil
      @errno = Errno::EUSERS
      return -1
    end
    fd = UNIXSocket.new(path)
    @data = []
    @readlen = 0
    @fd = fd
    return 0
  end

  def EIBSocketRemote(host, port = 6720)
    puts "EIBSocketURL(host=#{host} port=#{port})" if $DEBUG
    if @fd != nil
      @errno = Errno::EUSERS
      return -1
    end
    fd = TCPSocket.new(host, port)
    @data = []
    @readlen = 0
    @fd = fd
    return 0
  end
  
  def EIBSocketURL(url)
    if url.start_with?('local:')
      return EIBSocketLocal(url[6..-1])
    end
    if url.start_with?('ip:')
      parts=url.split(':')
      if (parts.length == 2)
        parts << "6720"
      end
      return EIBSocketRemote(parts[1], parts[2].to_i)
    end
    @errno = Errno::EINVAL
    return -1
  end
  
  def EIBComplete()
    if @__complete == nil
      @errno = Errno::EINVAL
      return -1
    end
    return @__complete
  end
  
  def EIBClose()
    if @fd == nil
      @errno = Errno::EINVAL
      return -1
    end
    @fd.close()
    @fd = nil
  end
  
  def EIBClose_sync()
    puts "EIBClose_sync()" if $DEBUG
    EIBReset()
    return EIBClose()
  end
  
  def __EIB_SendRequest(data)
    if @fd == nil
      @errno = Errno::ECONNRESET
      return -1
    end
    if data.length < 2 or data.length > 0xffff
      @errno = Errno::EINVAL
      return -1
    end
    data = [ (data.length>>8)&0xff, (data.length)&0xff ] + data
    #puts "__EIB_SendRequest(data=#{data.inspect} length=#{data.length})" if $DEBUG
    result = data.pack("C*")
    @fd.send(result, 0)
    puts "__EIB_SendRequest sent #{result.length} bytes: #{result.hexdump}" if $DEBUG
    return 0
  end
  
  def EIB_Poll_FD()
    if @fd == nil
      @errno = Errno::EINVAL
      return -1
    end
    return @fd
  end
  
  def EIB_Poll_Complete()
    puts "__EIB_Poll_Complete()" if $DEBUG
    if __EIB_CheckRequest(false) == -1
      return -1
    end
    if @readlen < 2 or (@readlen >= 2 and @readlen < @datalen + 2)
      return 0
    end
    return 1
  end
  
  def __EIB_GetRequest()
    puts "__EIB_GetRequest()" if $DEBUG
     while true
      if __EIB_CheckRequest(true) == -1
        return -1
      end
      if @readlen >= 2 and @readlen >= @datalen + 2
        @readlen = 0
        return 0
      end
    end
  end

  def __EIB_CheckRequest(block)
    puts "__EIB_CheckRequest(block=#{block})" if $DEBUG
    if @fd == nil
      @errno = Errno::ECONNRESET
      return -1
    end
    if @readlen == 0
      @head = []
      @data = []
    end
    if @readlen < 2
      maxlen = 2-@readlen
      result = block ? @fd.recv(maxlen) : @fd.recv_nonblock(maxlen)
      raise Errno::ECONNRESET if block and (result.length == 0)
      puts "__EIB_CheckRequest received #{result.length} bytes: #{result.hexdump})" if $DEBUG
      if result.length > 0
        @head.concat(result.split('').collect{|c| c.unpack('c')[0]})
      end
      puts "__EIB_CheckRequest @head after recv. = #{@head.inspect})" if $DEBUG
      @readlen += result.length
    end
    if @readlen < 2
      return 0
    end
    @datalen = (@head[0] << 8) | @head[1]
    if @readlen < @datalen + 2
      maxlen = @datalen + 2 -@readlen
      result = block ? @fd.recv(maxlen) : @fd.recv_nonblock(maxlen)
      raise Errno::ECONNRESET if block and (result.length == 0)
      puts "__EIB_CheckRequest received #{result.length} bytes: #{result.hexdump})" if $DEBUG
      if result.length > 0
        @data.concat(result.split('').collect{|c| c.unpack('c')[0]})
	puts "__EIB_CheckRequest @data after recv. = #{@data.inspect})" if $DEBUG
      end
      @readlen += result.length
    end #if
    return 0      
  end

