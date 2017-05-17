noinst_LIBRARIES = libeibstack.a libserver.a

AM_CPPFLAGS=-I$(top_srcdir)/src/include -I$(top_srcdir)/src/common -I$(top_srcdir)/src/usb $(LIBUSB_CFLAGS)

if HAVE_SYSTEMD
SYSTEMD_SERVER=systemdserver.cpp systemdserver.h
else
SYSTEMD_SERVER=
endif

COMMON=exception.h common.h common.cpp trace.h trace.cpp ipsupport.h ipsupport.cpp emi.h emi.cpp
PDUs=lpdu.h lpdu.cpp tpdu.h tpdu.cpp apdu.h apdu.cpp 
CORE=lowlevel.h lowlevel.cpp router.h router.cpp layer4.h layer4.cpp link.h link.cpp
if HAVE_GROUPCACHE
CACHE=groupcache.h groupcache.cpp groupcacheclient.h groupcacheclient.cpp 
else
CACHE=
endif
if HAVE_MANAGEMENT
MANAGEMENT=management.h management.cpp layer7.h layer7.cpp 
MANAGEMENT_C=managementclient.h managementclient.cpp 
else
MANAGEMENT=
MANAGEMENT_C=
endif
FRONTEND_C=client.h client.cpp connection.h connection.cpp
FRONT_C=$(FRONTEND_C) $(MANAGEMENT_C)

if HAVE_EMI
EMI=emi_common.h emi_common.cpp emi1.h emi1.cpp emi2.h emi2.cpp cemi.h cemi.cpp
else
EMI=
endif
if HAVE_BUSMONITOR
BUSMON=busmonitor.h busmonitor.cpp 
else
BUSMON=
endif
NETIP=eibnetip.cpp eibnetip.h 
if HAVE_EIBNETSERVER
EIBNETIP=eibnetserver.cpp eibnetserver.h
else
EIBNETIP=
endif
if HAVE_USB
USB=eibusb.cpp eibusb.h usblowlevel.cpp usblowlevel.h
else
USB=
endif

libeibstack_a_SOURCES =$(COMMON) $(CORE) $(PDUs) $(MANAGEMENT) $(NETIP) $(FRONT_C) $(CACHE) $(BUSMON)

libserver_a_SOURCES = server.h server.cpp localserver.h localserver.cpp inetserver.h inetserver.cpp $(SYSTEMD_SERVER) $(EIBNETIP) $(EMI) $(USB) llserial.h llserial.cpp lltcp.h lltcp.cpp lowlatency.h lowlatency.cpp
