#!/usr/bin/perl -w

# z80.pl: generate C code for Z80 opcodes
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

use lib '../perl';

use Fuse;

# The status of which flags relates to which condition

# These conditions involve !( F & FLAG_<whatever> )
my %not = map { $_ => 1 } qw( NC NZ P PO );

# Use F & FLAG_<whatever>
my %flag = (

      C => 'C', NC => 'C',
     PE => 'P', PO => 'P',
      M => 'S',  P => 'S',
      Z => 'Z', NZ => 'Z',

);

# Generalised opcode routines

sub arithmetic_logical ($$$) {

    my( $opcode, $arg1, $arg2 ) = @_;

    unless( $arg2 ) { $arg2 = $arg1; $arg1 = 'A'; }

    if( length $arg1 == 1 ) {
	if( length $arg2 == 1 ) {
	    print "      $opcode($arg2);\n";
	} else {
	    my $register = ( $arg2 eq '(HL)' ? 'HL' : 'PC' );
	    my $increment = ( $register eq 'PC' ? '++' : '' );
	    print << "CODE";
      contend( $register, 3 );
      {
	BYTE bytetemp=readbyte($register$increment);
	$opcode(bytetemp);
      }
CODE
	}
    } else {
	print "      ${opcode}16($arg1,$arg2);\n";
    }
}

sub call_jp ($$$) {

    my( $opcode, $condition, $offset ) = @_;

    print "      contend( PC, 3 ); contend( PC+1, 3 );\n";

    if( not defined $offset ) {
	print "      $opcode();\n";
    } else {
	if( defined $not{$condition} ) {
	    print "      if( ! ( F & FLAG_$flag{$condition} ) ) { $opcode(); }\n";
	} else {
	    print "      if( F & FLAG_$flag{$condition} ) { $opcode(); }\n";
	}
	print "      else PC+=2;\n";
    }
}

sub inc_dec ($$) {

    my( $opcode, $arg ) = @_;

    my $modifier = ( $opcode eq 'DEC' ? '--' : '++' );

    if( length $arg == 1 ) {
	print "      $opcode($arg);\n";
    } elsif( length $arg == 2 ) {
	print "      tstates += 2;\n      ${arg}$modifier;\n";
    } elsif( $arg eq '(HL)' ) {
	print << "CODE";
      contend( HL, 4 );
      {
	BYTE bytetemp=readbyte(HL);
	$opcode(bytetemp);
	contend( HL, 3 );
	writebyte(HL,bytetemp);
      }
CODE
    }
}

sub push_pop ($$) {

    my( $opcode, $regpair ) = @_;

    my( $high, $low ) = ( $regpair =~ /^(.)(.)$/ );

    print "      ${opcode}16($low,$high);\n";
}

# Individual opcode routines

sub opcode_ADC (@) { arithmetic_logical( 'ADC', $_[0], $_[1] ); }

sub opcode_ADD (@) { arithmetic_logical( 'ADD', $_[0], $_[1] ); }

sub opcode_AND (@) { arithmetic_logical( 'AND', $_[0], $_[1] ); }

sub opcode_CALL (@) { call_jp( 'CALL', $_[0], $_[1] ); }

sub opcode_CCF (@) {
    print << "CCF";
      F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	( ( F & FLAG_C ) ? FLAG_H : FLAG_C ) | ( A & ( FLAG_3 | FLAG_5 ) );
CCF
}

sub opcode_CP (@) { arithmetic_logical( 'CP', $_[0], $_[1] ); }

sub opcode_CPL (@) {
    print << "CPL";
      A ^= 0xff;
      F = ( F & ( FLAG_C | FLAG_P | FLAG_Z | FLAG_S ) ) |
	( A & ( FLAG_3 | FLAG_5 ) ) | ( FLAG_N | FLAG_H );
CPL
}

