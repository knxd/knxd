#~ @head = []
#~ data = "\000\002".split('').collect{|c| c.unpack('c')[0]}
#~ @head.concat(data)
#~ puts @head.inspect

#~ puts (@head[0] <<8)|@head[1]
#~ a=@head[0].unpack('c')[0]
#~ b=@head[1].unpack('c')[0]
#~ puts a,b
#~ @datalen = (a << 8) || b
#~ puts (@datalen.inspect)
#~ puts ''

#~ foo = (a<<8) | b
#~ puts foo.inspect

#~ c=0
#~ d=2
#~ bar = (c << 8) | d
#~ puts bar.inspect

data = [0, 0x80| 1]
puts data.inspect
puts data.class

ibuf = [0] * 2;
sendlen = data.length
    ibuf.concat(data)
    ibuf[0] = 0
    ibuf[1] = 37

    puts ibuf.inspect