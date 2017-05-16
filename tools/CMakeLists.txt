include_directories(${PROJECT_SOURCE_DIR}/src/common)
add_executable(test_inih test_inih.cpp)
target_link_libraries(test_inih common)

add_executable(test_for_fail test_for_fail.c)
# Tests

file(GLOB BAD_INI bad*.ini)
add_test(NAME test_inih COMMAND test_inih ${CMAKE_CURRENT_SOURCE_DIR}/test.ini ${BAD_INI})
	#tools/test_inih tools/test.ini tools/bad*.ini
