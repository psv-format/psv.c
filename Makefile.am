bin_PROGRAMS = psv
psv_SOURCES = src/main.c src/hello.h

check_PROGRAMS = unit_test
unit_test_SOURCES = tests/unit_test.c src/hello.h

# ACLOCAL_AMFLAGS specifies additional flags for aclocal.
# -I m4: Include the m4 directory for additional macros.
# --install: Automatically install missing macros into the system-wide directory.
ACLOCAL_AMFLAGS = -I m4 --install
