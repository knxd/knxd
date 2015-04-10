dnl ##    BCU SDK bcu development enviroment - autoconf & automake files
dnl ##    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>
dnl ##
dnl ##    Redistribution and use in source and binary forms, with or without
dnl ##    modification, are permitted provided that the following conditions
dnl ##    are met:
dnl ##    1. Redistributions of source code must retain the above copyright
dnl ##       notice, this list of conditions and the following disclaimer.
dnl ##    2. Redistributions in binary form must reproduce the above copyright
dnl ##       notice, this list of conditions and the following disclaimer in the
dnl ##       documentation and/or other materials provided with the distribution.
dnl ##    
dnl ##    THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
dnl ##    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
dnl ##    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
dnl ##    ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
dnl ##    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
dnl ##    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
dnl ##    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
dnl ##    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
dnl ##    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
dnl ##    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
dnl ##    SUCH DAMAGE.


AC_PREREQ(2.59)

AC_INIT(knxd, 0.1.0)
AM_INIT_AUTOMAKE([foreign])

AC_CANONICAL_HOST
AC_PROG_CC
AC_PROG_CXX
AC_STDC_HEADERS
AC_PROG_YACC
AC_PROG_LEX
AM_PROG_AS
AC_PROG_RANLIB
AC_PROG_LIBTOOL

AC_CHECK_HEADERS([sys/time.h])

GMP_PROG_CPP_FOR_BUILD
GMP_PROG_EXEEXT_FOR_BUILD
CC_FOR_BUILD_OPTS

AC_COMPILER_OPTION(nortti,[-fno-rtti],[-fno-rtti],CXXFLAGS="$CXXFLAGS -fno-rtti")
AC_COMPILER_OPTION(noexceptions,[-fno-exceptions],[-fno-exceptions],CXXFLAGS="$CXXFLAGS -fno-exceptions")
AC_CHECK_PTHSEM(2.0.8,yes,yes,no)
PTHSEM_LIB="$PTH_LDFLAGS $PTH_LIBS"
AC_SUBST(PTHSEM_LIB)

AX_LOCAL_EXPAND_CONFIG
AX_ALLOW_SYSTEMD

AC_CHECK_HEADER(argp.h,,[AC_MSG_ERROR([argp_parse not found])])
AC_SEARCH_LIBS(argp_parse,argp,,[AC_MSG_ERROR([argp_parse not found])])
AC_CHECK_HEADER(linux/serial.h,[AC_DEFINE(HAVE_LINUX_LOWLATENCY, 1 , [Linux low latency mode enabled])],[AC_MSG_WARN([No supported low latency mode found])])
have_source_info=no
have_linux_api=no
AC_CHECK_HEADER(linux/rtnetlink.h,[AC_DEFINE(HAVE_LINUX_NETLINK, 1,[Linux netlink layer available]) have_source_info=yes],[],[-])
AC_CHECK_HEADER(linux/usbdevice_fs.h,[AC_DEFINE(OS_LINUX, 1, [Linux usb available]) have_linux_api=yes; have_usb=yes],[],[-])
AC_CHECK_HEADER(iphlpapi.h,[AC_DEFINE(HAVE_WINDOWS_IPHELPER, 1,[Windows IPHelper available]) 
  LIBS="-liphlpapi $LIBS"; have_source_info=yes],[],[-])

AC_CHECK_DECL(SA_SIZE,[AC_DEFINE(HAVE_SA_SIZE, 1,[SA_SIZE function available])],[],[#include <net/route.h>])
have_bsd_source_info=no
AC_CHECK_DECL(__FreeBSD__,[have_bsd_source_info=yes],[],[])
AC_CHECK_DECL(__APPLE__,[have_bsd_source_info=yes],[],[])
if test x$have_bsd_source_info = xyes ; then
  AC_DEFINE(HAVE_BSD_SOURCEINFO, 1,[FreeBSD compatible sourceinfo])
  have_source_info=yes
fi

AC_CHECK_FUNCS(gethostbyname_r,,[AC_MSG_WARN([knxd client library not thread safe])])

AM_CONDITIONAL(LINUX_API, test x$have_linux_api = xyes)

AC_ARG_ENABLE(ft12,
[  --enable-ft12		enable FT1.2 backend],
[case "${enableval}" in
 yes) ft12=true ;;
  no)  ft12=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-ft12) ;;
 esac],[ft12=true])
