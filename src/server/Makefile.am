bin_PROGRAMS = knxd 
libexec_PROGRAMS = knxd_args

AM_CPPFLAGS=-I$(top_srcdir)/src/libserver -I$(top_srcdir)/src/backend -I$(top_srcdir)/src/common -I$(top_srcdir)/src/usb $(LIBUSB_CFLAGS) $(SYSTEMD_CFLAGS) -Wno-missing-field-initializers
knxd_CPPFLAGS=$(AM_CPPFLAGS) -DLIBEXECDIR="\"$(libexecdir)\""
knxd_LDFLAGS=-Wl$(LINK_ALL)$(LINK_FORCE),../backend/libbackend.a$(LINK_FORCE),../libserver/libserver.a$(NO_LINK_ALL)
knxd_LDADD=../libserver/libeibstack.a ../common/libcommon.a ../usb/libusb.a $(LIBUSB_LIBS) $(SYSTEMD_LIBS) $(EV_LIBS)
knxd_DEPENDENCIES=../libserver/libserver.a ../backend/libbackend.a ../libserver/libeibstack.a ../common/libcommon.a ../usb/libusb.a
knxd_args_DEPENDENCIES=../common/libcommon.a
knxd_args_LDADD=../common/libcommon.a
knxd_SOURCES=knxd.cpp
knxd_args_SOURCES=knxd_args.cpp
