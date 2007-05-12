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
## E-mail: philip-fuse@shadowmagic.org.uk

AUTOMAKE_OPTIONS = foreign

noinst_LIBRARIES = libwidget.a

libwidget_a_SOURCES = \
	browse.c \
	debugger.c \
	error.c \
	filesel.c \
	memory.c \
	menu.c \
	menu_data.c \
	options.c \
	picture.c \
	pokefinder.c \
	query.c \
	roms.c \
	select.c \
	text.c \
	widget.c

INCLUDES = @GTK_CFLAGS@ @GLIB_CFLAGS@ @LIBSPEC_CFLAGS@

BUILT_SOURCES = menu_data.c \
		options.c \
		options.h \
		fuse.font

menu_data.c: ../perl/cpp-perl.pl ../menu_data.c ../menu_data.pl ../config.h
	@PERL@ ../perl/cpp-perl.pl ../config.h ../menu_data.c | @PERL@ ../menu_data.pl widget > $@

options.c: ../perl/cpp-perl.pl ../config.h options.pl ../ui/options.dat ../perl/Fuse.pm ../perl/Fuse/Dialog.pm
	@PERL@ ../perl/cpp-perl.pl ../config.h ../ui/options.dat | @PERL@ options.pl - > options.c

options.h: ../perl/cpp-perl.pl ../config.h options-header.pl ../ui/options.dat ../perl/Fuse.pm ../perl/Fuse/Dialog.pm
	@PERL@ ../perl/cpp-perl.pl ../config.h ../ui/options.dat | @PERL@ options-header.pl - > options.h

fuse.font: mkfusefont.pl fuse.font.sbf
	@PERL@ mkfusefont.pl fuse.font.sbf > fuse.font

noinst_HEADERS = widget.h widget_internals.h \
		 options.h options.pl options-header.pl

pkgdata_DATA = fuse.font

EXTRA_DIST = mkfusefont.pl \
	     fuse.font.sbf
