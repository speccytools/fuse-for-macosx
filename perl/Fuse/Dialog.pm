# Fuse::Dialog: routines for creating Fuse dialog boxes
# $Id$

package Fuse::Dialog;

use strict;

use English;
use IO::File;

sub read ($) {

    my $filename = shift;

    my $fh = new IO::File( "< $filename" )
	or die "Couldn't open `$filename': $!";

    local $INPUT_RECORD_SEPARATOR = ""; # Paragraph mode

    my @dialogs;
    while( <$fh> ) {

	my( $name, $title, @widgets ) = split /\n/;

	foreach( @widgets ) {
	    my( $widget_type, $text, $value, $key ) = split /\s*,\s*/;
	    $_ = { type => $widget_type,
		   text => $text,
		   value => $value,
		   key => $key,
	         };
	}

	push @dialogs, { name => $name,
			 title => $title,
			 widgets => \@widgets };
    }

    return @dialogs;
}

1;
