## Process this file with automake to produce Makefile.in
## Copyright (c) 1999-2002 Philip Kendall

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

SUBDIRS = hacking lib man perl roms ui widget z80

fuse_SOURCES = ay.c display.c event.c\
	fuse.c keyboard.c machine.c osssound.c rzx.c settings.c\
	snapshot.c sound.c spec128.c spec48.c specplus2.c specplus2a.c\
	specplus3.c spectrum.c sunsound.c tape.c timer.c ui.c uidisplay.c\
	utils.c	joystick.c printer.c tc2048.c scld.c screenshot.c

BUILT_SOURCES = settings.c

settings.c: settings.pl settings.dat
	@PERL@ settings.pl settings.dat > settings.c

INCLUDES = @GLIB_CFLAGS@ @GTK_CFLAGS@ @LIBSPEC_CFLAGS@ @DSK_CFLAGS@\
@XML_CFLAGS@ -DDATADIR="\"${pkgdatadir}\""
LDADD = @GLIB_LIBS@ @GTK_LIBS@ @PNG_LIBS@ @XML_LIBS@\
ui/aalib/libuiaalib.a ui/fb/libuifb.a ui/gtk/libuigtk.a ui/svga/libuisvga.a\
ui/xlib/libuixlib.a widget/libwidget.a z80/libz80.a @DSKLIBS@ @LIBSPEC_LIBS@

noinst_HEADERS = ay.h display.h event.h\
	fuse.h keyboard.h keysyms.h\
	machine.h osssound.h rzx.h screenshot.h settings.h snapshot.h sound.h\
        spec128.h spec48.h specplus2.h specplus2a.h specplus3.h spectrum.h\
	sunsound.h tape.h timer.h types.h utils.h\
	joystick.h printer.h tc2048.h scld.h

EXTRA_DIST = AUTHORS README THANKS keysyms.dat keysyms.pl settings.pl
