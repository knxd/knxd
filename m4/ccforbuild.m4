dnl  GMP specific autoconf macros


dnl  Copyright 2000, 2001, 2002, 2003, 2004, 2005, 2006 Free Software
dnl  Foundation, Inc.
dnl
dnl  This file is part of the GNU MP Library.
dnl
dnl  The GNU MP Library is free software; you can redistribute it and/or modify
dnl  it under the terms of the GNU Lesser General Public License as published
dnl  by the Free Software Foundation; either version 2.1 of the License, or (at
dnl  your option) any later version.
dnl
dnl  The GNU MP Library is distributed in the hope that it will be useful, but
dnl  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
dnl  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
dnl  License for more details.
dnl
dnl  You should have received a copy of the GNU Lesser General Public License
dnl  along with the GNU MP Library; see the file COPYING.LIB.  If not, write to
dnl  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
dnl  MA 02110-1301, USA.

dnl  GMP_PROG_CC_FOR_BUILD
dnl  ---------------------
dnl  Establish CC_FOR_BUILD, a C compiler for the build system.
dnl
dnl  If CC_FOR_BUILD is set then it's expected to work, likewise the old
dnl  style HOST_CC, otherwise some likely candidates are tried, the same as
dnl  configfsf.guess.

AC_DEFUN([GMP_PROG_CC_FOR_BUILD],
[AC_REQUIRE([AC_PROG_CC])
if test -n "$CC_FOR_BUILD"; then
  GMP_PROG_CC_FOR_BUILD_WORKS($CC_FOR_BUILD,,
    [AC_MSG_ERROR([Specified CC_FOR_BUILD doesn't seem to work])])
elif test -n "$HOST_CC"; then
  GMP_PROG_CC_FOR_BUILD_WORKS($HOST_CC,
    [CC_FOR_BUILD=$HOST_CC],
    [AC_MSG_ERROR([Specified HOST_CC doesn't seem to work])])
else
  for i in "$CC" "$CC $CFLAGS $CPPFLAGS" cc gcc c89 c99; do
    GMP_PROG_CC_FOR_BUILD_WORKS($i,
      [CC_FOR_BUILD=$i
       break])
  done
  if test -z "$CC_FOR_BUILD"; then
    AC_MSG_ERROR([Cannot find a build system compiler])
  fi
fi
    
AC_ARG_VAR(CC_FOR_BUILD,[build system C compiler])
AC_SUBST(CC_FOR_BUILD)
])


dnl  GMP_PROG_CC_FOR_BUILD_WORKS(cc/cflags[,[action-if-good][,action-if-bad]])
dnl  -------------------------------------------------------------------------
dnl  See if the given cc/cflags works on the build system.
dnl
dnl  It seems easiest to just use the default compiler output, rather than
dnl  figuring out the .exe or whatever at this stage.

AC_DEFUN([GMP_PROG_CC_FOR_BUILD_WORKS],
[AC_MSG_CHECKING([build system compiler $1])
# remove anything that might look like compiler output to our "||" expression
rm -f conftest* a.out b.out a.exe a_out.exe
cat >conftest.c <<EOF
int
main ()
{
  exit(0);
}
EOF
gmp_compile="$1 conftest.c"
cc_for_build_works=no
if AC_TRY_EVAL(gmp_compile); then
  if (./a.out || ./b.out || ./a.exe || ./a_out.exe || ./conftest) >&AC_FD_CC 2>&1; then
    cc_for_build_works=yes
  fi
fi
rm -f conftest* a.out b.out a.exe a_out.exe
AC_MSG_RESULT($cc_for_build_works)
if test "$cc_for_build_works" = yes; then
  ifelse([$2],,:,[$2])
else
  ifelse([$3],,:,[$3])
fi
])


dnl  GMP_PROG_CPP_FOR_BUILD
dnl  ---------------------
dnl  Establish CPP_FOR_BUILD, the build system C preprocessor.
dnl  The choices tried here are the same as AC_PROG_CPP, but with
dnl  CC_FOR_BUILD.

AC_DEFUN([GMP_PROG_CPP_FOR_BUILD],
[AC_REQUIRE([GMP_PROG_CC_FOR_BUILD])
AC_MSG_CHECKING([for build system preprocessor])
if test -z "$CPP_FOR_BUILD"; then
  AC_CACHE_VAL(gmp_cv_prog_cpp_for_build,
  [cat >conftest.c <<EOF
#define FOO BAR
EOF
  for i in "$CC_FOR_BUILD -E" "$CC_FOR_BUILD -E -traditional-cpp" "/lib/cpp"; do
    gmp_compile="$i conftest.c"
    if AC_TRY_EVAL(gmp_compile) >&AC_FD_CC 2>&1; then
      gmp_cv_prog_cpp_for_build=$i
      break
    fi
  done
  rm -f conftest* a.out b.out a.exe a_out.exe
  if test -z "$gmp_cv_prog_cpp_for_build"; then
    AC_MSG_ERROR([Cannot find build system C preprocessor.])
  fi
  ])
  CPP_FOR_BUILD=$gmp_cv_prog_cpp_for_build
fi
AC_MSG_RESULT([$CPP_FOR_BUILD])

AC_ARG_VAR(CPP_FOR_BUILD,[build system C preprocessor])
AC_SUBST(CPP_FOR_BUILD)
])


dnl  GMP_PROG_EXEEXT_FOR_BUILD
dnl  -------------------------
dnl  Determine EXEEXT_FOR_BUILD, the build system executable suffix.
dnl
dnl  The idea is to find what "-o conftest$foo" will make it possible to run
dnl  the program with ./conftest.  On Unix-like systems this is of course
dnl  nothing, for DOS it's ".exe", or for a strange RISC OS foreign file
dnl  system cross compile it can be ",ff8" apparently.  Not sure if the
dnl  latter actually applies to a build-system executable, maybe it doesn't,
dnl  but it won't hurt to try.

AC_DEFUN([GMP_PROG_EXEEXT_FOR_BUILD],
[AC_REQUIRE([GMP_PROG_CC_FOR_BUILD])
AC_CACHE_CHECK([for build system executable suffix],
               gmp_cv_prog_exeext_for_build,
[cat >conftest.c <<EOF
int
main ()
{
  exit (0);
}
EOF
for i in .exe ,ff8 ""; do
  gmp_compile="$CC_FOR_BUILD conftest.c -o conftest$i"
  if AC_TRY_EVAL(gmp_compile); then
    if (./conftest) 2>&AC_FD_CC; then
      gmp_cv_prog_exeext_for_build=$i
      break
    fi
  fi
done
rm -f conftest*
if test "${gmp_cv_prog_exeext_for_build+set}" != set; then
  AC_MSG_ERROR([Cannot determine executable suffix])
fi
])
AC_SUBST(EXEEXT_FOR_BUILD,$gmp_cv_prog_exeext_for_build)
])