sub opcode_DAA (@) {
    print << "DAA";
      {
	BYTE add = 0,carry= ( F & FLAG_C );
	if( ( F & FLAG_H ) || ( (A & 0x0f)>9 ) ) add=6;
	if( carry || (A > 0x9f ) ) add|=0x60;
	if( A > 0x99 ) carry=1;
	if ( F & FLAG_N ) {
	  SUB(add);
	} else {
	  if( (A>0x90) && ( (A & 0x0f)>9) ) add|=0x60;
	  ADD(add);
	}
	F = ( F & ~( FLAG_C | FLAG_P) ) | carry | parity_table[A];
      }
DAA
}

sub opcode_DEC (@) { inc_dec( 'DEC', $_[0] ); }

sub opcode_DI (@) { print "      IFF1=IFF2=0;\n"; }

sub opcode_DJNZ (@) {
    print << "DJNZ";
      tstates++; contend( PC, 3 );
      B--;
      if(B) { JR(); }
      PC++;
DJNZ
}

sub opcode_EI (@) { print "      IFF1=IFF2=1;\n"; }

sub opcode_EX (@) {

    my( $arg1, $arg2 ) = @_;

    if( $arg1 eq 'AF' and $arg2 eq "AF'" ) {
	print << "EX";
      /* Tape saving trap: note this traps the EX AF,AF\' at #04d0, not
	 #04d1 as PC has already been incremented */
     /* 0x76 - Timex 2068 save routine in EXROM */
      if( PC == 0x04d1 || PC == 0x0077) {
	if( tape_save_trap() == 0 ) break;
      }

      {
	WORD wordtemp=AF; AF=AF_; AF_=wordtemp;
      }
EX
    } elsif( $arg1 eq '(SP)' and $arg2 eq 'HL' ) {
	print << "EX";
      {
	BYTE bytetempl=readbyte(SP), bytetemph=readbyte(SP+1);
	contend( SP, 3 ); contend( SP+1, 4 );
	contend( SP, 3 ); contend( SP+1, 5 );
	writebyte(SP,L); writebyte(SP+1,H);
	L=bytetempl; H=bytetemph;
      }
EX
    } elsif( $arg1 eq 'DE' and $arg2 eq 'HL' ) {
	print << "EX";
      {
	WORD wordtemp=DE; DE=HL; HL=wordtemp;
      }
EX
    }
}

sub opcode_EXX (@) {
    print << "EXX";
      {
	WORD wordtemp=BC; BC=BC_; BC_=wordtemp;
	wordtemp=DE; DE=DE_; DE_=wordtemp;
	wordtemp=HL; HL=HL_; HL_=wordtemp;
      }
EXX
}

sub opcode_HALT (@) { print "      z80.halted=1;\n      PC--;\n"; }

sub opcode_IN (@) {

    my( $register, $port ) = @_;

    if( $register eq 'A' and $port eq '(nn)' ) {
	print << "IN";
      { 
	WORD intemp;
	contend( PC, 4 );
	intemp = readbyte( PC++ ) + ( A << 8 );
	contend_io( intemp, 3 );
        A=readport( intemp );
      }
IN
    }
}

sub opcode_INC (@) { inc_dec( 'INC', $_[0] ); }

sub opcode_JP (@) {

    my( $condition, $offset ) = @_;

    if( $condition eq 'HL' ) {
	print "      PC=HL;\t\t/* NB: NOT INDIRECT! */\n";
	return;
    } else {
	call_jp( 'JP', $condition, $offset );
    }
}

sub opcode_JR (@) {

    my( $condition, $offset ) = @_;

    if( not defined $offset ) { $offset = $condition; $condition = ''; }

    print "      contend( PC, 3 );\n";

    if( !$condition ) {
	print "      JR();\n";
    } elsif( defined $not{$condition} ) {
	print "      if( ! ( F & FLAG_$flag{$condition} ) ) { JR(); }\n";
    } else {
	print "      if( F & FLAG_$flag{$condition} ) { JR(); }\n";
    }

    print "      PC++;\n";
}