AM_CONDITIONAL(HAVE_FT12, test x$ft12 = xtrue)
if test x$ft12 = xtrue ; then
 AC_DEFINE(HAVE_FT12, 1 , [FT1.2 enabled])
fi

AC_ARG_ENABLE(pei16,
[  --enable-pei16	enable BCU1 kernel driver backend],
[case "${enableval}" in
 yes) pei16=true ;;
  no)  pei16=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-pei16) ;;
 esac],[pei16=false])
AM_CONDITIONAL(HAVE_PEI16, test x$pei16 = xtrue)
if test x$pei16 = xtrue; then
 AC_DEFINE(HAVE_PEI16, 1 , [PEI16 enabled])
fi

AC_ARG_ENABLE(tpuart,
[  --enable-tpuart	enable TPUART kernel driver backend (deprecated)],
[case "${enableval}" in
 yes) tpuart=true ;;
  no)  tpuart=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-tpuart) ;;
 esac],[tpuart=false])
AM_CONDITIONAL(HAVE_TPUART, test x$tpuart = xtrue)
if test x$tpuart = xtrue ; then
 AC_DEFINE(HAVE_TPUART, 1 , [TPUART enabled])
fi

AC_ARG_ENABLE(pei16s,
[  --enable-pei16s	enable BCU1 user driver backend (very experimental)],
[case "${enableval}" in
 yes) pei16s=true ;;
  no)  pei16s=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-pei16s) ;;
 esac],[pei16s=false])
AM_CONDITIONAL(HAVE_PEI16s, test x$pei16s = xtrue)
if test x$pei16s = xtrue ; then
 AC_DEFINE(HAVE_PEI16s, 1 , [PEI16s enabled])
fi

AC_ARG_ENABLE(tpuarts,
[  --enable-tpuarts	enable TPUART user driver backend],
[case "${enableval}" in
 yes) tpuarts=true ;;
  no)  tpuarts=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-tpuarts) ;;
 esac],[tpuarts=true])
AM_CONDITIONAL(HAVE_TPUARTs, test x$tpuarts = xtrue)
if test x$tpuarts = xtrue ; then
 AC_DEFINE(HAVE_TPUARTs, 1 , [TPUARTs enabled])
fi

AC_ARG_ENABLE(eibnetip,
[  --enable-eibnetip	enable EIBnet/IP routing backend],
[case "${enableval}" in
 yes) eibnetip=true ;;
  no)  eibnetip=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-eibnetip) ;;
 esac],[eibnetip=true])
AM_CONDITIONAL(HAVE_EIBNETIP, test x$eibnetip = xtrue)
if test x$eibnetip = xtrue ; then
 AC_DEFINE(HAVE_EIBNETIP, 1 , [EIBnet/IP enabled])
fi

AC_ARG_ENABLE(eibnetiptunnel,
[  --enable-eibnetiptunnel	enable EIBnet/IP tunneling backend],
[case "${enableval}" in
 yes) eibnetiptunnel=true ;;
  no)  eibnetiptunnel=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-eibnetiptunnel) ;;
 esac],[eibnetiptunnel=true])
AM_CONDITIONAL(HAVE_EIBNETIPTUNNEL, test x$eibnetiptunnel = xtrue)
if test x$eibnetiptunnel = xtrue ; then
 if test x$have_source_info != xyes ; then
  AC_MSG_ERROR(not all needed functions for EIBnet/IP tunneling backend available)
 fi
 AC_DEFINE(HAVE_EIBNETIPTUNNEL, 1 , [EIBnet/IP tunneling enabled])
fi

AC_ARG_ENABLE(usb,
[  --enable-usb			enable USB backend],
[case "${enableval}" in
 yes) usb=true ;;
  no)  usb=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-usb) ;;
 esac],[usb=false])

if test x$usb = xtrue ; then
 PKG_CHECK_MODULES([LIBUSB], [libusb-1.0 >= 1.0.10] , AM_CONDITIONAL(HAVE_USB, test x$usb = xtrue))
 if test x$have_usb != xyes ; then
  AC_MSG_ERROR(not all needed functions for USB backend available)
 fi
 AC_DEFINE(HAVE_USB, 1 , [USB backend enabled])
