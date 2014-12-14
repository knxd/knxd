dnl ##  AC_COMPILER_OPTION based on: GNU Pth - The GNU Portable Threads
dnl ##  Copyright (c) 1999-2006 Ralf S. Engelschall <rse@engelschall.com>
dnl ##
dnl ##  This file is part of GNU Pth, a non-preemptive thread scheduling
dnl ##  library which can be found at http://www.gnu.org/software/pth/.
dnl ##
dnl ##  This library is free software; you can redistribute it and/or
dnl ##  modify it under the terms of the GNU Lesser General Public
dnl ##  License as published by the Free Software Foundation; either
dnl ##  version 2.1 of the License, or (at your option) any later version.
dnl ##
dnl ##  This library is distributed in the hope that it will be useful,
dnl ##  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl ##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
dnl ##  Lesser General Public License for more details.
dnl ##
dnl ##  You should have received a copy of the GNU Lesser General Public
dnl ##  License along with this library; if not, write to the Free Software
dnl ##  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
dnl ##  USA, or contact Ralf S. Engelschall <rse@engelschall.com>.
dnl ##
dnl ##  Check whether compiler option works
dnl ##
dnl ##  configure.ac:
dnl ##    AC_COMPILER_OPTION(<name>, <display>, <option>,
dnl ##                       <action-success>, <action-failure>)
dnl ##

AC_DEFUN([AC_COMPILER_OPTION],[dnl
AC_MSG_CHECKING([for compiler option $2])
AC_CACHE_VAL(ac_cv_compiler_option_$1,[
cat >conftest.$ac_ext <<EOF
int main() { return 0; }
EOF
${CXX} -c $CFLAGS $CPPFLAGS $3 conftest.$ac_ext 1>conftest.out 2>conftest.err
if test $? -ne 0 -o -s conftest.err; then
     ac_cv_compiler_option_$1=no
else
     ac_cv_compiler_option_$1=yes
fi
rm -f conftest.$ac_ext conftest.out conftest.err
])dnl
if test ".$ac_cv_compiler_option_$1" = .yes; then
    ifelse([$4], , :, [$4])
else
    ifelse([$5], , :, [$5])
fi
AC_MSG_RESULT([$ac_cv_compiler_option_$1])
])dnl

dnl ## END
