m4_define([program_version], [0.1])
AC_INIT([psv], [program_version])
AC_PREREQ([2.65]) # Minimum version number for autoconf build system
AC_CONFIG_SRCDIR([src/main.c]) # Ensure the main source file exists
AC_CONFIG_HEADERS([src/config.h:config_h.in]) # Generate config header
AC_CONFIG_AUX_DIR([build-aux]) # Put autotools auxiliary files into a seperate dir to reduce clutter
AC_CONFIG_MACRO_DIRS([m4]) # Put autotools M4 macros files into a seperate dir to reduce clutter
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects]) # Initialize Automake with options
AC_CONFIG_FILES([Makefile]) # Specify Makefile generation for main directory and src directory
AC_PROG_CC # Find and set up C compiler
AC_OUTPUT # Generate output files
