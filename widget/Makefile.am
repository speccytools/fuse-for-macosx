## Process this file with automake to produce Makefile.in
## Copyright (c) 2001,2002 Philip Kendall

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

noinst_LIBRARIES = libwidget.a

libwidget_a_SOURCES = \
	browse.c \
	debugger.c \
	error.c \
	filesel.c \
	menu.c \
	menu_data.c \
	options.c \
	picture.c \
	roms.c \
	select.c \
	text.c \
	widget.c

INCLUDES = @GTK_CFLAGS@ @GLIB_CFLAGS@ @LIBSPEC_CFLAGS@

BUILT_SOURCES = menu_data.c \
		options.c \
		options.h

menu_data.c: ../perl/cpp-perl.pl ../menu_data.c ../menu_data.pl
	@PERL@ ../perl/cpp-perl.pl ../config.h ../menu_data.c | @PERL@ ../menu_data.pl widget > $@

options.c: options.pl ../ui/options.dat ../perl/Fuse.pm ../perl/Fuse/Dialog.pm
	@PERL@ options.pl ../ui/options.dat > options.c

options.h: options-header.pl ../ui/options.dat ../perl/Fuse.pm ../perl/Fuse/Dialog.pm
	@PERL@ options-header.pl ../ui/options.dat > options.h

noinst_HEADERS = widget.h widget_internals.h \
		 options.h options.pl options-header.pl
