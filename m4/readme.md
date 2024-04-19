# Autotools M4 Macro Directory
This directory contains custom or third-party macros used by Autotools.
## Purpose
These macros are included in the Autotools build process via the `AC_CONFIG_MACRO_DIRS([m4])` directive in the `configure.ac` file.
It is also accessible within  `configure.ac` as well as m4 does a double pass as well.
## Usage
Ensure to include `-I m4` in the `ACLOCAL_AMFLAGS` variable within the `Makefile.am` file to instruct `aclocal` to search for macros in this directory.
