#!/usr/bin/perl -w

# keysyms.pl: generate keysyms.c from keysyms.dat
# Copyright (c) 2000-2003 Philip Kendall, Matan Ziv-Av, Russell Marks,
#			  Fredrick Meunier

# $Id$

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 49 Temple Place, Suite 330, Boston, MA 02111-1307 USA

# Author contact information:

# E-mail: pak21-fuse@srcf.ucam.org
# Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

use strict;

use lib '../../perl';

use Fuse;

my $ui = shift;
$ui = 'gtk' unless defined $ui;

die "$0: unrecognised user interface: $ui\n"
  unless 0 < grep { $ui eq $_ } ( 'ggi', 'gtk', 'x', 'svga', 'fb', 'sdl' );

# The define and header files needed for each UI
my %preprocessor_directives = (

    fb   => [ 'FB',   [ ],                   ],
    ggi  => [ 'GGI',  [ 'ggi/ggi.h' ]        ],
    gtk  => [ 'GTK',  [ 'gdk/gdkkeysyms.h' ] ],
    sdl  => [ 'SDL',  [ 'sdl.h' ],	     ],
    svga => [ 'SVGA', [ 'vgakeyboard.h' ]    ],
    x    => [ 'X',    [ 'X11/keysym.h' ]     ],

);

# The GGI keysyms which start with `GIIK', not `GIIUC'
my %ggi_giik_keysyms = map { $_ => 1 } qw(

    AltL
    AltR
    CapsLock
    CtrlL
    CtrlR
    Down
    End
    Home
    HyperL
    HyperR
    Left
    Menu
    MetaL
    MetaR
    PageUp
    PageDown
    Right
    ShiftL
    ShiftR
    SuperL
    SuperR
    Up

);

# Some keysyms which don't easily do the Xlib -> GGI conversion
my %ggi_keysyms = (

    Control_L   => 'CtrlL',
    Control_R   => 'CtrlR',
    Mode_switch => 'Menu',
    numbersign  => 'DoubleQuote',

);

# Some keysyms which don't easily do the Xlib -> SVGAlib conversion
my %svga_keysyms = (

    CAPS_LOCK  => 'CAPSLOCK',
    NUMBERSIGN => 'BACKSLASH',	# That's what `#' returns on a UK keyboard!
    RETURN     => 'ENTER',

    LEFT       => 'CURSORBLOCKLEFT',
    DOWN       => 'CURSORBLOCKDOWN',
    UP         => 'CURSORBLOCKUP',
    RIGHT      => 'CURSORBLOCKRIGHT',
);

# Some keysyms which don't easily do the Xlib -> SDL conversion
my %sdl_keysyms = (

    CAPS_LOCK  => 'CAPSLOCK',
    NUMBERSIGN => 'HASH',
    EQUAL => 'EQUALS',
    APOSTROPHE => 'QUOTE',
    MODE_SWITCH => 'MENU',
);

# Translation table for any UI which uses keyboard mode K_MEDIUMRAW
my @cooked_keysyms = (
    # 0x00
    undef, 'ESCAPE', '1', '2', '3', '4', '5', '6',
    '7', '8', '9', '0', 'MINUS', 'EQUAL', 'BACKSPACE', 'TAB',
    # 0x10
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', 'BRACKETLEFT', 'BRACKETRIGHT', 'RETURN', 'CONTROL_L', 'A', 'S',
    # 0x20
    'D', 'F', 'G', 'H', 'J', 'K', 'L', 'SEMICOLON',
    'APOSTROPHE', 'GRAVE', 'SHIFT_L', 'NUMBERSIGN', 'Z', 'X', 'C', 'V',
    # 0x30
    'B', 'N', 'M', 'COMMA', 'PERIOD', 'SLASH', 'SHIFT_R', 'KB_MULTIPLY',
    'ALT_L', 'SPACE', 'CAPS_LOCK', 'F1', 'F2', 'F3', 'F4', 'F5',
    # 0x40
    'F6', 'F7', 'F8', 'F9', 'F10', 'NUM_LOCK', 'SCROLL_LOCK', 'KP_7',
    'KP_8', 'KP_9', 'KP_MINUS', 'KP_4', 'KP_5', 'KP_6', 'KP_PLUS', 'KP_1',
    # 0x50
    'KP_2', 'KP_3', 'KP_0', 'KP_DECIMAL', undef, undef, 'BACKSLASH', 'F11',
    'F12', undef, undef, undef, undef, undef, undef, undef,
    # 0x60
    'KP_ENTER', 'CONTROL_R', 'KP_DIVIDE', 'PRINT', 'ALT_R', undef, 'HOME','UP',
    'PAGE_UP', 'LEFT', 'RIGHT', 'END', 'DOWN', 'PAGE_DOWN', 'INSERT', 'DELETE',
    # 0x70
    undef, undef, undef, undef, undef, undef, undef, 'BREAK',
    undef, undef, undef, undef, undef, 'WIN_L', 'WIN_R', 'MENU'
);

sub ggi_keysym ($) {

    my $keysym = shift;

    # Some specific translations
    $keysym = $ggi_keysyms{$keysym} if $ggi_keysyms{$keysym};

    # GGI keysyms start with a capitial letter
    $keysym = ucfirst $keysym;

    # and have no underscores
    $keysym =~ s/_//g;

    if( $ggi_giik_keysyms{ $keysym } ) {
	$keysym = "GIIK_$keysym";
    } else {
	$keysym = "GIIUC_$keysym";
    }

    return $keysym;
}

sub gtk_keysym ($) { "GDK_$_[0]" }
    
