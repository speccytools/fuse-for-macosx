## Process this file with automake to produce Makefile.in
## Copyright (c) 2008 Philip Kendall
## Copyright (c) 2015 Sergio Baldoví

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
## You should have received a copy of the GNU General Public License along
## with this program; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
##
## Author contact information:
##
## E-mail: philip-fuse@shadowmagic.org.uk

fuse_SOURCES += unittests/unittests.c

noinst_HEADERS += unittests/unittests.h

## The display code tester

noinst_PROGRAMS += unittests/displaytest
noinst_PROGRAMS += unittests/sdl2displaytest
noinst_PROGRAMS += unittests/sdl2joysticktest
noinst_PROGRAMS += unittests/sdl2mousetest

unittests_displaytest_SOURCES = unittests/displaytest.c display.c
unittests_displaytest_LDADD = $(LIBSPECTRUM_LIBS) $(GLIB_LIBS)
unittests_displaytest_CPPFLAGS = $(AM_CPPFLAGS) -DDISPLAYTEST

unittests_sdl2displaytest_SOURCES = \
        unittests/sdl2displaytest.c \
        ui/sdl2/sdl2_display_internal.c
unittests_sdl2displaytest_LDADD = $(LIBSPECTRUM_LIBS)
unittests_sdl2displaytest_CPPFLAGS = $(AM_CPPFLAGS)

unittests_sdl2joysticktest_SOURCES = \
        unittests/sdl2joysticktest.c \
        ui/sdl2/sdl2_joystick_internal.c
unittests_sdl2joysticktest_LDADD = $(LIBSPECTRUM_LIBS)
unittests_sdl2joysticktest_CPPFLAGS = $(AM_CPPFLAGS)

unittests_sdl2mousetest_SOURCES = \
        unittests/sdl2mousetest.c \
        ui/sdl2/sdl2_mouse_internal.c
unittests_sdl2mousetest_LDADD = $(LIBSPECTRUM_LIBS)
unittests_sdl2mousetest_CPPFLAGS = $(AM_CPPFLAGS)
