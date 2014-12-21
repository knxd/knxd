--[[ 
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
]]

--import errno;
--import socket;
require("socket");
require("bit");

EIBBuffer = {};
function EIBBuffer.new(self, buf)
    self = self or {}
    self.buffer = buf or {};
    return self
end

EIBAddr = {};
function EIBAddr.new(self, value)
	error "FIXME"
    self.data = value or 0
end

EIBInt8 = {};
function EIBInt8.new(self, value)
	error "FIXME"
    self.data = value or 0
end

EIBInt16 = {};
function EIBInt16.new(self, value)
	error "FIXME"
    self.data = value or 0
end

EIBInt32 = {};
function EIBInt32.new(self, value)
	error "FIXME"
    self.data = value or 0
    return self
end

EIBConnection = {};
function EIBConnection:new(o)
print ("New EIBConnection")
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    self.data = {}
    self.readlen = 0
    self.datalen = 0
    self.fd = None
    self.errno = 0
    self.__complete = nil
    return o
end

function EIBConnection.EIBSocketLocal(self, path)
    if (self.fd ~= nil) then
      self.errno = "EUSERS"
      return -1
    end
    fd = socket.unix()
    fd:connect(path)
    self.data = {}
    self.readlen = 0
    self.fd = fd
    return 0
end

function EIBConnection.EIBSocketRemote(self, host, port)
    if (self.fd ~= nil) then
      self.errno = "EUSERS"
      return -1
  end
  port = port or 6720
  fd = socket.tcp()
  -- convert host name to ip address

  --FIXME: resolve hostname if not an IP address already
  -- local host = socket.dns.toip(host)
	hostip = host
	print("hostip=",hostip)
  fd:connect(hostip, port);
  self.data = {}
  self.readlen = 0
  self.fd = fd
  return 0
end

function EIBConnection.EIBSocketURL(self, url)
if DEBUG then print(url) end
    if (url:sub(0,6) == 'local:') then
      return self.EIBSocketLocal(url:sub(6))
    end
    if (url:sub(0,3) == 'ip:') then 
------ parts=url.split(':')
      hostport = url:sub(4,-1)
      host, port = nil, nil
      offset = hostport:find(":")
      if (offset) then
	  host = hostport:sub(1, offset)
	  port = tonumber(hostport:sub(offset+1))
	else
	  host = hostport
          port = 6720
      end
	print("host=",host," port=",port)
      return self:EIBSocketRemote(host, port)
    end
    self.errno = "EINVAL"
    return -1
end
  
function EIBConnection.EIBComplete(self)
  if DEBUG then   print("Entering EIBConnection.EIBComplete()") end
    if self.__complete == nil then
      self.errno = "EINVAL"
      return -1
    end
    return self.__complete
end
  
function EIBConnection.EIBClose(self)
  if DEBUG then print("Entering EIBConnection.EIBClose()") end
    if self.fd == nil then
      self.errno = "EINVAL"
      return -1
    end
    self.fd:close()
    self.fd = nil
end

function EIBConnection.EIBClose_sync(self)
  if DEBUG then print("Entering EIBConnection.EIBClose_sync()") end
    self.EIBReset()
    return self.EIBClose()
end

