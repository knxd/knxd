set(EIBD_USB usb.cpp)
set(USB_INCLUDES ${PROJECT_SOURCE_DIR}/src/libserver ${LIBUSB_INCLUDE_DIRS})

if(LINUX_API)
add_definitions(-DTHREADS_POSIX)
set(TARGET linux_usbfs.c)
endif()

if(HAVE_USB)
  add_library(usb STATIC ${EIBD_USB})
  target_link_libraries(usb fmt::fmt)
  target_include_directories(usb PUBLIC ${USB_INCLUDES})

  add_executable(findknxusb findknxusb.cpp)
  target_link_libraries(findknxusb usb ${LIBUSB_LIBRARIES})
  target_include_directories(findknxusb PUBLIC ${USB_INCLUDES})
  install(TARGETS findknxusb RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