else
 AM_CONDITIONAL(HAVE_USB, test x$usb = xtrue)
fi

AC_ARG_ENABLE(ncn5120,
[  --enable-ncn5120                 enable NCN5120 backend],
[case "${enableval}" in
 yes) ncn5120=true ;;
  no)  ncn5120=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-ncn5120) ;;
 esac],[ncn5120=false])
AM_CONDITIONAL(HAVE_NCN5120, test x$ncn5120 = xtrue)
if test x$ncn5120 = xtrue ; then
 AC_DEFINE(HAVE_NCN5120, 1 , [NCN5120 backend enabled])
fi

AC_ARG_ENABLE(eibnetipserver,
[  --enable-eibnetipserver	enable EIBnet/IP server frontend],
[case "${enableval}" in
 yes) eibnetipserver=true ;;
  no)  eibnetipserver=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-eibnetipserver) ;;
 esac],[eibnetipserver=true])
if test x$eibnetipserver = xtrue ; then
 if test x$have_source_info != xyes ; then
  AC_MSG_ERROR(not all needed functions for EIBnet/IP server available)
 fi
 AC_DEFINE(HAVE_EIBNETIPSERVER, 1 , [EIBnet/IP server enabled])
fi

AC_ARG_ENABLE(groupcache,
[  --enable-groupcache		enable Group Cache (default: yes)],
[case "${enableval}" in
 yes) groupcache=true ;;
  no)  groupcache=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-groupcache) ;;
 esac],[groupcache=true])
if test x$groupcache = xtrue ; then
 AC_DEFINE(HAVE_GROUPCACHE, 1 , [Group Cache enabled])
fi


AM_CONDITIONAL(HAVE_SOURCE_INFO, test x$have_source_info = xyes )

AC_CHECK_PROGS(INDENT, indent, no)
AC_CHECK_PROGS(JAR, fastjar jar, no)
AC_CHECK_PROGS(JAVAC, "gcj -C" javac, no)

AC_ARG_ENABLE(java,
[  --enable-java		build java client library],
[case "${enableval}" in
 yes) enablejava=true ;;
  no)  enablejava=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-java) ;;
 esac],[enablejava=false])
if test x$enablejava = xtrue ; then
 AC_DEFINE(BUILD_JAVA, 1 , [building java client library])

 if test x"$JAVAC" = xno ; then
  AC_MSG_ERROR(no java compiler found)
 fi

 if test x"$JAR" = xno ; then
  AC_MSG_ERROR(jar not found)
 fi

fi

AC_ARG_WITH(libstdc,
[  --without-libstdc	don't use function from libstdc++],
[case "${withval}" in
 yes) use_libstdc=true ;;
  no)  use_libstdc=false ;;
   *) AC_MSG_ERROR(bad value ${withval} for --with-libstdc) ;;
 esac],[use_libstdc=true])
if test x$use_libstdc = xfalse ; then
 AC_DEFINE(USE_NOLIBSTDC, 1 , [don't use libstdc++])
 CXX=$CC
fi

AM_CONDITIONAL(BUILD_JAVA, test x$enablejava = xtrue )

AC_CHECK_MEMBER([struct sockaddr_in.sin_len],[AC_DEFINE(HAVE_SOCKADDR_IN_LEN,1,[Do we have sockaddr_in.sin_len?])],[],
[
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
])

AC_CONFIG_HEADERS(config.h)
AC_OUTPUT(Makefile 
src/common/Makefile
src/Makefile src/include/Makefile  src/client/Makefile src/examples/Makefile src/libserver/Makefile src/server/Makefile src/backend/Makefile
src/client/def/Makefile src/client/c/Makefile src/client/java/Makefile src/client/php/Makefile src/client/cs/Makefile
src/client/perl/Makefile src/client/python/Makefile src/client/pascal/Makefile src/client/ruby/Makefile src/client/lua/Makefile
src/tools/eibnet/Makefile src/tools/bcu/Makefile src/usb/Makefile src/tools/Makefile systemd/Makefile systemd/knxd.service systemd/knxd.socket
)

AC_CONFIG_MACRO_DIR([m4])
