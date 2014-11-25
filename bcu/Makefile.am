bin_PROGRAMS=bcuaddrtab bcuread
AM_CPPFLAGS=-I$(top_srcdir)/common -I$(top_srcdir)/eibd/libserver -I$(top_srcdir)/eibd/backend -I$(top_srcdir)/eibd/usb $(PTHSEM_CFLAGS)
BACKEND_CONF= l-FT12.h l-PEI16.h l-PEI16s.h l-USB.h

noinst_LIBRARIES=libbcu.a
libbcu_a_SOURCES=addrtab.cpp addrtab.h lowlevelcreate.h lowlevelconf.h $(BACKEND_CONF)

bcuaddrtab_SOURCES=bcuaddrtab.cpp
bcuaddrtab_LDADD=libbcu.a ../backend/libbackend.a ../libserver/libeibstack.a ../../common/libcommon.a ../usb/libusb.a $(PTHSEM_LIBS)

bcuread_SOURCES=bcuread.cpp
bcuread_LDADD=libbcu.a ../backend/libbackend.a ../libserver/libeibstack.a ../../common/libcommon.a ../usb/libusb.a $(PTHSEM_LIBS)

