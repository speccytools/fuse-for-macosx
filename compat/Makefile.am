## Process this file with automake to produce Makefile.in
## Copyright (c) 2003-2007 Philip Kendall

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

AM_CPPFLAGS = -I$(srcdir)/..

SUBDIRS = @COMPAT_OSNAME@

DIST_SUBDIRS = amiga \
	       morphos \
	       unix \
	       win32

noinst_LIBRARIES = libcompat.a

libcompat_a_SOURCES =

EXTRA_libcompat_a_SOURCES = dirname.c \
			    getopt.c \
			    getopt1.c \
			    mkstemp.c

libcompat_a_LIBADD = $(COMPAT_LIBADD)
libcompat_a_DEPENDENCIES = $(COMPAT_LIBADD)

INCLUDES = @GLIB_CFLAGS@ @GTK_CFLAGS@ @LIBSPEC_CFLAGS@

noinst_HEADERS = getopt.h
