include_directories(${PROJECT_SOURCE_DIR}/src/libserver ${PROJECT_SOURCE_DIR}/src/backend )

add_executable(knxd knxd.cpp)
target_link_libraries(knxd -Wl,--whole-archive backend server -Wl,--no-whole-archive eibstack common usb ${LIBEV_LIBRARIES})
target_compile_definitions(knxd PRIVATE -DLIBEXECDIR="${CMAKE_INSTALL_PREFIX}/lib/${PACKAGE_NAME}")

# Only with CMake > 3.1
#target_compile_features(knxd PRIVATE cxx_auto_type)

install(TARGETS knxd RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(knxd_args knxd_args.cpp)
target_link_libraries(knxd_args common)
install(TARGETS knxd_args RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
