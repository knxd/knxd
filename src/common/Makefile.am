noinst_HEADERS=types.h callbacks.h
noinst_LIBRARIES=libcommon.a
libcommon_a_SOURCES=loadctl.h image.cpp image.h loadimage.h loadimage.cpp \
	iobuf.cpp inifile.h inifile.cpp
if HAVE_INIH
else
libcommon_a_SOURCES+=inih.h inih.c
endif

dist_include_HEADERS=eibloadresult.h
