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
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
##
## Author contact information:
##
## E-mail: philip-fuse@shadowmagic.org.uk

AUTOMAKE_OPTIONS = foreign

noinst_LIBRARIES = libuisvga.a

INCLUDES = @GLIB_CFLAGS@ @LIBSPEC_CFLAGS@

libuisvga_a_SOURCES = svgadisplay.c \
		      svgajoystick.c \
		      svgakeyboard.c \
		      svgaui.c \
		      keysyms.c

BUILT_SOURCES = keysyms.c

keysyms.c: ../../keysyms.pl ../../keysyms.dat
	@PERL@ ../../keysyms.pl svga ../../keysyms.dat > $@

noinst_HEADERS = svgadisplay.h svgakeyboard.h
