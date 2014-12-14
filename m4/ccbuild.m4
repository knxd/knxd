AC_DEFUN([CC_FOR_BUILD_OPTS],
[
if test -z "$CFLAGS_FOR_BUILD"; then
  CFLAGS_FOR_BUILD="-g -O2"
fi
if test -z "$LDFLAGS_FOR_BUILD"; then
  LDFLAGS_FOR_BUILD="-g"
fi
if test -z "$CPPFLAGS_FOR_BUILD"; then
  CPPFLAGS_FOR_BUILD=""
fi
if test -z "$LIBS_FOR_BUILD"; then
  LIBS_FOR_BUILD=""
fi
    
AC_ARG_VAR(CFLAGS_FOR_BUILD,[build system C compiler flags])
AC_ARG_VAR(LDFLAGS_FOR_BUILD,[build system C linker flags])
AC_ARG_VAR(CPPFLAGS_FOR_BUILD,[build system preprocessor flags])
AC_ARG_VAR(LIBS_FOR_BUILD,[build system LIBS variable])
AC_SUBST(CPPFLAGS_FOR_BUILD)
AC_SUBST(CFLAGS_FOR_BUILD)
AC_SUBST(LDFLAGS_FOR_BUILD)
AC_SUBST(LIBS_FOR_BUILD)
])
