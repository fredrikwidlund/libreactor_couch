ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = -std=gnu11 -O3 -flto -fuse-linker-plugin
AM_LDFLAGS = -static

SOURCE_FILES = \
src/reactor_couch/reactor_couch.c

HEADER_FILES = \
src/reactor_couch/reactor_couch.h

MAIN_HEADER_FILES = \
src/reactor_couch.h

AUTOMAKE_OPTIONS = subdir-objects
lib_LTLIBRARIES = libreactor_couch.la
libreactor_couch_la_SOURCES = $(SOURCE_FILES) $(HEADER_FILES)

headerfilesdir = $(includedir)/reactor_couch
headerfiles_HEADERS = $(HEADER_FILES)

mainheaderdir = $(includedir)
mainheader_HEADERS = $(MAIN_HEADER_FILES)

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in libreactor_couch-?.?.?.tar.gz
maintainer-clean-local:; rm -rf autotools m4 libreactor_couch-?.?.?

CLEANFILES = {.,src/reactor_couch,test}/*.{gcno,gcda,gcov}
