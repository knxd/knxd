bin_PROGRAMS = knxd 
AM_CPPFLAGS=-I$(top_srcdir)/src/libserver -I$(top_srcdir)/src/backend -I$(top_srcdir)/src/common -I$(top_srcdir)/src/usb $(LIBUSB_CFLAGS) $(SYSTEMD_CFLAGS) -Wno-missing-field-initializers
knxd_LDADD=../backend/libbackend.a ../libserver/libeibstack.a ../common/libcommon.a ../usb/libusb.a $(LIBUSB_LIBS) $(SYSTEMD_LIBS) $(EV_LIBS)
BACKEND_CONF= b-EIBNETIP.h b-FT12.h b-TPUARTs.h b-TPUARTs-TCP.h b-EIBNETIPTUNNEL.h b-USB.h b-NCN5120.h b-DUMMY.h f-DUMMY.h f-NAT.h
knxd_SOURCES=knxd.cpp create.h create.cpp layer2conf.h layer2create.h $(BACKEND_CONF)
