include_directories(${PROJECT_SOURCE_DIR}/src/libserver ${PROJECT_SOURCE_DIR}/src/backend )

add_executable(knxd knxd.cpp)
target_link_libraries(knxd -Wl,--whole-archive backend server -Wl,--no-whole-archive eibstack common usb ${LIBEV_LIBRARIES} fmt)
target_compile_definitions(knxd PRIVATE -DLIBEXECDIR="${CMAKE_INSTALL_PREFIX}/lib/${PACKAGE_NAME}")

# Only with CMake > 3.1
#target_compile_features(knxd PRIVATE cxx_auto_type)

install(TARGETS knxd RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(knxd_args knxd_args.cpp)
target_link_libraries(knxd_args common)
install(TARGETS knxd_args RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})

add_test(NAME CL1 COMMAND test_for_fail ${CMAKE_CURRENT_BINARY_DIR}/knxd -e 1.2.3 --stop-right-now)
add_test(NAME CL2 COMMAND test_for_fail ${CMAKE_CURRENT_BINARY_DIR}/knxd -e 1.2.3 --stop-right-now -b dummy:)
add_test(NAME CL3 COMMAND knxd -B log -e 1.2.3 --stop-right-now -b dummy: -b dummy:)
add_test(NAME CL4 COMMAND test_for_fail ${CMAKE_CURRENT_BINARY_DIR}/knxd -e 1.2.3 --stop-right-now -b dummy: -b dummy: --tpuarts-disch-reset)
add_test(NAME CL5 COMMAND test_for_fail ${CMAKE_CURRENT_BINARY_DIR}/knxd -e 1.2.3 --stop-right-now -b dummy: --tpuarts-disch-reset -b dummy:)
add_test(NAME CL6 COMMAND test_for_fail ${CMAKE_CURRENT_BINARY_DIR}/knxd -e 1.2.3 --stop-right-now -T -b dummy: -b dummy:)
