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
## E-mail: pak@ast.cam.ac.uk
## Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = fuse

fuse_SOURCES = ay.c display.c event.c fuse.c gtk.c gtkdisplay.c gtkkeyboard.c\
	gtkui.c keyboard.c keysyms.c snapshot.c spec128.c spec48.c specplus2.c\
	specplus3.c spectrum.c svga.c svgadisplay.c svgakeyboard.c svgaui.c\
	tape.c timer.c x.c xdisplay.c xkeyboard.c xui.c z80.c z80_ops.c
INCLUDES = @GLIB_CFLAGS@ @GTK_CFLAGS@
LDADD = @GLIB_LIBS@ @GTK_LIBS@

noinst_HEADERS = ay.h display.h event.h fuse.h gtk.h gtkdisplay.h\
	gtkkeyboard.h gtkui.h keyboard.h keysyms.h snapshot.h spec128.h\
	spec48.h specplus2.h specplus3.h spectrum.h svga.h svgadisplay.h\
	svgakeyboard.h svgaui.h tape.h timer.h types.h x.h xdisplay.h\
	xkeyboard.h xui.h z80.h z80_cb.c z80_ddfd.c z80_ddfdcb.c z80_ed.c\
	z80_macros.h 

EXTRA_DIST = 128-0.rom 128-1.rom 48.rom	plus2-0.rom plus2-1.rom plus3-0.rom\
	plus3-1.rom plus3-2.rom plus3-3.rom
