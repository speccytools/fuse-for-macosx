## Process this file with automake to produce Makefile.in
## Copyright (c) 2001-2004 Philip Kendall

## $Id$

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along
## with this program; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
##
## Author contact information:
##
## E-mail: philip-fuse@shadowmagic.org.uk

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(srcdir)/../..

noinst_LIBRARIES = libuiwin32.a

INCLUDES = @GLIB_CFLAGS@ @LIBSPEC_CFLAGS@

libuiwin32_a_SOURCES = debugger.c \
		       debugger.rc \
		       error.c \
		       keysyms.c \
		       menu_data.c \
		       menu_data.rc \
		       pokefinder.rc \
		       statusbar.c \
		       win32display.c \
		       win32keyboard.c \
		       win32ui.c \
		       win32ui.rc

BUILT_SOURCES = keysyms.c

keysyms.c: $(srcdir)/../../keysyms.pl $(srcdir)/../../keysyms.dat
	@PERL@ -I$(srcdir)/../../perl $(srcdir)/../../keysyms.pl win32 $(srcdir)/../../keysyms.dat > $@.tmp && mv $@.tmp $@

noinst_HEADERS = debugger.h \
		 menu_data.h \
		 pokefinder.h \
		 win32display.h \
		 win32internals.h \
		 win32keyboard.h

CLEANFILES = keysyms.c
