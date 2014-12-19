require 'EIBConnection'


#########################################################

APCICODES = "A_GroupValue_Read A_GroupValue_Response A_GroupValue_Write \
        A_PhysicalAddress_Write A_PhysicalAddress_Read A_PhysicalAddress_Response \
        A_ADC_Read A_ADC_Response A_Memory_Read A_Memory_Response A_Memory_Write \
        A_UserMemory A_DeviceDescriptor_Read A_DeviceDescriptor_Response A_Restart \
        A_OTHER".split()

TPDUCODES = "T_DATA_XXX_REQ T_DATA_CONNECTED_REQ T_DISCONNECT_REQ T_ACK".split()

PRIOCLASSES = "system alarm high low".split()

#########################################################

# addr2str: Convert an integer to an EIB address string, in the form "1/2/3" or "1.2.3"
def addr2str(a, ga=false)
	str=""
	if (ga)
		str = "#{(a >> 11) & 0xf}/#{(a >> 8) & 0x7}/#{a & 0xff}"
	else
		str = "#{a >> 12}.#{(a >> 8) & 0xf}.#{a & 0xff}"
	end
	return(str)
end

# str2addr: Parse an address string into an unsigned 16-bit integer
def str2addr(s)
	if m = s.match(/(\d*)\/(\d*)\/(\d*)/)
		a, b, c = m[1].to_i, m[2].to_i, m[3].to_i
		return ((a & 0x01f) << 11) | ((b & 0x07) << 8) | ((c & 0xff))
	end
	if m = s.match(/(\d*)\/(\d*)/)
		a,b = m[1].to_i, m[2].to_i
		return ((a & 0x01f) << 11) | ((b & 0x7FF))
	end
	if s.start_with?("0x")
		return (s.to_i & 0xffff)
	end
end
		
conn = EIBConnection.new()
conn_id = conn.EIBSocketURL("ip:192.168.0.10")
puts("conn=#{conn.inspect}\nconn_id=#{conn_id.inspect}")

buf = EIBBuffer.new()
puts(buf.inspect)

dest= str2addr("1/2/0")
puts(dest)
if (conn.EIBOpenT_Group(dest, 1) == -1)
	puts("KNX client: error setting socket mode")
	puts(conn.inspect)
	exit(1)
end
val=0
data = [0, 0x80| val]
puts ("data length=#{data.length}")
conn.EIBSendAPDU(data)
conn.EIBReset()