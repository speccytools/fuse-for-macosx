## Process this file with automake to produce Makefile.in
## Copyright (c) 2001-2003 Philip Kendall

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
## E-mail: pak21-fuse@srcf.ucam.org
## Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

AUTOMAKE_OPTIONS = foreign

noinst_LIBRARIES = libuigtk.a

INCLUDES = @GTK_CFLAGS@ @LIBSPEC_CFLAGS@

libuigtk_a_SOURCES = binary.c \
		     browse.c \
		     confirm.c \
	             debugger.c \
                     error.c \
		     fileselector.c \
		     gtkdisplay.c \
		     gtkjoystick.c \
		     gtkkeyboard.c \
                     gtkui.c \
	             options.c \
		     keysyms.c \
		     picture.c \
		     pokefinder.c \
		     roms.c \
		     statusbar.c

BUILT_SOURCES = keysyms.c options.c options.h

keysyms.c: ../../keysyms.pl ../../keysyms.dat
	@PERL@ ../../keysyms.pl gtk ../../keysyms.dat > $@

options.c: options.pl ../options.dat ../../perl/Fuse.pm ../../perl/Fuse/Dialog.pm
	@PERL@ options.pl ../options.dat > options.c

options.h: options-header.pl ../options.dat ../../perl/Fuse.pm ../../perl/Fuse/Dialog.pm
	@PERL@ options-header.pl ../options.dat > options.h

noinst_HEADERS = gtkinternals.h \
		 options.h \
		 options.pl \
		 options-header.pl
