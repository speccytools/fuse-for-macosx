#!/usr/bin/perl -w

# harness.pl: simple test harness for Fuse's z80 core
# Copyright (c) 2003 Philip Kendall

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

# E-mail: philip-fuse@shadowmagic.org.uk

use strict;

use IO::File;

sub parse_expected_result ($) {

    my( $filename ) = @_;

    my %results;

    my $fh = new IO::File( '<' . $filename )
	or die "Couldn't open expected result file '$filename': $!";

    while( <$fh> ) {

	if( /^\s+/ ) {

	    my( $tstates, $type, $address, $value ) = split;

	    if( $type =~ /C$/ ) {
		push @{ $results{ordered} },
		  { tstates => $tstates, type => $type, address => $address };
	    } else {
		$results{unordered}{"$type $address $value"}++;
	    }

	} else {

	    $results{final} .= $_;

	}

    }

    close $fh or die "Couldn't close expected result file '$filename': $!";

    return %results;
}

sub do_test ($) {

    my( $testfile ) = @_;

    ( my $outputname = $testfile ) =~ s/\.in$//; $outputname .= '.out';

    my %results = parse_expected_result( $outputname );

    my $test_result = `./coretest $testfile`;

    my $final;

    foreach my $line ( split /\n/, $test_result ) {

	if( $line =~ /^\s+/ ) {

	    my( $tstates, $type, $address, $value ) = split ' ', $line;

	    if( $type =~ /C$/ ) {
		
		my $expected = shift @{ $results{ordered} };

		return 0 unless defined $expected;

		return 0 if $type ne $expected->{type} or
		            $address ne $expected->{address} or
			    $tstates != $expected->{tstates};
		
	    } else {

		return 0 unless
		    defined $results{unordered}{"$type $address $value"};

		return 0 unless $results{unordered}{"$type $address $value"}--;

	    }

	} else {

	    $final .= "$line\n";

	}

    }

    return 0 if @{ $results{ordered} };

    foreach( keys %{ $results{unordered} } ) {
	return 0 if $results{unordered}{$_};
    }

    return 0 unless $final eq $results{final};

    return 1;
}

# Main program begins here

foreach my $testfile ( @ARGV ) {
    next if $testfile =~ /Makefile/;
    print "$testfile\n" unless do_test( $testfile );
}
