noinst_LIBRARIES = libeibstack.a
AM_CPPFLAGS=-I$(top_srcdir)/src/include -I$(top_srcdir)/src/common $(PTH_CFLAGS)

COMMON=exception.h queue.h common.h common.cpp threads.h threads.cpp trace.h trace.cpp 
PDUs=lpdu.h lpdu.cpp tpdu.h tpdu.cpp apdu.h apdu.cpp 
CORE=lowlevel.h layer2.h layer2.cpp layer3.h layer3.cpp layer4.h layer4.cpp layer7.h layer7.cpp 
CACHE=groupcache.h groupcache.cpp groupcacheclient.h groupcacheclient.cpp 
MANAGEMENT=management.h management.cpp
FRONTEND_C=client.h client.cpp busmonitor.h busmonitor.cpp connection.h connection.cpp managementclient.h managementclient.cpp 
FRONTEND=server.h server.cpp localserver.h localserver.cpp inetserver.h inetserver.cpp $(FRONTEND_C)
EMI=emi1.h emi1.cpp emi2.h emi2.cpp cemi.h cemi.cpp emi.h emi.cpp
EIBNETIP=eibnetip.cpp eibnetip.h eibnetserver.cpp eibnetserver.h
USB=eibusb.cpp eibusb.h

libeibstack_a_SOURCES =$(COMMON) $(CORE) $(PDUs) $(MANAGEMENT) $(FRONTEND) $(EMI) $(EIBNETIP) $(USB) $(CACHE)
