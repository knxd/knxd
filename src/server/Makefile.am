bin_PROGRAMS = knxd 
AM_CPPFLAGS=-I$(top_srcdir)/src/libserver -I$(top_srcdir)/src/backend -I$(top_srcdir)/src/common -I$(top_srcdir)/src/usb $(PTH_CFLAGS) $(LIBUSB_CFLAGS) $(SYSTEMD_CFLAGS) -Wno-missing-field-initializers
knxd_LDADD=../backend/libbackend.a ../libserver/libeibstack.a ../common/libcommon.a ../usb/libusb.a $(PTHSEM_LIB) $(LIBUSB_LIBS) $(SYSTEMD_LIBS)
BACKEND_CONF= b-EIBNETIP.h b-FT12.h b-PEI16s.h b-TPUARTs.h b-EIBNETIPTUNNEL.h b-USB.h b-NCN5120.h b-DUMMY.h
knxd_SOURCES=knxd.cpp layer2conf.h layer2create.h $(BACKEND_CONF)