sub sdl_keysym ($) {

    my $keysym = shift;

    if ( $keysym =~ /[a-zA-Z][a-z]+/ ) {
	$keysym =~ tr/a-z/A-Z/;
    }
    $keysym =~ s/^CONTROL/CTRL/;
    $keysym =~ s/(.*)_L$/L$1/;
    $keysym =~ s/(.*)_R$/R$1/;
    $keysym =~ s/^PAGE_/PAGE/;
    
    # Some specific translations
    $keysym = $sdl_keysyms{$keysym} if $sdl_keysyms{$keysym};

    # All the magic #defines start with `SDLK_'
    $keysym = "SDLK_$keysym";

    return $keysym;
}

sub svgalib_keysym ($) {
    
    my $keysym = shift;

    # General translations
    $keysym =~ tr/a-z/A-Z/;
    $keysym =~ s/(.*)_L$/LEFT$1/;
    $keysym =~ s/(.*)_R$/RIGHT$1/;
    $keysym =~ s/META$/WIN/;		# Fairly sensible mapping
    $keysym =~ s/^PAGE_/PAGE/;

    # Some specific translations
    $keysym = $svga_keysyms{$keysym} if $svga_keysyms{$keysym};

    # All the magic #defines start with `SCANCODE_'
    $keysym = "SCANCODE_$keysym";
    
    # Apart from this one :-)
    $keysym = "127" if $keysym eq 'SCANCODE_MODE_SWITCH';

    return $keysym;
}

sub xlib_keysym ($) { "XK_$_[0]" }

my @keys;
while(<>) {

    next if /^\s*$/;
    next if /^\s*\#/;

    chomp;

    my( $keysym, $key1, $key2 ) = split /\s*,\s*/;

    push @keys, [ $keysym, $key1, $key2 ]

}

my $declare = "const keysyms_key_info keysyms_data[] =\n{";

print Fuse::GPL(
    'keysyms.c: keysym to Spectrum key mappings for both Xlib and GDK',
    "2000-2003 Philip Kendall, Matan Ziv-Av, Russell Marks,\n" .
    "                           Fredrick Meunier, Catalin Mihaila"  ),
    << "CODE";

/* This file is autogenerated from keysyms.dat by keysyms.pl.
   Do not edit unless you know what you're doing! */

#include <config.h>

#ifdef UI_$preprocessor_directives{$ui}[0]

#include <stdlib.h>

#include "keyboard.h"
#include "ui/ui.h"

CODE

# Comment to unbreak Emacs' perl mode

foreach my $header ( @{ $preprocessor_directives{$ui}[1] } ) {
    print "#include <$header>\n";
}

print "\nconst keysyms_key_info keysyms_data[] = {\n\n";

if( $ui eq 'ggi' ) {

    foreach( @keys ) {

	my( $keysym, $key1, $key2 ) = @$_;

	printf "  { %-17s , KEYBOARD_%-9s KEYBOARD_%-6s },\n",
	    ggi_keysym( $keysym ), "$key1,", $key2;
    }

} elsif( $ui eq 'gtk' ) {

    foreach ( @keys ) {
	my( $keysym, $key1, $key2 ) = @$_;

	printf "  { %-15s , KEYBOARD_%-9s KEYBOARD_%-6s },\n",
	    gtk_keysym( $keysym ), "$key1,", $key2;
    }

} elsif( $ui eq 'x' ) {

    foreach ( @keys ) {
	my( $keysym, $key1, $key2 ) = @$_;

	printf "  { %-14s , KEYBOARD_%-9s KEYBOARD_%-6s },\n",
	    xlib_keysym( $keysym ), "$key1,", $key2;
    }

} elsif( $ui eq 'svga' ) {

    foreach( @keys ) {

	my( $keysym, $key1, $key2 ) = @$_;

	# svgalib doesn't believe in these keys
	next if( $keysym =~ /^Super_/ or $keysym =~ /^Hyper_/ );

	if( $keysym =~ /WIN$/ ) {
	    print "#ifdef $keysym\n";
	}

	printf "  { %-25s , KEYBOARD_%-9s KEYBOARD_%-6s },\n",
	    svgalib_keysym( $keysym ), "$key1,", $key2;

	if( $keysym =~ /WIN$/ ) {
	    print "#endif                          /* #ifdef $keysym */\n";
	}
    }

} elsif( $ui eq 'sdl' ) {

    foreach( @keys ) {

        my( $keysym, $key1, $key2 ) = @$_;

        # SDL doesn't believe in these keys
        next if( $keysym =~ /^Hyper_/ );

        printf "  { %-25s , KEYBOARD_%-9s KEYBOARD_%-6s },\n",
          sdl_keysym( $keysym ), "$key1,", $key2;

    }
  
} elsif( $ui eq 'fb' ) {

    foreach (@keys) {

	my ($keysym, $key1, $key2) = @$_;

	# General translations
	$keysym =~ tr/a-z/A-Z/;
	substr( $keysym, 0, 4 ) = 'WIN' if substr( $keysym, 0, 5 ) eq 'META_';
	$keysym = 'MENU' if $keysym eq 'MODE_SWITCH';

	my $i;
	for( $i = 0; $i <= $#cooked_keysyms; $i++) {
	    if( defined $cooked_keysyms[$i] and
		$cooked_keysyms[$i] eq $keysym ) {
		printf "  { %3i, KEYBOARD_%-9s KEYBOARD_%-6s },\n", $i,
		    "$key1,", $key2;
		last;
	    }
	}
    }

}

print << "CODE";

  { 0, 0, 0 }                   /* End marker: DO NOT MOVE! */

};

#endif
CODE

