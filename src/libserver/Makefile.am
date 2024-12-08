noinst_LIBRARIES = libeibstack.a libserver.a
AM_CPPFLAGS = -I$(top_srcdir)/src/include -I$(top_srcdir)/src/common -I$(top_srcdir)/src/usb $(LIBUSB_CFLAGS)

### libeibstack

COMMON = common.h common.cpp trace.h trace.cpp emi.h emi.cpp lowlevel.h lowlevel.cpp

# 03.02 Communication Media
CM = cm_tp1.h cm_tp1.cpp cm_ip.h cm_ip.cpp

# 03.03 Communication
L2 = lpdu.h lpdu.cpp link.h link.cpp
L3 = npdu.h npdu.cpp layer3.h layer3.cpp router.h router.cpp
if HAVE_GROUPCACHE
L3 += groupcache.h groupcache.cpp groupcacheclient.h groupcacheclient.cpp
endif
L4 = tpdu.h tpdu.cpp layer4.h layer4.cpp
L7 = apdu.h apdu.cpp
if HAVE_BUSMONITOR
L7 += busmonitor.h busmonitor.cpp
endif
if HAVE_MANAGEMENT
L7 += management.h management.cpp layer7.h layer7.cpp
L7 += managementclient.h managementclient.cpp
endif

# 03.06.03 External Message Interface

# 03.08 KNXnet/IP
NETIP = eibnetip.cpp eibnetip.h ipsupport.h ipsupport.cpp

FRONTEND = client.h client.cpp connection.h connection.cpp

libeibstack_a_SOURCES = $(COMMON) $(CM) $(L2) $(L3) $(L4) $(L7) $(CORE) $(NETIP) $(FRONTEND)

### libserver

if HAVE_SYSTEMD
SYSTEMD_SERVER = systemdserver.cpp systemdserver.h
else
SYSTEMD_SERVER =
endif

if HAVE_EIBNETSERVER
EIBNETIP = eibnetserver.cpp eibnetserver.h tcptunserver.cpp tcptunserver.h tunchannel.cpp tunchannel.h
else
EIBNETIP =
endif

if HAVE_EMI
EMI = emi_common.h emi_common.cpp emi1.h emi1.cpp emi2.h emi2.cpp cemi.h cemi.cpp
else
EMI =
endif

if HAVE_USB
USB = eibusb.cpp eibusb.h usblowlevel.cpp usblowlevel.h
else
USB =
endif

libserver_a_SOURCES = server.h server.cpp localserver.h localserver.cpp inetserver.h inetserver.cpp $(SYSTEMD_SERVER) $(EIBNETIP) $(EMI) $(USB) llserial.h llserial.cpp lltcp.h lltcp.cpp lowlatency.h lowlatency.cpp retry.h retry.cpp
