add_library(common STATIC)
target_link_libraries(common PUBLIC includes)
target_sources(common PRIVATE image.cpp loadimage.cpp iobuf.cpp inih.c
                              inifile.cpp)
target_include_directories(common PUBLIC .)

install(FILES eibloadresult.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
