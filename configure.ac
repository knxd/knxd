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

AC_INIT(knxd, m4_esyscmd(sh ./tools/version.sh))
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
AC_LANG_CPLUSPLUS

AC_CHECK_HEADERS([sys/time.h sys/select.h])
AX_CXX_COMPILE_STDCXX(11)

GMP_PROG_CPP_FOR_BUILD
GMP_PROG_EXEEXT_FOR_BUILD
CC_FOR_BUILD_OPTS

dnl AC_COMPILER_OPTION(nortti,[-fno-rtti],[-fno-rtti],CXXFLAGS="$CXXFLAGS -fno-rtti")
AC_COMPILER_OPTION(stdc,[-std=c++0x],[-std=c++0x],CXXFLAGS="$CXXFLAGS -std=c++0x -Wno-subobject-linkage")
dnl libev++ requires exceptions
dnl AC_COMPILER_OPTION(noexceptions,[-fno-exceptions],[-fno-exceptions],CXXFLAGS="$CXXFLAGS -fno-exceptions")

AX_LOCAL_EXPAND_CONFIG
AX_ENABLE_SYSTEMD

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

AC_DEFUN([NEED_EMI], [dnl
 need_emi=yes
 AC_MSG_RESULT([EMI because of $1])
])

AC_ARG_ENABLE(busmonitor,
[  --enable-busmonitor		enable bus monitoring],
[case "${enableval}" in
 yes) busmonitor=true ;;
  no)  busmonitor=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-busmonitor) ;;
 esac],[busmonitor=true])
AM_CONDITIONAL(HAVE_BUSMONITOR, test x$busmonitor = xtrue)
if test x$busmonitor = xtrue ; then
 AC_DEFINE(HAVE_BUSMONITOR, 1 , [bus monitoring enabled])
fi

AC_ARG_ENABLE(management,
[  --enable-management		enable management connections],
[case "${enableval}" in
 yes) management=true ;;
  no)  management=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-management) ;;
 esac],[management=false])
AM_CONDITIONAL(HAVE_MANAGEMENT, test x$management = xtrue)
if test x$management = xtrue ; then
 AC_DEFINE(HAVE_MANAGEMENT, 1 , [management connections enabled])
fi

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
 NEED_EMI(ft12)
fi

AC_ARG_ENABLE(dummy,
[  --enable-dummy		enable do-nothing backend],
[case "${enableval}" in
 yes) dummy=true ;;
  no)  dummy=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-dummy) ;;
 esac],[dummy=true])
AM_CONDITIONAL(HAVE_DUMMY, test x$dummy = xtrue)
if test x$dummy = xtrue ; then
 AC_DEFINE(HAVE_DUMMY, 1 , [dummy enabled])
fi

dnl AC_ARG_ENABLE(pei16s,
dnl [  --enable-pei16s     enable BCU1 user driver backend (very experimental)],
dnl [case "${enableval}" in
dnl  yes) pei16s=true ;;
dnl   no)  pei16s=false ;;
dnl    *) AC_MSG_ERROR(bad value ${enableval} for --enable-pei16s) ;;
dnl  esac],[pei16s=false])
dnl AM_CONDITIONAL(HAVE_PEI16s, test x$pei16s = xtrue)
dnl if test x$pei16s = xtrue ; then
dnl  AC_DEFINE(HAVE_PEI16s, 1 , [PEI16s enabled])
dnl  NEED_EMI(pei16s)
dnl fi

AC_ARG_ENABLE(tpuart,
[  --enable-tpuart	enable TPUART driver backends],
[case "${enableval}" in
 yes) tpuart=true ;;
  no)  tpuart=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-tpuart) ;;
 esac],[tpuart=true])

AM_CONDITIONAL(HAVE_TPUART, test x$tpuart = xtrue)
if test x$tpuart = xtrue ; then
 AC_DEFINE(HAVE_TPUART, 1 , [common TPUART code enabled])
fi

AC_ARG_ENABLE(eibnetip,
[  --enable-eibnetip	enable simple multicast EIBnet/IP backend],
[case "${enableval}" in
 yes) eibnetip=true ;;
  no)  eibnetip=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-eibnetip) ;;
 esac],[eibnetip=true])
AM_CONDITIONAL(HAVE_EIBNETIP, test x$eibnetip = xtrue)
if test x$eibnetip = xtrue ; then
 AC_DEFINE(HAVE_EIBNETIP, 1 , [EIBnet/IP multicast slave enabled])
fi

AC_ARG_ENABLE(eibnetserver,
[  --enable-eibnetserver	enable full-featured EIBnet/IP routing],
[case "${enableval}" in
 yes) eibnetserver=true ;;
  no)  eibnetserver=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-eibnetserver) ;;
 esac],[eibnetserver=true])
AM_CONDITIONAL(HAVE_EIBNETSERVER, test x$eibnetserver = xtrue)
if test x$eibnetserver = xtrue ; then
 AC_DEFINE(HAVE_EIBNETSERVER, 1 , [EIBnet/IP master enabled])
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
 esac],[usb=true])

if test x$usb = xtrue ; then
 PKG_CHECK_MODULES([LIBUSB], [libusb-1.0 >= 1.0.9] , AM_CONDITIONAL(HAVE_USB, test x$usb = xtrue))
 if test x$have_usb != xyes ; then
  AC_MSG_ERROR(not all needed functions for USB backend available)
 fi
 AC_DEFINE(HAVE_USB, 1 , [USB backend enabled])
 NEED_EMI(usb)
else
 AM_CONDITIONAL(HAVE_USB, test x$usb = xtrue)
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
AM_CONDITIONAL(HAVE_GROUPCACHE, test x$groupcache = xtrue)
if test x$groupcache = xtrue ; then
 AC_DEFINE(HAVE_GROUPCACHE, 1 , [Group Cache enabled])
fi

AM_CONDITIONAL(HAVE_EMI, test x$need_emi = xyes)

LIBS="-lfmt $LIBS"
AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <fmt/format.h>], [fmt::format("The answer is {}", 42);
])],
    [HAVE_FMT=1],                                           
    [
CC=${CC} CXX=${CXX} sh ${ac_aux_dir}/tools/get_libfmt || exit 1
CPPFLAGS="$CPPFLAGS -I$(pwd)/libfmt"
LDFLAGS="$LDFLAGS -L$(pwd)/libfmt/fmt"

AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <fmt/format.h>], [fmt::format("The answer is {}", 42);
])],
[],
[AC_MSG_ERROR([libfmt just built is not usable!?!])])

    ]
    )
AC_DEFINE(HAVE_FMT, 1, [ String formatting library installed. ])
AC_CHECK_HEADER([fmt/printf.h],
AC_DEFINE(HAVE_FMT_PRINTF, 1, [ fmt/printf.h header found ])
)

AC_CHECK_LIB([ev], [ev_run],,[AC_MSG_ERROR([libev not found])])
EV_LIBS="-lev"
AC_SUBST([EV_LIBS])

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
src/client/perl/Makefile src/client/python/Makefile src/client/pascal/Makefile src/client/ruby/Makefile src/client/lua/Makefile src/client/go/Makefile
src/tools/eibnet/Makefile src/tools/bcu/Makefile src/usb/Makefile src/tools/Makefile systemd/Makefile systemd/knxd.service systemd/knxd.socket
tools/Makefile
)

AC_CONFIG_MACRO_DIR([m4])
