## Process this file with automake to produce Makefile.in
## Copyright (c) 2007 Stuart Brady

## $Id: Makefile.am 2889 2007-05-26 17:45:08Z zubzero $

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

noinst_LIBRARIES = libdisk.a

INCLUDES = @LIBSPEC_CFLAGS@ @GTK_CFLAGS@ @GLIB_CFLAGS@

libdisk_a_SOURCES = plusd.c \
		    wd1770.c

noinst_HEADERS = plusd.h \
		 wd1770.h
