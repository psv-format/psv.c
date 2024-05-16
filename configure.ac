m4_define([program_version], [0.1])
AC_INIT([psv], [program_version])
AC_PREREQ([2.65]) # Minimum version number for autoconf build system
AC_CONFIG_SRCDIR([src/main.c]) # Ensure the main source file exists
AC_CONFIG_HEADERS([src/config.h:config_h.in]) # Generate config header
AC_CONFIG_AUX_DIR([build-aux]) # Put autotools auxiliary files into a separate dir to reduce clutter
AC_CONFIG_MACRO_DIRS([m4]) # Put autotools M4 macros files into a separate dir to reduce clutter
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects]) # Initialize Automake with options

# Doxygen Support (https://stackoverflow.com/questions/10682603/generating-and-installing-doxygen-documentation-with-autotools)
AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN"; then
   AC_MSG_WARN([Doxygen not found - continue without Doxygen support])
fi
AC_CHECK_PROGS([DOT], [dot])
if test -z "$DOT"; then
   AC_MSG_ERROR([Doxygen needs dot, please install dot first])
fi
AC_CHECK_PROGS([PDFLATEX], [pdflatex])
if test -z "$PDFLATEX"; then
   AC_MSG_ERROR([Doxygen needs pdflatex program, it is part of TeX http://www.tug.org/texlive/acquire-netinstall.html])
fi
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([docs/Doxyfile])])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([docs/Makefile])])

# Enable all warnings and treat them as errors by default
CFLAGS="$CFLAGS -Wall -Werror"

AC_CONFIG_FILES([Makefile]) # Specify Makefile generation for main directory and src directory
AC_PROG_CC # Find and set up C compiler
AC_OUTPUT # Generate output files
