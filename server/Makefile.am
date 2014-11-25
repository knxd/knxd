bin_PROGRAMS = eibd
AM_CPPFLAGS=-I$(top_srcdir)/eibd/libserver -I$(top_srcdir)/eibd/backend -I$(top_srcdir)/common -I$(top_srcdir)/eibd/usb $(PTHSEM_CFLAGS)
eibd_LDADD=../backend/libbackend.a ../libserver/libeibstack.a ../../common/libcommon.a ../usb/libusb.a $(PTHSEM_LIBS)
BACKEND_CONF= b-EIBNETIP.h b-FT12.h b-PEI16.h b-PEI16s.h b-TPUART.h b-TPUARTs.h b-EIBNETIPTUNNEL.h b-USB.h
eibd_SOURCES=eibd.cpp layer2conf.h layer2create.h $(BACKEND_CONF)
