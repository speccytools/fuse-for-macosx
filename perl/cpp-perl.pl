#!/usr/bin/perl

# cpp-perl.pl: trivial preprocessor with just about enough intelligence
# to parse config.h

# Copyright (c) 2004 Philip Kendall

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

# Philip Kendall <philip-fuse@shadowmagic.org.uk>

use warnings;
use strict;

use IO::File;

my %defines;
my @conditions;

sub parse_file ($;$) {
    my( $filename, $inhibit ) = @_;

    $inhibit ||= 0;

    my $fh = new IO::File( '<' . $filename )
	or die "Couldn't open '$filename': $!";

    while( <$fh> ) {

	if( /^#define ([[:alnum:]_]+) (.*)$/ ) {
	    $defines{$1} = $2;
	    next;
	}

	if( /^#ifdef ([[:alnum:]_]+)/ ) {
	    push @conditions, $conditions[-1] && defined $defines{$1};
	    next;
	}

	if( /^#ifndef ([[:alnum:]_]+)/ ) {
	    push @conditions, $conditions[-1] && not defined $defines{$1};
	    next;
	}

	if( /^#endif/ ) {
	    pop @conditions;
	    next;
	}

	s/^#.*$//;


	print if $conditions[-1] and not $inhibit;
    }
}
	
die "$0: usage: $0 /path/to/config.h /path/to/data/file\n" unless @ARGV == 2;

push @conditions, 1;
parse_file( $ARGV[0], 1 );
parse_file( $ARGV[1] ); 
