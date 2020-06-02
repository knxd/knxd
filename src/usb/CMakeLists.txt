add_library(usb STATIC)
target_link_libraries(usb PUBLIC server)
target_include_directories(usb PUBLIC .)
if(LINUX_API)
  target_compile_definitions(usb PUBLIC -DTHREADS_POSIX)
endif()
if(HAVE_USB)
  target_link_libraries(
    usb
    PUBLIC PkgConfig::libusb
    PRIVATE LibEv::LibEv)
  target_sources(usb PRIVATE usb.cpp)
endif()
target_sources(usb PRIVATE dummy.cpp)

if(HAVE_USB)
  add_executable(findknxusb)
  target_sources(findknxusb PRIVATE findknxusb.cpp)
  target_link_libraries(findknxusb PRIVATE usb eibstack)

  install(
    TARGETS findknxusb
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
