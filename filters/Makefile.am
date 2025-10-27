# 定义该目录下的库文件名称（通常以 lib 开头，后缀根据平台自动生成）
noinst_LTLIBRARIES = libfilters.la

# 指定库文件的源文件
libfilters_la_SOURCES = \
    addressfilter.cpp \
    addressfilter.h

# 包含头文件搜索路径（如果需要）
AM_CPPFLAGS = -I$(top_srcdir)/src/libserver \
              -I$(top_srcdir)/src/backend \
              -I$(top_srcdir)/src/common

# 链接依赖（如果需要链接其他库）
libfilters_la_LIBADD = $(top_builddir)/src/libserver/libserver.la