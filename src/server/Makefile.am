bin_PROGRAMS = eibd 
AM_CPPFLAGS=-I$(top_srcdir)/src/libserver -I$(top_srcdir)/src/backend -I$(top_srcdir)/src/common -I$(top_srcdir)/src/usb $(PTH_CFLAGS) $(LIBUSB_CFLAGS)
eibd_LDADD=../backend/libbackend.a ../libserver/libeibstack.a ../common/libcommon.a ../usb/libusb.a $(PTHSEM_LIB) $(LIBUSB_LIBS)
BACKEND_CONF= b-EIBNETIP.h b-FT12.h b-PEI16.h b-PEI16s.h b-TPUART.h b-TPUARTs.h b-EIBNETIPTUNNEL.h b-USB.h b-NCN5120.h
eibd_SOURCES=knxd.cpp layer2conf.h layer2create.h $(BACKEND_CONF)
