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
## E-mail: philip-fuse@shadowmagic.org.uk

AUTOMAKE_OPTIONS = foreign

noinst_LIBRARIES = libz80.a

libz80_a_SOURCES = z80.c z80_ops.c

INCLUDES = @GTK_CFLAGS@ @GLIB_CFLAGS@ @LIBSPEC_CFLAGS@

BUILT_SOURCES = opcodes_base.c z80_cb.c z80_ddfd.c z80_ddfdcb.c z80_ed.c

opcodes_base.c: z80.pl opcodes_base.dat
	@PERL@ z80.pl opcodes_base.dat > opcodes_base.c

z80_cb.c: z80.pl opcodes_cb.dat
	@PERL@ z80.pl opcodes_cb.dat > z80_cb.c

z80_ddfd.c: z80.pl opcodes_ddfd.dat
	@PERL@ z80.pl opcodes_ddfd.dat > z80_ddfd.c

z80_ddfdcb.c: z80.pl opcodes_ddfd.dat
	@PERL@ z80.pl opcodes_ddfdcb.dat > z80_ddfdcb.c

z80_ed.c: z80.pl opcodes_ed.dat
	@PERL@ z80.pl opcodes_ed.dat > z80_ed.c

noinst_HEADERS = z80.h \
		 z80_macros.h 

EXTRA_DIST = opcodes_base.c \
	     opcodes_base.dat \
	     opcodes_cb.dat \
	     opcodes_ddfd.dat \
	     opcodes_ddfdcb.dat \
	     opcodes_ed.dat \
	     z80.pl \
	     z80_cb.c \
	     z80_ddfd.c \
	     z80_ddfdcb.c \
	     z80_ed.c

## The core tester

SUBDIRS = tests

noinst_PROGRAMS = coretest

coretest_SOURCES = coretest.c z80.c
coretest_LDADD = z80_coretest.o @LIBSPEC_LIBS@

coretest.o: coretest.c
	$(COMPILE) -DCORETEST -c coretest.c -o coretest.o

z80_coretest.o: z80_ops.c
	$(COMPILE) -DCORETEST -c z80_ops.c -o z80_coretest.o

test: coretest harness.pl
	@PERL@ harness.pl tests/*.in
