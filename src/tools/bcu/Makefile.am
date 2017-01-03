bin_PROGRAMS=bcuaddrtab bcuread
AM_CPPFLAGS=-I$(top_srcdir)/src/common -I$(top_srcdir)/src/libserver -I$(top_srcdir)/src/backend -I$(top_srcdir)/src/usb $(LIBUSB_CFLAGS)
BACKEND_CONF= l-FT12.h l-USB.h

noinst_LIBRARIES=libbcu.a
libbcu_a_SOURCES=addrtab.cpp addrtab.h lowlevelcreate.h lowlevelconf.h $(BACKEND_CONF)

bcuaddrtab_SOURCES=bcuaddrtab.cpp
bcuaddrtab_LDADD=libbcu.a ../../backend/libbackend.a ../../libserver/libeibstack.a ../../common/libcommon.a ../../usb/libusb.a $(LIBUSB_LIBS) $(EV_LIBS)

bcuread_SOURCES=bcuread.cpp
bcuread_LDADD=libbcu.a ../../backend/libbackend.a ../../libserver/libeibstack.a ../../common/libcommon.a ../../usb/libusb.a $(LIBUSB_LIBS) $(EV_LIBS)


