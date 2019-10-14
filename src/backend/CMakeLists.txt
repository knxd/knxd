if(HAVE_FT12)
set(FT12  ft12.h ft12.cpp)
endif()

if(HAVE_TPUART)
set(TPUART_COMMON  tpuart.h tpuart.cpp ncn5120.h ncn5120.cpp)
endif()

if(HAVE_EIBNETIP)
set(EIBNETIP  eibnetrouter.h eibnetrouter.cpp)
endif()

if(HAVE_EIBNETIPTUNNEL)
set(EIBNETIPTUNNEL  eibnettunnel.h eibnettunnel.cpp)
endif()

# noinst_LIBRARIES  libbackend.a
# AM_CPPFLAGS-I$(top_srcdir)/src/libserver -I$(top_srcdir)/src/common -I$(top_srcdir)/src/usb $(LIBUSB_CFLAGS)
set(BACKEND_INCLUDES ${PROJECT_SOURCE_DIR}/src/libserver)

add_library(backend STATIC ${FT12} ${TPUART_COMMON} ${EIBNETIP} ${EIBNETIPTUNNEL} 
	log.cpp dummy.cpp nat.cpp fqueue.cpp fpace.cpp)
target_include_directories(backend PUBLIC ${BACKEND_INCLUDES})