sub opcode_LD (@) {

    my( $dest, $src ) = @_;

    if( length $dest == 1 ) {

	if( length $src == 1 ) {
	    print "      $dest=$src;\n" if $dest ne $src;
	} elsif( $src eq 'nn' ) {
	    print "      contend( PC, 3 );\n      $dest=readbyte(PC++);\n";
	} elsif( $src =~ /^\(..\)$/ ) {
	    my $register = substr $src, 1, 2;
	    print << "LD";
      contend( $register, 3 );
      $dest=readbyte($register);
LD
        } elsif( $src eq '(nnnn)' ) {
	    print << "LD";
      {
	WORD wordtemp;
	contend( PC, 3 );
	wordtemp = readbyte(PC++);
	contend( PC, 3 );
	wordtemp|= ( readbyte(PC++) << 8 );
	contend( wordtemp, 3 );
	A=readbyte(wordtemp);
      }
LD
        }

    } elsif( length $dest == 2 ) {

	my( $high, $low );

	if( $dest eq 'SP' ) {
	    ( $high, $low ) = ( 'SPH', 'SPL' );
	} else {
	    ( $high, $low ) = ( $dest =~ /^(.)(.)$/ );
	}

	if( $src eq 'nnnn' ) {

	    print << "LD";
      contend( PC, 3 );
      $low=readbyte(PC++);
      contend( PC, 3 );
      $high=readbyte(PC++);
LD
        } elsif( $src eq 'HL' ) {
	    print "      tstates += 2;\n      SP=HL;\n";
        } elsif( $src eq '(nnnn)' ) {
	    print "      LD16_RRNN($low,$high);\n";
	}

    } elsif( $dest =~ /^\(..\)$/ ) {

	my $register = substr $dest, 1, 2;

	if( length $src == 1 ) {
	    print << "LD";
      contend( $register, 3 );
      writebyte($register,$src);
LD
	} elsif( $src eq 'nn' ) {
	    print << "LD";
      contend( PC, 3 ); contend( $register, 3 );
      writebyte($register,readbyte(PC++));
LD
        }

    } elsif( $dest eq '(nnnn)' ) {

	if( $src eq 'A' ) {
	    print << "LD";
      contend( PC, 3 );
      {
	WORD wordtemp=readbyte(PC++);
	contend( PC, 3 );
	wordtemp|=readbyte(PC++) << 8;
	contend( wordtemp, 3 );
	writebyte(wordtemp,A);
      }
LD
        } elsif( $src eq 'HL' ) {
	    print "      LD16_NNRR(L,H);\n";
	}
    }

}

sub opcode_NOP (@) { }

sub opcode_OR (@) { arithmetic_logical( 'OR', $_[0], $_[1] ); }

sub opcode_OUT (@) {

    my( $port, $register ) = @_;

    if( $port eq '(nn)' and $register eq 'A' ) {
	print << "OUT";
      { 
	WORD outtemp;
	contend( PC, 4 );
	outtemp = readbyte( PC++ ) + ( A << 8 );
	OUT( outtemp , A );
      }
OUT
    }
}

sub opcode_POP (@) { push_pop( 'POP', $_[0] ); }

sub opcode_PUSH (@) {

    my( $regpair ) = @_;

    print "      tstates++;\n";
    push_pop( 'PUSH', $regpair );
}

sub opcode_RET (@) {

    my( $condition ) = @_;

    if( not defined $condition ) {
	print "      RET();\n";
    } else {
	print "      tstates++;\n";
	
	if( $condition eq 'NZ' ) {
	    print << "RET";
      if( PC==0x056c || PC == 0x0112) {
	if( tape_load_trap() == 0 ) break;
      }
RET
        }

	if( defined $not{$condition} ) {
	    print "      if( ! ( F & FLAG_$flag{$condition} ) ) { RET(); }\n";
	} else {
	    print "      if( F & FLAG_$flag{$condition} ) { RET(); }\n";
	}
    }
}

