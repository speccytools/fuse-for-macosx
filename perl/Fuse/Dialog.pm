# Fuse::Dialog: routines for creating Fuse dialog boxes
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