function EIBConnection.__EIB_SendRequest(self, data)
-- EKARAK: MUST DECLARE ALL LOCAL VARIABLES, OR ELSE THEY POLLUTE GLOBAL NAMESPACE
  local result, errmsg, data2
  if DEBUG then print("Entering EIBConnection.__EIB_SendRequest()") end
  --print(dump(data))
    if type(data) == "table" then
	result = ""
        for i = 1, #data do
            result = result .. string.char(data[i])
        end
        data = result
	--data = table.concat(data)
    end
    if self.fd == nil then
      self.errno = "ECONNRESET"
      return -1
    end
    if (#(data) < 2 or #(data) > 0xffff) then
      self.errno = "EINVAL"
      return -1
    end
    -- data = [ (len(data)>>8)&0xff, (len(data))&0xff ] + data
    data2 = string.char( bit.band ( bit.rshift(#data,8) , 0xff )) .. string.char( bit.band (#(data), 0xff) ) .. data
    for i=1,#data2 do self.data[i] = data2:byte(i) end
	if DEBUG then print("EIBConnection.__EIB_SendRequest() sending:", hex_dump(data2)) end
    result, errmsg = self.fd:send(data2)
	if not result then
		error("connection error: "..errmsg)
		os.exit()
	end
    return 0
end
  
  
function EIBConnection.EIB_Poll_FD(self)
  if DEBUG then print("Entering EIBConnection.EIB_Poll_FD()") end
    if self.fd == nil then
      self.errno = "EINVAL"
      return -1
    end
    return self.fd
end
  
function EIBConnection.EIB_Poll_Complete(self)
  if DEBUG then print("Entering EIBConnection.EIB_Poll_Complete()") end
    if self.__EIB_CheckRequest(false) == -1 then
      return -1
    end
    if self.readlen < 2 or (self.readlen >= 2 and self.readlen < self.datalen + 2) then
      return 0
    end
    return 1
end
  
function EIBConnection.__EIB_GetRequest(self)
  if DEBUG then   print("Entering EIBConnection.__EIB_GetRequest()") end
     while true do
      if self:__EIB_CheckRequest(true) == -1 then
        return -1
      end
	if DEBUG then print(string.format("__EIB_GetRequest, self.readlen=%d self.datalen=%d", self.readlen, self.datalen)) end
      if (self.readlen >= 2) and (self.readlen >= self.datalen + 2) then
        self.readlen = 0
        return 0
      end
    end
end

function EIBConnection.__EIB_CheckRequest(self, block)
-- EKARAK: MUST DECLARE ALL LOCAL VARIABLES, OR ELSE THEY POLLUTE GLOBAL NAMESPACE
  local result, errmsg
--
  if DEBUG then print("Entering EIBConnection.__EIB_CheckRequest()") end
    if self.fd == nil then
      self.errno = "ECONNRESET"
      return -1
    end
    if self.readlen == 0 then
      self.head = {}
      self.data = {}
    end
    if self.readlen < 2 then
--      self.fd:setblocking(block)
	if block then
	  self.fd:settimeout(1)
	else 
	  self.fd:settimeout(3)
	end
      result, errmsg = self.fd:receive(2-self.readlen)
	if result then
		if DEBUG then print(string.format("received %d bytes: %s", #result, hex_dump(result))) end
		oldlen = #(self.head)
		self.readlen = self.readlen + #result -- yucks yucks YUCKS
		--print("self.readlen == ", self.readlen, "type(#result)==",type(#result))		
		for i=1,#result do table.insert(self.head, result:byte(i)) end
		--print("self.head dump:", hex_dump(self.head))
	else 
		print("ERROR receing: errmsg=", errmsg)
		os.exit()
	end
    end
    if self.readlen < 2 then
      return 0
    end
    -----self.datalen = (self.head[0] << 8) | self.head[1]
    self.datalen = bit.bor( bit.lshift(self.head[1] , 8),  self.head[2])

    if DEBUG then print(string.format("__EIB_CheckRequest stage 2, self.readlen=%d self.datalen=%d", self.readlen, self.datalen)) end
    if (self.readlen < self.datalen + 2) then
	if block then
	  self.fd:settimeout(1)
	else 
	  self.fd:settimeout(3)
	end
      result, errmsg = self.fd:receive(self.datalen + 2 -self.readlen)
	if result then
		if DEBUG then print(string.format("received %d bytes: %s", #result, hex_dump(result))) end
		oldlen = #(self.data)
		self.readlen = self.readlen + #result -- yucks yucks YUCKS
		-- ekarak: YUCKS! 
		for i=1,#result do 
			table.insert(self.data, result:byte(i)) 
		end
	else 
		print("ERROR receing: errmsg=", errmsg)
		os.exit()
	end
    end
    return 0      
end


-- HELPER FUNCTIONS
function hex_dump(buf)
    result = ""
    for i=1,math.ceil(#buf/16) * 16 do
     	if (i-1) % 16 == 0 then result = result..(string.format('%08X  ', i-1)) end
	foo, bar = "", ""
	if type(buf) == "string" then 
		foo = buf:byte(i)
		bar = buf:sub(i-16+1, i):gsub('%c','.')
	elseif type(buf) == "table" then 
		foo = buf[i]
		bar = table.concat(buf):sub(i-16+1, i):gsub('%c','.')
	end
        result = result..( i > #buf and '   ' or string.format('%02X ', foo) )
        if i %  8 == 0 then result = result..(' ') end
        if i % 16 == 0 then result = result..bar..'\n' end
     end
     return result
end

