if(SYSTEMD_FOUND)
  set(SYSTEMD_SERVER systemdserver.cpp)
endif()

set(COMMON common.cpp trace.cpp ipsupport.cpp emi.cpp)
set(PDUs lpdu.cpp tpdu.cpp apdu.cpp)
set(CORE lowlevel.cpp router.cpp layer4.cpp link.cpp)

if(HAVE_GROUPCACHE)
  set(CACHE groupcache.cpp groupcacheclient.cpp)
endif()

if(HAVE_MANAGEMENT)
  set(MANAGEMENT management.cpp layer7.cpp)
  set(MANAGEMENT_C managementclient.cpp)
endif()

set(FRONTEND_C client.cpp connection.cpp)
set(FRONT_C ${FRONTEND_C} ${MANAGEMENT_C})

if(HAVE_EMI)
  set(EMI emi_common.cpp emi1.cpp emi2.cpp cemi.cpp)
endif()

if(HAVE_BUSMONITOR)
  set(BUSMON busmonitor.h busmonitor.cpp)
endif()

set(NETIP eibnetip.cpp eibnetip.h)

if(HAVE_EIBNETSERVER)
  set(EIBNETIP eibnetserver.cpp eibnetserver.h)
endif()

if(HAVE_USB)
  set(USB=eibusb.cpp eibusb.h usblowlevel.cpp usblowlevel.h)
endif()

add_library(eibstack STATIC ${COMMON} ${CORE} ${PDUs} ${MANAGEMENT} ${NETIP} ${FRONT_C} ${CACHE} ${BUSMON})

add_library(server STATIC server.cpp localserver.cpp inetserver.cpp
  llserial.cpp lltcp.cpp lowlatency.cpp
  ${SYSTEMD_SERVER} ${EIBNETIP} ${EMI} ${USB})
target_link_libraries(server fmt)
