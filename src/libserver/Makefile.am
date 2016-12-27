noinst_LIBRARIES = libeibstack.a
AM_CPPFLAGS=-I$(top_srcdir)/src/include -I$(top_srcdir)/src/common $(PTH_CFLAGS)

if HAVE_SYSTEMD
SYSTEMD_SERVER=systemdserver.cpp systemdserver.h
else
SYSTEMD_SERVER=
endif

COMMON=exception.h common.h common.cpp trace.h trace.cpp ipsupport.h ipsupport.cpp
if HAVE_PTHSEM
THREADS=threads.h threads.cpp
else
THREADS=
endif
PDUs=lpdu.h lpdu.cpp tpdu.h tpdu.cpp apdu.h apdu.cpp 
CORE=lowlevel.h layer2.h layer2.cpp layer3.h layer3.cpp layer4.h layer4.cpp layer23.h layer23.cpp
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
FRONTEND_C=client.h client.cpp busmonitor.h busmonitor.cpp connection.h connection.cpp
FRONTEND=server.h server.cpp localserver.h localserver.cpp inetserver.h inetserver.cpp $(SYSTEMD_SERVER) $(FRONTEND_C) $(MANAGEMENT_C)
EMI=emi1.h emi1.cpp emi2.h emi2.cpp cemi.h cemi.cpp emi.h emi.cpp
if HAVE_EIBNETSERVER
EIBNETIP=eibnetip.cpp eibnetip.h eibnetserver.cpp eibnetserver.h
else
EIBNETIP=
endif
USB=eibusb.cpp eibusb.h

libeibstack_a_SOURCES =$(COMMON) $(CORE) $(THREADS) $(PDUs) $(MANAGEMENT) $(FRONTEND) $(EMI) $(EIBNETIP) $(USB) $(CACHE)
