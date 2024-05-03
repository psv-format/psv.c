bin_PROGRAMS = psv
psv_SOURCES = src/main.c src/psv.c src/psv.h src/psv_json.c src/psv_json.h src/cJSON.c src/cJSON.h

check_PROGRAMS = unit_test
unit_test_SOURCES = tests/unit_test.c

# ACLOCAL_AMFLAGS specifies additional flags for aclocal.
# -I m4: Include the m4 directory for additional macros.
# --install: Automatically install missing macros into the system-wide directory.
ACLOCAL_AMFLAGS = -I m4 --install

# Add a target to generate Doxygen documentation
doxygen:
	doxygen Doxyfile

# Define a phony target for cleaning Doxygen documentation
clean-doxygen:
	rm -rf docs

# Add Doxygen to the list of targets to be built
all-local: doxygen

# Setup Python Enviroments
.PHONY: generate
update-cbor-tags:
	python3 -m venv venv
	. venv/bin/activate
	venv/bin/pip install -r requirements.txt
	cd c; ./c_header_cbor.py
