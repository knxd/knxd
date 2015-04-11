# Copyright (C) 2014 Luis R. Rodriguez <mcgrof@suse.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

AC_DEFUN([AX_ARG_DEFAULT_ENABLE], [
AC_ARG_ENABLE([$1], AS_HELP_STRING([--disable-$1], [$2 (default is ENABLED)]))
AX_PARSE_VALUE([$1], [y])
])

AC_DEFUN([AX_ARG_DEFAULT_DISABLE], [
AC_ARG_ENABLE([$1], AS_HELP_STRING([--enable-$1], [$2 (default is DISABLED)]))
AX_PARSE_VALUE([$1], [n])
])

dnl This function should not be called outside of this file
AC_DEFUN([AX_PARSE_VALUE], [
AS_IF([test "x$enable_$1" = "xno"], [
    ax_cv_$1="n"
], [test "x$enable_$1" = "xyes"], [
    ax_cv_$1="y"
], [test -z $ax_cv_$1], [
    ax_cv_$1="$2"
])
$1=$ax_cv_$1
AC_SUBST($1)])