sub opcode_RLCA (@) {
    print << "RLCA";
      A = ( A << 1 ) | ( A >> 7 );
      F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	( A & ( FLAG_C | FLAG_3 | FLAG_5 ) );
RLCA
}

sub opcode_RLA (@) {
    print << "RLA";
      {
	BYTE bytetemp = A;
	A = ( A << 1 ) | ( F & FLAG_C );
	F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	  ( A & ( FLAG_3 | FLAG_5 ) ) | ( bytetemp >> 7 );
      }
RLA
}

sub opcode_RRA (@) {
    print << "RRA";
      {
	BYTE bytetemp = A;
	A = ( A >> 1 ) | ( F << 7 );
	F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	  ( A & ( FLAG_3 | FLAG_5 ) ) | ( bytetemp & FLAG_C ) ;
      }
RRA
}

sub opcode_RRCA (@) {
    print << "RRCA";
      F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) | ( A & FLAG_C );
      A = ( A >> 1) | ( A << 7 );
      F |= ( A & ( FLAG_3 | FLAG_5 ) );
RRCA
}

sub opcode_RST (@) {

    my( $value ) = @_;

    printf "      tstates++;\n      RST(0x%02x);\n", hex $value;
}

sub opcode_SBC (@) { arithmetic_logical( 'SBC', $_[0], $_[1] ); }

sub opcode_SCF (@) {
    print << "SCF";
      F &= ~( FLAG_N | FLAG_H );
      F |= ( A & ( FLAG_3 | FLAG_5 ) ) | FLAG_C;
SCF
}

sub opcode_SUB (@) { arithmetic_logical( 'SUB', $_[0], $_[1] ); }

sub opcode_XOR (@) { arithmetic_logical( 'XOR', $_[0], $_[1] ); }

sub opcode_shift (@) {

    my( $opcode ) = @_;

    my $lc_opcode = lc $opcode;

    print << "shift";
      {
	BYTE opcode2;
	contend( PC, 4 );
	opcode2 = readbyte_internal( PC++ );
	R++;
#ifdef HAVE_ENOUGH_MEMORY
	switch(opcode2) {
shift

    if( $opcode eq 'DD' or $opcode eq 'FD' ) {
	my $register = ( $opcode eq 'DD' ? 'IX' : 'IY' );
	print << "shift";
#define REGISTER  $register
#define REGISTERL ${register}L
#define REGISTERH ${register}H
#include "z80_ddfd.c"
#undef REGISTERH
#undef REGISTERL
#undef REGISTER
shift
    } else {
	print "#include \"z80_$lc_opcode.c\"\n";
    }

    print << "shift"
	}
#else			/* #ifdef HAVE_ENOUGH_MEMORY */
	z80_${lc_opcode}xx(opcode2);
#endif			/* #ifdef HAVE_ENOUGH_MEMORY */
      }
shift
}

# Main program

print Fuse::GPL( 'opcodes_base.c: unshifted Z80 opcodes',
                 '1999-2003 Philip Kendall' );

print << "COMMENT";

/* NB: this file is autogenerated by '$0' from '$ARGV[0]',
   and included in 'z80_ops.c' */

COMMENT

while(<>) {

    # Remove comments
    s/#.*//;

    # Skip (now) blank lines
    next if /^\s*$/;

    chomp;

    my( $number, $opcode, $arguments ) = split;

    $arguments ||= '';
    my @arguments = split ',', $arguments;

    print "    case $number:\t\t/* $opcode";

    print ' ', join ',', @arguments if @arguments;

    print " */\n";

    {
	no strict qw( refs );

	if( exists &{ "opcode_$opcode" } ) {
	    "opcode_$opcode"->( @arguments );
	} else {
	    print STDERR "$opcode unknown\n";
	}
    }

    print "      break;\n";
}
