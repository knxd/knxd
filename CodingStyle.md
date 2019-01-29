# KNXD Coding Style

KNXD is written in C++. It has originally been formatted using "indent",
which unfortunately knows only C but next to nothing about C++, thus some
of the spacing in the code doesn't look at all like idiomatic C++.

Please try to adhere to the formatting of existing code as much as
possible, or at least as much as you're comfortable with. Specifically,
please adhere to the two/four-space indent scheme:

    if (x)
      {
        more_statements;
        more_statements;
      }
    else
      single_statement;

## "#ifdef"s

All "#ifdef" statements should be either

* guard macros (#ifndef foo / #define foo … / #endif)

* conditional on a specific compiler (cf. "#ifdef __gnuc__" in
  src/common/types.h)

* conditional on "HAVE_XXX" as discovered by autoconf.

Operating system specific conditionals are not allowed: they tend
to stop working when you switch compilers, cross-compile, or build
with an alternate libc.

