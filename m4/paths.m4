# paths.m4 - Macros to help expand configure variables early   -*- Autoconf -*-
#
# Copyright (C) 2014 Luis R. Rodriguez <mcgrof@suse.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

dnl these helpers expands generic autotools variables early so we can use
dnl these for substitutions with AC_CONFIG_FILES().
AC_DEFUN([AX_LOCAL_EXPAND_CONFIG], [
test "x$prefix" = "xNONE" && prefix=$ac_default_prefix
test "x$exec_prefix" = "xNONE" && exec_prefix=$ac_default_prefix

dnl BINDIR=$prefix/bin
BINDIR=`eval echo $bindir`
AC_SUBST(BINDIR)

SBINDIR=`eval echo $sbindir`
AC_SUBST(SBINDIR)

LIBEXEC=`eval echo $libexec`
AC_SUBST(LIBEXEC)

LIBDIR=`eval echo $libdir`
AC_SUBST(LIBDIR)

SHAREDIR=`eval echo $prefix/share`
AC_SUBST(SHAREDIR)

dnl Not part of a ./configure option, does LSB define it?
RUNDIR="/var/run"
AC_SUBST(RUNDIR)

CONFIG_DIR=/etc
AC_SUBST(CONFIG_DIR)
])
