## Process this file with automake to produce Makefile.in
## Copyright (c) 1999-2001 Philip Kendall

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

bin_PROGRAMS = fuse

SUBDIRS = libspectrum roms widget z80

fuse_SOURCES = ay.c display.c event.c fbdisplay.c fbkeyboard.c fbui.c\
	fuse.c gtkdisplay.c gtkkeyboard.c gtkui.c keyboard.c keysyms.c\
	machine.c myglib.c osssound.c settings.c snapshot.c sound.c spec128.c\
	spec48.c specplus2.c specplus3.c spectrum.c svgadisplay.c\
	svgakeyboard.c svgaui.c tape.c timer.c xdisplay.c xkeyboard.c xui.c\
	aalibui.c aalibdisplay.c aalibkeyboard.c

BUILT_SOURCES = keysyms.c

keysyms.c: keysyms.pl keysyms.dat
	./keysyms.pl keysyms.dat > keysyms.c

INCLUDES = @GLIB_CFLAGS@ @GTK_CFLAGS@ -DROMSDIR="\"${pkgdatadir}\""
LDADD = @GLIB_LIBS@ @GTK_LIBS@ libspectrum/libspectrum.a widget/libwidget.a z80/libz80.a

noinst_HEADERS = ay.h display.h event.h fbkeyboard.h\
	fuse.h gtkkeyboard.h gtkui.h keyboard.h keysyms.h\
	machine.h myglib.h osssound.h settings.h snapshot.h sound.h spec128.h\
	spec48.h specplus2.h specplus3.h spectrum.h svgakeyboard.h\
	tape.h timer.h types.h ui.h uidisplay.h xdisplay.h\
	xkeyboard.h xui.h aalibui.h aalibkeyboard.h

EXTRA_DIST = AUTHORS README THANKS keysyms.dat keysyms.pl
