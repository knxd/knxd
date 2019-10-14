include_directories(${PROJECT_SOURCE_DIR}/src/common)
add_executable(test_inih test_inih.cpp)
target_link_libraries(test_inih common)

add_executable(test_for_fail test_for_fail.c)

# Tests
file(GLOB BAD_INI bad*.ini)
add_test(NAME test_inih COMMAND test_inih ${CMAKE_CURRENT_SOURCE_DIR}/test.ini ${BAD_INI})

add_test(NAME CL1 COMMAND test_for_fail $<TARGET_FILE:knxd> -e 1.2.3 --stop-right-now)
add_test(NAME CL2 COMMAND test_for_fail $<TARGET_FILE:knxd> -e 1.2.3 --stop-right-now -b dummy:)
add_test(NAME CL3 COMMAND knxd -B log -e 1.2.3 --stop-right-now -b dummy: -b dummy:)
add_test(NAME CL4 COMMAND test_for_fail $<TARGET_FILE:knxd> -e 1.2.3 --stop-right-now -b dummy: -b dummy: --tpuarts-disch-reset)
add_test(NAME CL5 COMMAND test_for_fail $<TARGET_FILE:knxd> -e 1.2.3 --stop-right-now -b dummy: --tpuarts-disch-reset -b dummy:)
add_test(NAME CL6 COMMAND test_for_fail $<TARGET_FILE:knxd> -e 1.2.3 --stop-right-now -T -b dummy: -b dummy:)
add_test(NAME IT COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/test.sh $<TARGET_FILE:knxd> $<TARGET_FILE:knxtool>)
