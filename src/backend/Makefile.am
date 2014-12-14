if HAVE_FT12
FT12 = ft12.h ft12.cpp lowlatency.h lowlatency.cpp
else
FT12 =
endif

if HAVE_PEI16
PEI16 = bcu1.h bcu1.cpp
else
PEI16 =
endif

if HAVE_TPUART
TPUART = tpuart.h tpuart.cpp
else
TPUART =
endif

if HAVE_PEI16s
PEI16s = bcu1serial.h bcu1serial.cpp lowlatency.h lowlatency.cpp
else
PEI16s =
endif

if HAVE_TPUARTs
TPUARTs = tpuartserial.h tpuartserial.cpp lowlatency.h lowlatency.cpp
else
TPUARTs =
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

if HAVE_USB
USB = usbif.h usbif.cpp
else
USB =
endif

noinst_LIBRARIES = libbackend.a
AM_CPPFLAGS=-I$(top_srcdir)/eibd/libserver -I$(top_srcdir)/common -I$(top_srcdir)/eibd/usb $(PTHSEM_CFLAGS)

libbackend_a_SOURCES= $(FT12) $(PEI16) $(TPUART) $(PEI16s) $(TPUARTs) $(EIBNETIP) $(EIBNETIPTUNNEL) $(USB) dummy.cpp

