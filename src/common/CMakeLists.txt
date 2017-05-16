add_library(common STATIC
  image.cpp loadimage.cpp
  iobuf.cpp inih.c inifile.cpp
  )

install(FILES eibloadresult.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
