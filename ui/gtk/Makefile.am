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

noinst_LTLIBRARIES = libuigtk.la

INCLUDES = @GTK_CFLAGS@ @LIBSPEC_CFLAGS@

libuigtk_la_SOURCES = binary.c \
		     browse.c \
		     confirm.c \
	             debugger.c \
		     fileselector.c \
		     gtkdisplay.c \
		     gtkjoystick.c \
		     gtkkeyboard.c \
		     gtkmouse.c \
                     gtkui.c \
		     keysyms.c \
		     memory.c \
		     menu_data.c \
	             options.c \
		     picture.c \
		     pixmaps.c \
		     pokefinder.c \
		     rollback.c \
		     roms.c \
		     statusbar.c \
		     stock.c

libuigtk_la_LIBADD = @GTK_LIBS@ -lX11 

BUILT_SOURCES = keysyms.c \
		menu_data.c \
		options.c \
		options.h

keysyms.c: ../../keysyms.pl ../../keysyms.dat
	@PERL@ ../../keysyms.pl gtk ../../keysyms.dat > $@

menu_data.c: ../../perl/cpp-perl.pl ../../menu_data.pl ../../menu_data.dat ../../config.h
	@PERL@ ../../perl/cpp-perl.pl ../../config.h ../../menu_data.dat | @PERL@ ../../menu_data.pl gtk > $@

options.c: ../../perl/cpp-perl.pl ../../config.h options.pl ../options.dat ../../perl/Fuse.pm ../../perl/Fuse/Dialog.pm
	@PERL@ ../../perl/cpp-perl.pl ../../config.h ../options.dat | @PERL@ options.pl - > options.c

options.h: ../../perl/cpp-perl.pl ../../config.h options-header.pl ../options.dat ../../perl/Fuse.pm ../../perl/Fuse/Dialog.pm
	@PERL@ ../../perl/cpp-perl.pl ../../config.h ../options.dat | @PERL@ options-header.pl - > options.h

noinst_HEADERS = gtkinternals.h \
		 options.h \
		 options.pl \
		 options-header.pl
