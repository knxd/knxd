#!/usr/bin/python

from __future__ import print_function

from EIBConnection import EIBConnection,EIBAddr,EIBBuffer

def run(port,adr):
    c = EIBConnection()
    if port[0] == '/':
      c.EIBSocketLocal(port)
    else:
      parts=port.split(':')
      if len(parts) == 1:
        parts.append(6720)
      c.EIBSocketRemote(parts[0], int(parts[1]))
    c.EIBOpen_GroupSocket(adr)
    buf = EIBBuffer()
    src = EIBAddr()
    dest = EIBAddr()
    while c.EIBGetGroup_Src(buf,src,dest):
        print("%s > %s: %s" % (src.data,dest.data,repr(buf.buffer)))

if __name__ == "__main__":
    import sys
    args = list(sys.argv[1:])
    if args:
        try:
            adr = int(args[-1])
        except ValueError:
            adr = 0
        else:
            args.pop()
    else:
        adr = 0
    if args:
        port = args[0]
    else:
        import os
        if os.path.exists('/run/knx'):
            port = '/run/knx'
        elif os.path.exists('/tmp/eib'):
            port = '/tmp/eib'
        else:
            port = 'localhost'
    run(port, adr)
