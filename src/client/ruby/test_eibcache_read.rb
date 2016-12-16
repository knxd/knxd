require 'EIBConnection'
require 'knx_tools'

Conn = EIBConnection.new()
Conn.EIBSocketURL('local:/run/knx')
Buf =  EIBBuffer.new()
Src = EIBAddr.new()
Dest = str2addr('1/1/1')

result = Conn.EIB_Cache_Read_Sync(Dest,Src, Buf, 0)
if (result == -1)
    puts "not found in cache, requesting value on bus..."
    if (Conn.EIBOpenT_Group(Dest, 1) == -1) then
        raise("KNX client: error setting socket mode")
    end
    Conn.EIBSendAPDU([0,0].pack('c*'))
    # the receiver thread will receive the response
else
    puts "found in cache, last sender was #{addr2str(Src.data)}"
    puts "  value is #{Buf.buffer}"
end
