if HAVE_FT12
FT12 = ft12.h ft12.cpp
else
FT12 =
endif

if HAVE_TPUART
TPUART_COMMON = tpuart.h tpuart.cpp ncn5120.h ncn5120.cpp
else
TPUART_COMMON = 
endif

if HAVE_EIBNETIP
EIBNETIP = eibnetrouter.h eibnetrouter.cpp
else
EIBNETIP =
endif

if HAVE_EIBNETIPTUNNEL
EIBNETIPTUNNEL = eibnettunnel.h eibnettunnel.cpp
else
EIBNETIPTUNNEL =
endif

noinst_LIBRARIES = libbackend.a
AM_CPPFLAGS=-I$(top_srcdir)/src/libserver -I$(top_srcdir)/src/common -I$(top_srcdir)/src/usb $(LIBUSB_CFLAGS)

libbackend_a_SOURCES= $(FT12) $(TPUART_COMMON) $(EIBNETIP) $(EIBNETIPTUNNEL) \
	log.cpp dummy.cpp nat.cpp fqueue.cpp fpace.cpp

