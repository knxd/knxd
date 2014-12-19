require ("EIBConnection")
require ("bit")

-- addr2str: Convert an integer to an EIB address string, in the form "1/2/3" or "1.2.3"
function addr2str(a, ga)
	str = ""
	if (ga == 0) then
		str = string.format("%d.%d.%d", bit.rshift(a, 12), bit.band(bit.rshift(a, 8), 0xf), bit.band(a, 0xff))
	else
		str = string.format("%d/%d/%d", bit.band(bit.rshift(a, 11), 0xf), bit.band(bit.rshift(a, 8), 0x7), bit.band(a, 0xff))
	end
	return(str)
end

-- str2addr: Parse an address string into an unsigned 16-bit integer
function str2addr(s)
	a, b, c  = s:match("(%d+)[^%d]*(%d+)[^%d]*(%d+)")
	if (c) then
		--a, b, c = int(a), int(b), int(c)
		-- (a & 0x01f) << 11) | ((b & 0x07) << 8) | ((c & 0xff)
		op1 = bit.lshift(bit.band(a, 0x1f), 11) 
		op2 = bit.lshift(bit.band(b, 0x07), 8)
		op3 = bit.band(c, 0xff) 
		return (bit.bor(op1, bit.bor(op2, op3)))
	end
	a, b = s:match("(\d*)\/(\d*)")
	if (b) then
		--a,b = int(a), int(b)
		-- (a & 0x01f) << 11) | ((b & 0x7FF)
		op1 = bit.lshift(b.band(a, 0x1f), 11)
		op2 = bit.band(b, 0x7ff)
		return (bit.bor(op1, op2))
	end
	if s:find("^0x") then
		return (bit.band(int(s), 0xffff))
	end
end

DEBUG = false
conn = EIBConnection:new()
conn_id = conn:EIBSocketURL("ip:192.168.0.10")
print("conn=", conn)

buf = EIBBuffer:new()
--puts(buf.inspect)

dest = str2addr("1/2/0")
print(dest)
if (conn:EIBOpenT_Group(dest, 1) == -1) then	
	print("KNX client: error setting socket mode")
	print(conn)
	os.exit(1)
end

--data = [0, 0x80| val]
val=0

data={}
data[1] = 0
data[2] = bit.bor(0x0080, val)
print("SendAPDU result=", conn:EIBSendAPDU(data))
conn:EIBReset()
