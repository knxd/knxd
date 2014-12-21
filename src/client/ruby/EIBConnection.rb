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

  def __EIBGetAPDU_Complete()
    puts "__EIBGetAPDU_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 37 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    @buf.buffer = @data[2..-1]
    return @buf.buffer.length
  end


  def EIBGetAPDU_async(buf)
    puts "EIBGetAPDU_async()" if $DEBUG
    ibuf = [0] * 2;
    @buf = buf
    @__complete = __EIBGetAPDU_Complete()
    return 0
  end

  def EIBGetAPDU(buf)
    puts "EIBGetAPDU()" if $DEBUG
    if EIBGetAPDU_async(buf) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBGetAPDU_Src_Complete()
    puts "__EIBGetAPDU_Src_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 37 or @data.length < 4
      @errno = Errno::ECONNRESET
      return -1
    end
    if @ptr5 != nil
      @ptr5.data = (((@data[2])<<8)|(@data[2+1]))
    end
    @buf.buffer = @data[4..-1]
    return @buf.buffer.length
  end


  def EIBGetAPDU_Src_async(buf, src)
    puts "EIBGetAPDU_Src_async()" if $DEBUG
    ibuf = [0] * 2;
    @buf = buf
    @ptr5 = src
    @__complete = __EIBGetAPDU_Src_Complete()
    return 0
  end

  def EIBGetAPDU_Src(buf, src)
    puts "EIBGetAPDU_Src()" if $DEBUG
    if EIBGetAPDU_Src_async(buf, src) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBGetBusmonitorPacket_Complete()
    puts "__EIBGetBusmonitorPacket_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 20 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    @buf.buffer = @data[2..-1]
    return @buf.buffer.length
  end


  def EIBGetBusmonitorPacket_async(buf)
    puts "EIBGetBusmonitorPacket_async()" if $DEBUG
    ibuf = [0] * 2;
    @buf = buf
    @__complete = __EIBGetBusmonitorPacket_Complete()
    return 0
  end

  def EIBGetBusmonitorPacket(buf)
    puts "EIBGetBusmonitorPacket()" if $DEBUG
    if EIBGetBusmonitorPacket_async(buf) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBGetBusmonitorPacketTS_Complete()
    puts "__EIBGetBusmonitorPacketTS_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 21 or @data.length < 7
      @errno = Errno::ECONNRESET
      return -1
    end
    if @ptr2 != nil
      @ptr2.data = @data[2]
    end
    if @ptr7 != nil
      @ptr7.data = (((@data[3])<<24)|((@data[3+1])<<16)|((@data[3+2])<<8)|(@data[3+3]))
    end
    @buf.buffer = @data[7..-1]
    return @buf.buffer.length
  end


  def EIBGetBusmonitorPacketTS_async(status, timestamp, buf)
    puts "EIBGetBusmonitorPacketTS_async()" if $DEBUG
    ibuf = [0] * 2;
    @ptr2 = status
    @ptr7 = timestamp
    @buf = buf
    @__complete = __EIBGetBusmonitorPacketTS_Complete()
    return 0
  end

  def EIBGetBusmonitorPacketTS(status, timestamp, buf)
    puts "EIBGetBusmonitorPacketTS()" if $DEBUG
    if EIBGetBusmonitorPacketTS_async(status, timestamp, buf) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBGetGroup_Src_Complete()
    puts "__EIBGetGroup_Src_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 39 or @data.length < 6
      @errno = Errno::ECONNRESET
      return -1
    end
    if @ptr5 != nil
      @ptr5.data = (((@data[2])<<8)|(@data[2+1]))
    end
    if @ptr6 != nil
      @ptr6.data = (((@data[4])<<8)|(@data[4+1]))
    end
    @buf.buffer = @data[6..-1]
    return @buf.buffer.length
  end


  def EIBGetGroup_Src_async(buf, src, dest)
    puts "EIBGetGroup_Src_async()" if $DEBUG
    ibuf = [0] * 2;
    @buf = buf
    @ptr5 = src
    @ptr6 = dest
    @__complete = __EIBGetGroup_Src_Complete()
    return 0
  end

  def EIBGetGroup_Src(buf, src, dest)
    puts "EIBGetGroup_Src()" if $DEBUG
    if EIBGetGroup_Src_async(buf, src, dest) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBGetTPDU_Complete()
    puts "__EIBGetTPDU_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 37 or @data.length < 4
      @errno = Errno::ECONNRESET
      return -1
    end
    if @ptr5 != nil
      @ptr5.data = (((@data[2])<<8)|(@data[2+1]))
    end
    @buf.buffer = @data[4..-1]
    return @buf.buffer.length
  end


  def EIBGetTPDU_async(buf, src)
    puts "EIBGetTPDU_async()" if $DEBUG
    ibuf = [0] * 2;
    @buf = buf
    @ptr5 = src
    @__complete = __EIBGetTPDU_Complete()
    return 0
  end

  def EIBGetTPDU(buf, src)
    puts "EIBGetTPDU()" if $DEBUG
    if EIBGetTPDU_async(buf, src) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_Cache_Clear_Complete()
    puts "__EIB_Cache_Clear_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 114 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIB_Cache_Clear_async()
    puts "EIB_Cache_Clear_async()" if $DEBUG
    ibuf = [0] * 2;
    ibuf[0] = 0
    ibuf[1] = 114
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_Cache_Clear_Complete()
    return 0
  end

  def EIB_Cache_Clear()
    puts "EIB_Cache_Clear()" if $DEBUG
    if EIB_Cache_Clear_async() == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_Cache_Disable_Complete()
    puts "__EIB_Cache_Disable_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 113 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIB_Cache_Disable_async()
    puts "EIB_Cache_Disable_async()" if $DEBUG
    ibuf = [0] * 2;
    ibuf[0] = 0
    ibuf[1] = 113
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_Cache_Disable_Complete()
    return 0
  end

  def EIB_Cache_Disable()
    puts "EIB_Cache_Disable()" if $DEBUG
    if EIB_Cache_Disable_async() == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_Cache_Enable_Complete()
    puts "__EIB_Cache_Enable_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 1
      @errno = Errno::EBUSY
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 112 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIB_Cache_Enable_async()
    puts "EIB_Cache_Enable_async()" if $DEBUG
    ibuf = [0] * 2;
    ibuf[0] = 0
    ibuf[1] = 112
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_Cache_Enable_Complete()
    return 0
  end

  def EIB_Cache_Enable()
    puts "EIB_Cache_Enable()" if $DEBUG
    if EIB_Cache_Enable_async() == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_Cache_Read_Complete()
    puts "__EIB_Cache_Read_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 117 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    if (((@data[4])<<8)|(@data[4+1])) == 0
      @errno = Errno::ENODEV
      return -1
    end
    if @data.length <= 6
      @errno = Errno::ENOENT
      return -1
    end
    if @ptr5 != nil
      @ptr5.data = (((@data[2])<<8)|(@data[2+1]))
    end
    @buf.buffer = @data[6..-1]
    return @buf.buffer.length
  end


  def EIB_Cache_Read_async(dst, src, buf)
    puts "EIB_Cache_Read_async()" if $DEBUG
    ibuf = [0] * 4;
    @buf = buf
    @ptr5 = src
    ibuf[2] = ((dst>>8)&0xff)
    ibuf[3] = ((dst)&0xff)
    ibuf[0] = 0
    ibuf[1] = 117
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_Cache_Read_Complete()
    return 0
  end

  def EIB_Cache_Read(dst, src, buf)
    puts "EIB_Cache_Read()" if $DEBUG
    if EIB_Cache_Read_async(dst, src, buf) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_Cache_Read_Sync_Complete()
    puts "__EIB_Cache_Read_Sync_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 116 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    if (((@data[4])<<8)|(@data[4+1])) == 0
      @errno = Errno::ENODEV
      return -1
    end
    if @data.length <= 6
      @errno = Errno::ENOENT
      return -1
    end
    if @ptr5 != nil
      @ptr5.data = (((@data[2])<<8)|(@data[2+1]))
    end
    @buf.buffer = @data[6..-1]
    return @buf.buffer.length
  end


  def EIB_Cache_Read_Sync_async(dst, src, buf, age)
    puts "EIB_Cache_Read_Sync_async()" if $DEBUG
    ibuf = [0] * 6;
    @buf = buf
    @ptr5 = src
    ibuf[2] = ((dst>>8)&0xff)
    ibuf[3] = ((dst)&0xff)
    ibuf[4] = ((age>>8)&0xff)
    ibuf[5] = ((age)&0xff)
    ibuf[0] = 0
    ibuf[1] = 116
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_Cache_Read_Sync_Complete()
    return 0
  end

  def EIB_Cache_Read_Sync(dst, src, buf, age)
    puts "EIB_Cache_Read_Sync()" if $DEBUG
    if EIB_Cache_Read_Sync_async(dst, src, buf, age) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_Cache_Remove_Complete()
    puts "__EIB_Cache_Remove_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 115 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIB_Cache_Remove_async(dest)
    puts "EIB_Cache_Remove_async()" if $DEBUG
    ibuf = [0] * 4;
    ibuf[2] = ((dest>>8)&0xff)
    ibuf[3] = ((dest)&0xff)
    ibuf[0] = 0
    ibuf[1] = 115
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_Cache_Remove_Complete()
    return 0
  end

  def EIB_Cache_Remove(dest)
    puts "EIB_Cache_Remove()" if $DEBUG
    if EIB_Cache_Remove_async(dest) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_Cache_LastUpdates_Complete()
    puts "__EIB_Cache_LastUpdates_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 118 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    if @ptr4 != nil
      @ptr4.data = (((@data[2])<<8)|(@data[2+1]))
    end
    @buf.buffer = @data[4..-1]
    return @buf.buffer.length
  end


  def EIB_Cache_LastUpdates_async(start, timeout, buf, ende)
    puts "EIB_Cache_LastUpdates_async()" if $DEBUG
    ibuf = [0] * 5;
    @buf = buf
    @ptr4 = ende
    ibuf[2] = ((start>>8)&0xff)
    ibuf[3] = ((start)&0xff)
    ibuf[4] = ((timeout)&0xff)
    ibuf[0] = 0
    ibuf[1] = 118
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_Cache_LastUpdates_Complete()
    return 0
  end

  def EIB_Cache_LastUpdates(start, timeout, buf, ende)
    puts "EIB_Cache_LastUpdates()" if $DEBUG
    if EIB_Cache_LastUpdates_async(start, timeout, buf, ende) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_LoadImage_Complete()
    puts "__EIB_LoadImage_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 99 or @data.length < 4
      @errno = Errno::ECONNRESET
      return -1
    end
    return (((@data[2])<<8)|(@data[2+1]))
  end


  def EIB_LoadImage_async(image)
    puts "EIB_LoadImage_async()" if $DEBUG
    ibuf = [0] * 2;
    if image.length < 0
      @errno = Errno::EINVAL
      return -1
    end
    @sendlen = image.length
    ibuf.concat(image)
    ibuf[0] = 0
    ibuf[1] = 99
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_LoadImage_Complete()
    return 0
  end

  def EIB_LoadImage(image)
    puts "EIB_LoadImage()" if $DEBUG
    if EIB_LoadImage_async(image) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_Authorize_Complete()
    puts "__EIB_MC_Authorize_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 87 or @data.length < 3
      @errno = Errno::ECONNRESET
      return -1
    end
    return @data[2]
  end


  def EIB_MC_Authorize_async(key)
    puts "EIB_MC_Authorize_async()" if $DEBUG
    ibuf = [0] * 6;
    if key.length != 4
      @errno = Errno::EINVAL
      return -1
    end
    ibuf[2..6] = key
    ibuf[0] = 0
    ibuf[1] = 87
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_Authorize_Complete()
    return 0
  end

  def EIB_MC_Authorize(key)
    puts "EIB_MC_Authorize()" if $DEBUG
    if EIB_MC_Authorize_async(key) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_Connect_Complete()
    puts "__EIB_MC_Connect_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 80 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIB_MC_Connect_async(dest)
    puts "EIB_MC_Connect_async()" if $DEBUG
    ibuf = [0] * 4;
    ibuf[2] = ((dest>>8)&0xff)
    ibuf[3] = ((dest)&0xff)
    ibuf[0] = 0
    ibuf[1] = 80
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_Connect_Complete()
    return 0
  end

  def EIB_MC_Connect(dest)
    puts "EIB_MC_Connect()" if $DEBUG
    if EIB_MC_Connect_async(dest) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_Individual_Open_Complete()
    puts "__EIB_MC_Individual_Open_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 73 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIB_MC_Individual_Open_async(dest)
    puts "EIB_MC_Individual_Open_async()" if $DEBUG
    ibuf = [0] * 4;
    ibuf[2] = ((dest>>8)&0xff)
    ibuf[3] = ((dest)&0xff)
    ibuf[0] = 0
    ibuf[1] = 73
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_Individual_Open_Complete()
    return 0
  end

  def EIB_MC_Individual_Open(dest)
    puts "EIB_MC_Individual_Open()" if $DEBUG
    if EIB_MC_Individual_Open_async(dest) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_GetMaskVersion_Complete()
    puts "__EIB_MC_GetMaskVersion_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 89 or @data.length < 4
      @errno = Errno::ECONNRESET
      return -1
    end
    return (((@data[2])<<8)|(@data[2+1]))
  end


  def EIB_MC_GetMaskVersion_async()
    puts "EIB_MC_GetMaskVersion_async()" if $DEBUG
    ibuf = [0] * 2;
    ibuf[0] = 0
    ibuf[1] = 89
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_GetMaskVersion_Complete()
    return 0
  end

  def EIB_MC_GetMaskVersion()
    puts "EIB_MC_GetMaskVersion()" if $DEBUG
    if EIB_MC_GetMaskVersion_async() == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_GetPEIType_Complete()
    puts "__EIB_MC_GetPEIType_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 85 or @data.length < 4
      @errno = Errno::ECONNRESET
      return -1
    end
    return (((@data[2])<<8)|(@data[2+1]))
  end


  def EIB_MC_GetPEIType_async()
    puts "EIB_MC_GetPEIType_async()" if $DEBUG
    ibuf = [0] * 2;
    ibuf[0] = 0
    ibuf[1] = 85
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_GetPEIType_Complete()
    return 0
  end

  def EIB_MC_GetPEIType()
    puts "EIB_MC_GetPEIType()" if $DEBUG
    if EIB_MC_GetPEIType_async() == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_Progmode_Off_Complete()
    puts "__EIB_MC_Progmode_Off_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 96 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIB_MC_Progmode_Off_async()
    puts "EIB_MC_Progmode_Off_async()" if $DEBUG
    ibuf = [0] * 3;
    ibuf[2] = ((0)&0xff)
    ibuf[0] = 0
    ibuf[1] = 96
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_Progmode_Off_Complete()
    return 0
  end

  def EIB_MC_Progmode_Off()
    puts "EIB_MC_Progmode_Off()" if $DEBUG
    if EIB_MC_Progmode_Off_async() == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_Progmode_On_Complete()
    puts "__EIB_MC_Progmode_On_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 96 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIB_MC_Progmode_On_async()
    puts "EIB_MC_Progmode_On_async()" if $DEBUG
    ibuf = [0] * 3;
    ibuf[2] = ((1)&0xff)
    ibuf[0] = 0
    ibuf[1] = 96
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_Progmode_On_Complete()
    return 0
  end

  def EIB_MC_Progmode_On()
    puts "EIB_MC_Progmode_On()" if $DEBUG
    if EIB_MC_Progmode_On_async() == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_Progmode_Status_Complete()
    puts "__EIB_MC_Progmode_Status_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 96 or @data.length < 3
      @errno = Errno::ECONNRESET
      return -1
    end
    return @data[2]
  end


  def EIB_MC_Progmode_Status_async()
    puts "EIB_MC_Progmode_Status_async()" if $DEBUG
    ibuf = [0] * 3;
    ibuf[2] = ((3)&0xff)
    ibuf[0] = 0
    ibuf[1] = 96
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_Progmode_Status_Complete()
    return 0
  end

  def EIB_MC_Progmode_Status()
    puts "EIB_MC_Progmode_Status()" if $DEBUG
    if EIB_MC_Progmode_Status_async() == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_Progmode_Toggle_Complete()
    puts "__EIB_MC_Progmode_Toggle_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 96 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIB_MC_Progmode_Toggle_async()
    puts "EIB_MC_Progmode_Toggle_async()" if $DEBUG
    ibuf = [0] * 3;
    ibuf[2] = ((2)&0xff)
    ibuf[0] = 0
    ibuf[1] = 96
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_Progmode_Toggle_Complete()
    return 0
  end

  def EIB_MC_Progmode_Toggle()
    puts "EIB_MC_Progmode_Toggle()" if $DEBUG
    if EIB_MC_Progmode_Toggle_async() == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_PropertyDesc_Complete()
    puts "__EIB_MC_PropertyDesc_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 97 or @data.length < 6
      @errno = Errno::ECONNRESET
      return -1
    end
    if @ptr2 != nil
      @ptr2.data = @data[2]
    end
    if @ptr4 != nil
      @ptr4.data = (((@data[3])<<8)|(@data[3+1]))
    end
    if @ptr3 != nil
      @ptr3.data = @data[5]
    end
    return 0
  end


  def EIB_MC_PropertyDesc_async(obj, propertyno, proptype, max_nr_of_elem, access)
    puts "EIB_MC_PropertyDesc_async()" if $DEBUG
    ibuf = [0] * 4;
    @ptr2 = proptype
    @ptr4 = max_nr_of_elem
    @ptr3 = access
    ibuf[2] = ((obj)&0xff)
    ibuf[3] = ((propertyno)&0xff)
    ibuf[0] = 0
    ibuf[1] = 97
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_PropertyDesc_Complete()
    return 0
  end

  def EIB_MC_PropertyDesc(obj, propertyno, proptype, max_nr_of_elem, access)
    puts "EIB_MC_PropertyDesc()" if $DEBUG
    if EIB_MC_PropertyDesc_async(obj, propertyno, proptype, max_nr_of_elem, access) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_PropertyRead_Complete()
    puts "__EIB_MC_PropertyRead_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 83 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    @buf.buffer = @data[2..-1]
    return @buf.buffer.length
  end


  def EIB_MC_PropertyRead_async(obj, propertyno, start, nr_of_elem, buf)
    puts "EIB_MC_PropertyRead_async()" if $DEBUG
    ibuf = [0] * 7;
    @buf = buf
    ibuf[2] = ((obj)&0xff)
    ibuf[3] = ((propertyno)&0xff)
    ibuf[4] = ((start>>8)&0xff)
    ibuf[5] = ((start)&0xff)
    ibuf[6] = ((nr_of_elem)&0xff)
    ibuf[0] = 0
    ibuf[1] = 83
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_PropertyRead_Complete()
    return 0
  end

  def EIB_MC_PropertyRead(obj, propertyno, start, nr_of_elem, buf)
    puts "EIB_MC_PropertyRead()" if $DEBUG
    if EIB_MC_PropertyRead_async(obj, propertyno, start, nr_of_elem, buf) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_PropertyScan_Complete()
    puts "__EIB_MC_PropertyScan_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 98 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    @buf.buffer = @data[2..-1]
    return @buf.buffer.length
  end


  def EIB_MC_PropertyScan_async(buf)
    puts "EIB_MC_PropertyScan_async()" if $DEBUG
    ibuf = [0] * 2;
    @buf = buf
    ibuf[0] = 0
    ibuf[1] = 98
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_PropertyScan_Complete()
    return 0
  end

  def EIB_MC_PropertyScan(buf)
    puts "EIB_MC_PropertyScan()" if $DEBUG
    if EIB_MC_PropertyScan_async(buf) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_PropertyWrite_Complete()
    puts "__EIB_MC_PropertyWrite_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 84 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    @buf.buffer = @data[2..-1]
    return @buf.buffer.length
  end


  def EIB_MC_PropertyWrite_async(obj, propertyno, start, nr_of_elem, buf, res)
    puts "EIB_MC_PropertyWrite_async()" if $DEBUG
    ibuf = [0] * 7;
    ibuf[2] = ((obj)&0xff)
    ibuf[3] = ((propertyno)&0xff)
    ibuf[4] = ((start>>8)&0xff)
    ibuf[5] = ((start)&0xff)
    ibuf[6] = ((nr_of_elem)&0xff)
    if buf.length < 0
      @errno = Errno::EINVAL
      return -1
    end
    @sendlen = buf.length
    ibuf.concat(buf)
    @buf = res
    ibuf[0] = 0
    ibuf[1] = 84
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_PropertyWrite_Complete()
    return 0
  end

  def EIB_MC_PropertyWrite(obj, propertyno, start, nr_of_elem, buf, res)
    puts "EIB_MC_PropertyWrite()" if $DEBUG
    if EIB_MC_PropertyWrite_async(obj, propertyno, start, nr_of_elem, buf, res) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_ReadADC_Complete()
    puts "__EIB_MC_ReadADC_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 86 or @data.length < 4
      @errno = Errno::ECONNRESET
      return -1
    end
    if @ptr1 != nil
      @ptr1.data = (((@data[2])<<8)|(@data[2+1]))
    end
    return 0
  end


  def EIB_MC_ReadADC_async(channel, count, val)
    puts "EIB_MC_ReadADC_async()" if $DEBUG
    ibuf = [0] * 4;
    @ptr1 = val
    ibuf[2] = ((channel)&0xff)
    ibuf[3] = ((count)&0xff)
    ibuf[0] = 0
    ibuf[1] = 86
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_ReadADC_Complete()
    return 0
  end

  def EIB_MC_ReadADC(channel, count, val)
    puts "EIB_MC_ReadADC()" if $DEBUG
    if EIB_MC_ReadADC_async(channel, count, val) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_Read_Complete()
    puts "__EIB_MC_Read_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 81 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    @buf.buffer = @data[2..-1]
    return @buf.buffer.length
  end


  def EIB_MC_Read_async(addr, buf_len, buf)
    puts "EIB_MC_Read_async()" if $DEBUG
    ibuf = [0] * 6;
    @buf = buf
    ibuf[2] = ((addr>>8)&0xff)
    ibuf[3] = ((addr)&0xff)
    ibuf[4] = ((buf_len>>8)&0xff)
    ibuf[5] = ((buf_len)&0xff)
    ibuf[0] = 0
    ibuf[1] = 81
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_Read_Complete()
    return 0
  end

  def EIB_MC_Read(addr, buf_len, buf)
    puts "EIB_MC_Read()" if $DEBUG
    if EIB_MC_Read_async(addr, buf_len, buf) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_Restart_Complete()
    puts "__EIB_MC_Restart_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 90 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIB_MC_Restart_async()
    puts "EIB_MC_Restart_async()" if $DEBUG
    ibuf = [0] * 2;
    ibuf[0] = 0
    ibuf[1] = 90
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_Restart_Complete()
    return 0
  end

  def EIB_MC_Restart()
    puts "EIB_MC_Restart()" if $DEBUG
    if EIB_MC_Restart_async() == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_SetKey_Complete()
    puts "__EIB_MC_SetKey_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 2
      @errno = Errno::EPERM
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 88 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIB_MC_SetKey_async(key, level)
    puts "EIB_MC_SetKey_async()" if $DEBUG
    ibuf = [0] * 7;
    if key.length != 4
      @errno = Errno::EINVAL
      return -1
    end
    ibuf[2..6] = key
    ibuf[6] = ((level)&0xff)
    ibuf[0] = 0
    ibuf[1] = 88
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_SetKey_Complete()
    return 0
  end

  def EIB_MC_SetKey(key, level)
    puts "EIB_MC_SetKey()" if $DEBUG
    if EIB_MC_SetKey_async(key, level) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_Write_Complete()
    puts "__EIB_MC_Write_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 68
      @errno = Errno::EIO
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 82 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return @sendlen
  end


  def EIB_MC_Write_async(addr, buf)
    puts "EIB_MC_Write_async()" if $DEBUG
    ibuf = [0] * 6;
    ibuf[2] = ((addr>>8)&0xff)
    ibuf[3] = ((addr)&0xff)
    ibuf[4] = (((buf.length)>>8)&0xff)
    ibuf[5] = (((buf.length))&0xff)
    if buf.length < 0
      @errno = Errno::EINVAL
      return -1
    end
    @sendlen = buf.length
    ibuf.concat(buf)
    ibuf[0] = 0
    ibuf[1] = 82
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_Write_Complete()
    return 0
  end

  def EIB_MC_Write(addr, buf)
    puts "EIB_MC_Write()" if $DEBUG
    if EIB_MC_Write_async(addr, buf) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_MC_Write_Plain_Complete()
    puts "__EIB_MC_Write_Plain_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 91 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return @sendlen
  end


  def EIB_MC_Write_Plain_async(addr, buf)
    puts "EIB_MC_Write_Plain_async()" if $DEBUG
    ibuf = [0] * 6;
    ibuf[2] = ((addr>>8)&0xff)
    ibuf[3] = ((addr)&0xff)
    ibuf[4] = (((buf.length)>>8)&0xff)
    ibuf[5] = (((buf.length))&0xff)
    if buf.length < 0
      @errno = Errno::EINVAL
      return -1
    end
    @sendlen = buf.length
    ibuf.concat(buf)
    ibuf[0] = 0
    ibuf[1] = 91
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_MC_Write_Plain_Complete()
    return 0
  end

  def EIB_MC_Write_Plain(addr, buf)
    puts "EIB_MC_Write_Plain()" if $DEBUG
    if EIB_MC_Write_Plain_async(addr, buf) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_M_GetMaskVersion_Complete()
    puts "__EIB_M_GetMaskVersion_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 49 or @data.length < 4
      @errno = Errno::ECONNRESET
      return -1
    end
    return (((@data[2])<<8)|(@data[2+1]))
  end


  def EIB_M_GetMaskVersion_async(dest)
    puts "EIB_M_GetMaskVersion_async()" if $DEBUG
    ibuf = [0] * 4;
    ibuf[2] = ((dest>>8)&0xff)
    ibuf[3] = ((dest)&0xff)
    ibuf[0] = 0
    ibuf[1] = 49
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_M_GetMaskVersion_Complete()
    return 0
  end

  def EIB_M_GetMaskVersion(dest)
    puts "EIB_M_GetMaskVersion()" if $DEBUG
    if EIB_M_GetMaskVersion_async(dest) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_M_Progmode_Off_Complete()
    puts "__EIB_M_Progmode_Off_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 48 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIB_M_Progmode_Off_async(dest)
    puts "EIB_M_Progmode_Off_async()" if $DEBUG
    ibuf = [0] * 5;
    ibuf[2] = ((dest>>8)&0xff)
    ibuf[3] = ((dest)&0xff)
    ibuf[4] = ((0)&0xff)
    ibuf[0] = 0
    ibuf[1] = 48
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_M_Progmode_Off_Complete()
    return 0
  end

  def EIB_M_Progmode_Off(dest)
    puts "EIB_M_Progmode_Off()" if $DEBUG
    if EIB_M_Progmode_Off_async(dest) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_M_Progmode_On_Complete()
    puts "__EIB_M_Progmode_On_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 48 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIB_M_Progmode_On_async(dest)
    puts "EIB_M_Progmode_On_async()" if $DEBUG
    ibuf = [0] * 5;
    ibuf[2] = ((dest>>8)&0xff)
    ibuf[3] = ((dest)&0xff)
    ibuf[4] = ((1)&0xff)
    ibuf[0] = 0
    ibuf[1] = 48
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_M_Progmode_On_Complete()
    return 0
  end

  def EIB_M_Progmode_On(dest)
    puts "EIB_M_Progmode_On()" if $DEBUG
    if EIB_M_Progmode_On_async(dest) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_M_Progmode_Status_Complete()
    puts "__EIB_M_Progmode_Status_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 48 or @data.length < 3
      @errno = Errno::ECONNRESET
      return -1
    end
    return @data[2]
  end


  def EIB_M_Progmode_Status_async(dest)
    puts "EIB_M_Progmode_Status_async()" if $DEBUG
    ibuf = [0] * 5;
    ibuf[2] = ((dest>>8)&0xff)
    ibuf[3] = ((dest)&0xff)
    ibuf[4] = ((3)&0xff)
    ibuf[0] = 0
    ibuf[1] = 48
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_M_Progmode_Status_Complete()
    return 0
  end

  def EIB_M_Progmode_Status(dest)
    puts "EIB_M_Progmode_Status()" if $DEBUG
    if EIB_M_Progmode_Status_async(dest) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_M_Progmode_Toggle_Complete()
    puts "__EIB_M_Progmode_Toggle_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 48 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIB_M_Progmode_Toggle_async(dest)
    puts "EIB_M_Progmode_Toggle_async()" if $DEBUG
    ibuf = [0] * 5;
    ibuf[2] = ((dest>>8)&0xff)
    ibuf[3] = ((dest)&0xff)
    ibuf[4] = ((2)&0xff)
    ibuf[0] = 0
    ibuf[1] = 48
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_M_Progmode_Toggle_Complete()
    return 0
  end

  def EIB_M_Progmode_Toggle(dest)
    puts "EIB_M_Progmode_Toggle()" if $DEBUG
    if EIB_M_Progmode_Toggle_async(dest) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_M_ReadIndividualAddresses_Complete()
    puts "__EIB_M_ReadIndividualAddresses_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 50 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    @buf.buffer = @data[2..-1]
    return @buf.buffer.length
  end


  def EIB_M_ReadIndividualAddresses_async(buf)
    puts "EIB_M_ReadIndividualAddresses_async()" if $DEBUG
    ibuf = [0] * 2;
    @buf = buf
    ibuf[0] = 0
    ibuf[1] = 50
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_M_ReadIndividualAddresses_Complete()
    return 0
  end

  def EIB_M_ReadIndividualAddresses(buf)
    puts "EIB_M_ReadIndividualAddresses()" if $DEBUG
    if EIB_M_ReadIndividualAddresses_async(buf) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIB_M_WriteIndividualAddress_Complete()
    puts "__EIB_M_WriteIndividualAddress_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 65
      @errno = Errno::EADDRINUSE
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 67
      @errno = Errno::ETIMEDOUT
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 66
      @errno = Errno::EADDRNOTAVAIL
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 64 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIB_M_WriteIndividualAddress_async(dest)
    puts "EIB_M_WriteIndividualAddress_async()" if $DEBUG
    ibuf = [0] * 4;
    ibuf[2] = ((dest>>8)&0xff)
    ibuf[3] = ((dest)&0xff)
    ibuf[0] = 0
    ibuf[1] = 64
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIB_M_WriteIndividualAddress_Complete()
    return 0
  end

  def EIB_M_WriteIndividualAddress(dest)
    puts "EIB_M_WriteIndividualAddress()" if $DEBUG
    if EIB_M_WriteIndividualAddress_async(dest) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBOpenBusmonitor_Complete()
    puts "__EIBOpenBusmonitor_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 1
      @errno = Errno::EBUSY
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 16 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIBOpenBusmonitor_async()
    puts "EIBOpenBusmonitor_async()" if $DEBUG
    ibuf = [0] * 2;
    ibuf[0] = 0
    ibuf[1] = 16
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIBOpenBusmonitor_Complete()
    return 0
  end

  def EIBOpenBusmonitor()
    puts "EIBOpenBusmonitor()" if $DEBUG
    if EIBOpenBusmonitor_async() == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBOpenBusmonitorText_Complete()
    puts "__EIBOpenBusmonitorText_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 1
      @errno = Errno::EBUSY
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 17 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIBOpenBusmonitorText_async()
    puts "EIBOpenBusmonitorText_async()" if $DEBUG
    ibuf = [0] * 2;
    ibuf[0] = 0
    ibuf[1] = 17
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIBOpenBusmonitorText_Complete()
    return 0
  end

  def EIBOpenBusmonitorText()
    puts "EIBOpenBusmonitorText()" if $DEBUG
    if EIBOpenBusmonitorText_async() == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBOpenBusmonitorTS_Complete()
    puts "__EIBOpenBusmonitorTS_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 1
      @errno = Errno::EBUSY
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 22 or @data.length < 6
      @errno = Errno::ECONNRESET
      return -1
    end
    if @ptr7 != nil
      @ptr7.data = (((@data[2])<<24)|((@data[2+1])<<16)|((@data[2+2])<<8)|(@data[2+3]))
    end
    return 0
  end


  def EIBOpenBusmonitorTS_async(timebase)
    puts "EIBOpenBusmonitorTS_async()" if $DEBUG
    ibuf = [0] * 2;
    @ptr7 = timebase
    ibuf[0] = 0
    ibuf[1] = 22
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIBOpenBusmonitorTS_Complete()
    return 0
  end

  def EIBOpenBusmonitorTS(timebase)
    puts "EIBOpenBusmonitorTS()" if $DEBUG
    if EIBOpenBusmonitorTS_async(timebase) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBOpen_GroupSocket_Complete()
    puts "__EIBOpen_GroupSocket_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 38 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIBOpen_GroupSocket_async(write_only)
    puts "EIBOpen_GroupSocket_async()" if $DEBUG
    ibuf = [0] * 5;
    if write_only != 0
      ibuf[4] = 0xff
    else
      ibuf[4] = 0x00
    end
    ibuf[0] = 0
    ibuf[1] = 38
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIBOpen_GroupSocket_Complete()
    return 0
  end

  def EIBOpen_GroupSocket(write_only)
    puts "EIBOpen_GroupSocket()" if $DEBUG
    if EIBOpen_GroupSocket_async(write_only) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBOpenT_Broadcast_Complete()
    puts "__EIBOpenT_Broadcast_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 35 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIBOpenT_Broadcast_async(write_only)
    puts "EIBOpenT_Broadcast_async()" if $DEBUG
    ibuf = [0] * 5;
    if write_only != 0
      ibuf[4] = 0xff
    else
      ibuf[4] = 0x00
    end
    ibuf[0] = 0
    ibuf[1] = 35
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIBOpenT_Broadcast_Complete()
    return 0
  end

  def EIBOpenT_Broadcast(write_only)
    puts "EIBOpenT_Broadcast()" if $DEBUG
    if EIBOpenT_Broadcast_async(write_only) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBOpenT_Connection_Complete()
    puts "__EIBOpenT_Connection_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 32 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIBOpenT_Connection_async(dest)
    puts "EIBOpenT_Connection_async()" if $DEBUG
    ibuf = [0] * 5;
    ibuf[2] = ((dest>>8)&0xff)
    ibuf[3] = ((dest)&0xff)
    ibuf[0] = 0
    ibuf[1] = 32
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIBOpenT_Connection_Complete()
    return 0
  end

  def EIBOpenT_Connection(dest)
    puts "EIBOpenT_Connection()" if $DEBUG
    if EIBOpenT_Connection_async(dest) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBOpenT_Group_Complete()
    puts "__EIBOpenT_Group_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 34 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIBOpenT_Group_async(dest, write_only)
    puts "EIBOpenT_Group_async()" if $DEBUG
    ibuf = [0] * 5;
    ibuf[2] = ((dest>>8)&0xff)
    ibuf[3] = ((dest)&0xff)
    if write_only != 0
      ibuf[4] = 0xff
    else
      ibuf[4] = 0x00
    end
    ibuf[0] = 0
    ibuf[1] = 34
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIBOpenT_Group_Complete()
    return 0
  end

  def EIBOpenT_Group(dest, write_only)
    puts "EIBOpenT_Group()" if $DEBUG
    if EIBOpenT_Group_async(dest, write_only) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBOpenT_Individual_Complete()
    puts "__EIBOpenT_Individual_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 33 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIBOpenT_Individual_async(dest, write_only)
    puts "EIBOpenT_Individual_async()" if $DEBUG
    ibuf = [0] * 5;
    ibuf[2] = ((dest>>8)&0xff)
    ibuf[3] = ((dest)&0xff)
    if write_only != 0
      ibuf[4] = 0xff
    else
      ibuf[4] = 0x00
    end
    ibuf[0] = 0
    ibuf[1] = 33
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIBOpenT_Individual_Complete()
    return 0
  end

  def EIBOpenT_Individual(dest, write_only)
    puts "EIBOpenT_Individual()" if $DEBUG
    if EIBOpenT_Individual_async(dest, write_only) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBOpenT_TPDU_Complete()
    puts "__EIBOpenT_TPDU_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 36 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIBOpenT_TPDU_async(src)
    puts "EIBOpenT_TPDU_async()" if $DEBUG
    ibuf = [0] * 5;
    ibuf[2] = ((src>>8)&0xff)
    ibuf[3] = ((src)&0xff)
    ibuf[0] = 0
    ibuf[1] = 36
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIBOpenT_TPDU_Complete()
    return 0
  end

  def EIBOpenT_TPDU(src)
    puts "EIBOpenT_TPDU()" if $DEBUG
    if EIBOpenT_TPDU_async(src) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBOpenVBusmonitor_Complete()
    puts "__EIBOpenVBusmonitor_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 1
      @errno = Errno::EBUSY
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 18 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIBOpenVBusmonitor_async()
    puts "EIBOpenVBusmonitor_async()" if $DEBUG
    ibuf = [0] * 2;
    ibuf[0] = 0
    ibuf[1] = 18
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIBOpenVBusmonitor_Complete()
    return 0
  end

  def EIBOpenVBusmonitor()
    puts "EIBOpenVBusmonitor()" if $DEBUG
    if EIBOpenVBusmonitor_async() == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBOpenVBusmonitorText_Complete()
    puts "__EIBOpenVBusmonitorText_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 1
      @errno = Errno::EBUSY
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 19 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIBOpenVBusmonitorText_async()
    puts "EIBOpenVBusmonitorText_async()" if $DEBUG
    ibuf = [0] * 2;
    ibuf[0] = 0
    ibuf[1] = 19
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIBOpenVBusmonitorText_Complete()
    return 0
  end

  def EIBOpenVBusmonitorText()
    puts "EIBOpenVBusmonitorText()" if $DEBUG
    if EIBOpenVBusmonitorText_async() == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBOpenVBusmonitorTS_Complete()
    puts "__EIBOpenVBusmonitorTS_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 1
      @errno = Errno::EBUSY
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 23 or @data.length < 6
      @errno = Errno::ECONNRESET
      return -1
    end
    if @ptr7 != nil
      @ptr7.data = (((@data[2])<<24)|((@data[2+1])<<16)|((@data[2+2])<<8)|(@data[2+3]))
    end
    return 0
  end


  def EIBOpenVBusmonitorTS_async(timebase)
    puts "EIBOpenVBusmonitorTS_async()" if $DEBUG
    ibuf = [0] * 2;
    @ptr7 = timebase
    ibuf[0] = 0
    ibuf[1] = 23
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIBOpenVBusmonitorTS_Complete()
    return 0
  end

  def EIBOpenVBusmonitorTS(timebase)
    puts "EIBOpenVBusmonitorTS()" if $DEBUG
    if EIBOpenVBusmonitorTS_async(timebase) == -1
      return -1
    end
    return EIBComplete()
  end

  def __EIBReset_Complete()
    puts "__EIBReset_Complete()" if $DEBUG
    @__complete = nil
    if __EIB_GetRequest() == -1
      return -1
    end
    if (((@data[0])<<8)|(@data[0+1])) != 4 or @data.length < 2
      @errno = Errno::ECONNRESET
      return -1
    end
    return 0
  end


  def EIBReset_async()
    puts "EIBReset_async()" if $DEBUG
    ibuf = [0] * 2;
    ibuf[0] = 0
    ibuf[1] = 4
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    @__complete = __EIBReset_Complete()
    return 0
  end

  def EIBReset()
    puts "EIBReset()" if $DEBUG
    if EIBReset_async() == -1
      return -1
    end
    return EIBComplete()
  end

  def EIBSendAPDU(data)
    puts "EIBSendAPDU()" if $DEBUG
    ibuf = [0] * 2;
    if data.length < 2
      @errno = Errno::EINVAL
      return -1
    end
    @sendlen = data.length
    ibuf.concat(data)
    ibuf[0] = 0
    ibuf[1] = 37
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    return @sendlen

  end

  def EIBSendGroup(dest, data)
    puts "EIBSendGroup()" if $DEBUG
    ibuf = [0] * 4;
    ibuf[2] = ((dest>>8)&0xff)
    ibuf[3] = ((dest)&0xff)
    if data.length < 2
      @errno = Errno::EINVAL
      return -1
    end
    @sendlen = data.length
    ibuf.concat(data)
    ibuf[0] = 0
    ibuf[1] = 39
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    return @sendlen

  end

  def EIBSendTPDU(dest, data)
    puts "EIBSendTPDU()" if $DEBUG
    ibuf = [0] * 4;
    ibuf[2] = ((dest>>8)&0xff)
    ibuf[3] = ((dest)&0xff)
    if data.length < 2
      @errno = Errno::EINVAL
      return -1
    end
    @sendlen = data.length
    ibuf.concat(data)
    ibuf[0] = 0
    ibuf[1] = 37
    md = caller[0].match(/`(\w*)'/); clr = md and md[1] or caller[0]
    puts("#{clr} calling __EIB_SendRequest, ibuf=#{ibuf.inspect}") if $DEBUG
    if __EIB_SendRequest(ibuf) == -1
      return -1
    end
    return @sendlen

  end

IMG_UNKNOWN_ERROR = 0
IMG_UNRECOG_FORMAT = 1
IMG_INVALID_FORMAT = 2
IMG_NO_BCUTYPE = 3
IMG_UNKNOWN_BCUTYPE = 4
IMG_NO_CODE = 5
IMG_NO_SIZE = 6
IMG_LODATA_OVERFLOW = 7
IMG_HIDATA_OVERFLOW = 8
IMG_TEXT_OVERFLOW = 9
IMG_NO_ADDRESS = 10
IMG_WRONG_SIZE = 11
IMG_IMAGE_LOADABLE = 12
IMG_NO_DEVICE_CONNECTION = 13
IMG_MASK_READ_FAILED = 14
IMG_WRONG_MASK_VERSION = 15
IMG_CLEAR_ERROR = 16
IMG_RESET_ADDR_TAB = 17
IMG_LOAD_HEADER = 18
IMG_LOAD_MAIN = 19
IMG_ZERO_RAM = 20
IMG_FINALIZE_ADDR_TAB = 21
IMG_PREPARE_RUN = 22
IMG_RESTART = 23
IMG_LOADED = 24
IMG_NO_START = 25
IMG_WRONG_ADDRTAB = 26
IMG_ADDRTAB_OVERFLOW = 27
IMG_OVERLAP_ASSOCTAB = 28
IMG_OVERLAP_TEXT = 29
IMG_NEGATIV_TEXT_SIZE = 30
IMG_OVERLAP_PARAM = 31
IMG_OVERLAP_EEPROM = 32
IMG_OBJTAB_OVERFLOW = 33
IMG_WRONG_LOADCTL = 34
IMG_UNLOAD_ADDR = 35
IMG_UNLOAD_ASSOC = 36
IMG_UNLOAD_PROG = 37
IMG_LOAD_ADDR = 38
IMG_WRITE_ADDR = 39
IMG_SET_ADDR = 40
IMG_FINISH_ADDR = 41
IMG_LOAD_ASSOC = 42
IMG_WRITE_ASSOC = 43
IMG_SET_ASSOC = 44
IMG_FINISH_ASSOC = 45
IMG_LOAD_PROG = 46
IMG_ALLOC_LORAM = 47
IMG_ALLOC_HIRAM = 48
IMG_ALLOC_INIT = 49
IMG_ALLOC_RO = 50
IMG_ALLOC_EEPROM = 51
IMG_ALLOC_PARAM = 52
IMG_SET_PROG = 53
IMG_SET_TASK_PTR = 54
IMG_SET_OBJ = 55
IMG_SET_TASK2 = 56
IMG_FINISH_PROC = 57
IMG_WRONG_CHECKLIM = 58
IMG_INVALID_KEY = 59
IMG_AUTHORIZATION_FAILED = 60
IMG_KEY_WRITE = 61
end #class EIBConnection