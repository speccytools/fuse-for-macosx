## Process this file with automake to produce Makefile.in
## Copyright (c) 1999-2004 Philip Kendall

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

SUBDIRS = compat \
	  debugger \
	  hacking \
	  lib \
	  machines \
	  man \
	  perl \
	  pokefinder \
	  roms \
	  sound \
	  ui \
	  @WIDGET@ \
	  z80

DIST_SUBDIRS = compat \
	       debugger \
	       hacking \
	       lib \
	       man \
	       perl \
	       pokefinder \
	       roms \
	       sound \
	       ui \
	       widget \
	       z80

fuse_SOURCES = ay.c \
	dck.c \
	display.c \
	event.c \
	fuse.c \
	if2.c \
	input.c \
	joystick.c \
	keyboard.c \
	machine.c \
	memory.c \
	menu.c \
	periph.c \
	printer.c \
	psg.c \
	rzx.c \
	scld.c \
	screenshot.c \
	settings.c \
	simpleide.c \
	slt.c \
	snapshot.c \
	sound.c \
	spectrum.c \
	tape.c \
	timer.c \
	trdos.c \
	ui.c \
	uidisplay.c \
	utils.c \
	zxatasp.c \
	zxcf.c

fuse_LDADD = debugger/libdebugger.a \
@UI_LIBS@ \
machines/libmachines.a \
pokefinder/libpokefinder.a \
sound/libsound.a \
ui/scaler/libscaler.a \
debugger/libdebugger.a \
z80/libz80.a \
@DSKLIBS@ \
@LIBSPEC_LIBS@ \
@GLIB_LIBS@ \
@GTK_LIBS@ \
@PNG_LIBS@ \
@XML_LIBS@ \
compat/libcompat.a

fuse_DEPENDENCIES = @UI_LIBS@ \
		    debugger/libdebugger.a \
		    machines/libmachines.a \
		    sound/libsound.a \
	            z80/libz80.a

BUILT_SOURCES = settings.c settings.h

settings.c: settings.pl settings.dat
	@PERL@ settings.pl settings.dat > settings.c

settings.h: settings-header.pl settings.dat
	@PERL@ settings-header.pl settings.dat > settings.h

INCLUDES = @GLIB_CFLAGS@ @GTK_CFLAGS@ @LIBSPEC_CFLAGS@ @DSK_CFLAGS@\
@XML_CFLAGS@ -DFUSEDATADIR="\"${pkgdatadir}\""

noinst_HEADERS = ay.h \
	compat.h \
	dck.h \
	display.h \
	event.h \
	fuse.h \
	keyboard.h \
	machine.h \
	memory.h \
	menu.h \
	periph.h \
	psg.h \
	rzx.h \
	screenshot.h \
	settings.h \
	simpleide.h \
	slt.h \
	snapshot.h \
	sound.h \
	spectrum.h \
	tape.h \
	timer.h \
	trdos.h \
	utils.h \
	joystick.h \
	printer.h \
	scld.h \
	zxatasp.h \
	zxcf.h

EXTRA_DIST = AUTHORS README THANKS keysyms.dat keysyms.pl settings.pl \
	     settings-header.pl settings.dat
