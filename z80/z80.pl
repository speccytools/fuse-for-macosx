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
	if( length $arg2 == 1 or $arg2 =~ /^REGISTER[HL]$/ ) {
	    print "      $opcode($arg2);\n";
	} elsif( $arg2 eq '(REGISTER+dd)' ) {
	    print << "CODE";
      tstates += 11;		/* FIXME: how is this contended? */
      {
	BYTE bytetemp=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
	$opcode(bytetemp);
      }
CODE
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
    } elsif( $opcode eq 'ADD' ) {
	print "      ${opcode}16($arg1,$arg2);\n";
    } elsif( $arg1 eq 'HL' and length $arg2 == 2 ) {
	print "      tstates += 7;\n      ${opcode}16($arg2);\n";
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

sub cpi_cpd ($) {

    my( $opcode ) = @_;

    my $modifier = ( $opcode eq 'CPI' ? '++' : '--' );

    print << "CODE";
      {
	BYTE value=readbyte(HL),bytetemp=A-value,
	  lookup = ( (A & 0x08) >> 3 ) | ( ( (value) & 0x08 ) >> 2 ) |
	  ( (bytetemp & 0x08) >> 1 );
	contend( HL, 3 ); contend( HL, 1 ); contend( HL, 1 ); contend( HL, 1 );
	contend( HL, 1 ); contend( HL, 1 );
	HL$modifier; BC--;
	F = ( F & FLAG_C ) | ( BC ? ( FLAG_V | FLAG_N ) : FLAG_N ) |
	  halfcarry_sub_table[lookup] | ( bytetemp ? 0 : FLAG_Z ) |
	  ( bytetemp & FLAG_S );
	if(F & FLAG_H) bytetemp--;
	F |= ( bytetemp & FLAG_3 ) | ( (bytetemp&0x02) ? FLAG_5 : 0 );
      }
CODE
}

sub cpir_cpdr ($) {

    my( $opcode ) = @_;

    my $modifier = ( $opcode eq 'CPIR' ? '++' : '--' );

    print << "CODE";
      {
	BYTE value=readbyte(HL),bytetemp=A-value,
	  lookup = ( (A & 0x08) >> 3 ) | ( ( (value) & 0x08 ) >> 2 ) |
	  ( (bytetemp & 0x08) >> 1 );
	contend( HL, 3 ); contend( HL, 1 ); contend( HL, 1 ); contend( HL, 1 );
	contend( HL, 1 ); contend( HL, 1 );
	HL$modifier; BC--;
	F = ( F & FLAG_C ) | ( BC ? ( FLAG_V | FLAG_N ) : FLAG_N ) |
	  halfcarry_sub_table[lookup] | ( bytetemp ? 0 : FLAG_Z ) |
	  ( bytetemp & FLAG_S );
	if(F & FLAG_H) bytetemp--;
	F |= ( bytetemp & FLAG_3 ) | ( (bytetemp&0x02) ? FLAG_5 : 0 );
	if( ( F & ( FLAG_V | FLAG_Z ) ) == FLAG_V ) {
	  contend( HL, 1 ); contend( HL, 1 ); contend( HL, 1 );
	  contend( HL, 1 ); contend( HL, 1 );
	  PC-=2;
	}
      }
CODE
}

sub inc_dec ($$) {

    my( $opcode, $arg ) = @_;

    my $modifier = ( $opcode eq 'INC' ? '++' : '--' );

    if( length $arg == 1 or $arg =~ /^REGISTER[HL]$/ ) {
	print "      $opcode($arg);\n";
    } elsif( length $arg == 2 or $arg eq 'REGISTER' ) {
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
    } elsif( $arg eq '(REGISTER+dd)' ) {
	print << "CODE";
      tstates += 15;		/* FIXME: how is this contended? */
      {
	WORD wordtemp=REGISTER+(SBYTE)readbyte(PC++);
	BYTE bytetemp=readbyte(wordtemp);
	$opcode(bytetemp);
	writebyte(wordtemp,bytetemp);
      }
CODE
    }

}

sub ini_ind ($) {

    my( $opcode ) = @_;

    my $modifier = ( $opcode eq 'INI' ? '++' : '--' );

    print << "CODE";
      {
	WORD initemp=readport(BC);
	tstates += 2; contend_io( BC, 3 ); contend( HL, 3 );
	writebyte(HL,initemp);
	B--; HL$modifier;
	F = (initemp & 0x80 ? FLAG_N : 0 ) | sz53_table[B];
	/* C,H and P/V flags not implemented */
      }
CODE
}

sub inir_indr ($) {

    my( $opcode ) = @_;

    my $modifier = ( $opcode eq 'INIR' ? '++' : '--' );

    print << "CODE";
      {
	WORD initemp=readport(BC);
	tstates += 2; contend_io( BC, 3 ); contend( HL, 3 );
	writebyte(HL,initemp);
	B--; HL$modifier;
	F = (initemp & 0x80 ? FLAG_N : 0 ) | sz53_table[B];
	/* C,H and P/V flags not implemented */
	if(B) {
	  contend( HL, 1 ); contend( HL, 1 ); contend( HL, 1 ); contend( HL, 1 );
	  contend( HL, 1 );
	  PC-=2;
	}
      }
CODE
}


sub ldi_ldd ($) {

    my( $opcode ) = @_;

    my $modifier = ( $opcode eq 'LDI' ? '++' : '--' );

    print << "CODE";
      {
	BYTE bytetemp=readbyte(HL);
	contend( HL, 3 ); contend( DE, 3 ); contend( DE, 1 ); contend( DE, 1 );
	BC--;
	writebyte(DE,bytetemp);
	DE$modifier; HL$modifier;
	bytetemp += A;
	F = ( F & ( FLAG_C | FLAG_Z | FLAG_S ) ) | ( BC ? FLAG_V : 0 ) |
	  ( bytetemp & FLAG_3 ) | ( (bytetemp & 0x02) ? FLAG_5 : 0 );
      }
CODE
}

sub ldir_lddr ($) {

    my( $opcode ) = @_;

    my $modifier = ( $opcode eq 'LDIR' ? '++' : '--' );

    print << "CODE";
      {
	BYTE bytetemp=readbyte(HL);
	contend( HL, 3 ); contend( DE, 3 ); contend( DE, 1 ); contend( DE, 1 );
	writebyte(DE,bytetemp);
	HL$modifier; DE$modifier; BC--;
	bytetemp += A;
	F = ( F & ( FLAG_C | FLAG_Z | FLAG_S ) ) | ( BC ? FLAG_V : 0 ) |
	  ( bytetemp & FLAG_3 ) | ( (bytetemp & 0x02) ? FLAG_5 : 0 );
	if(BC) {
	  contend( DE, 1 ); contend( DE, 1 ); contend( DE, 1 );
	  contend( DE, 1 ); contend( DE, 1 );
	  PC-=2;
	}
      }
CODE
}

sub otir_otdr ($) {

    my( $opcode ) = @_;

    my $modifier = ( $opcode eq 'OTIR' ? '++' : '--' );

    print << "CODE";
      {
	WORD outitemp=readbyte(HL);
	tstates++; contend( HL, 4 );
	B--; HL$modifier; /* This does happen first, despite what the specs say */
	writeport(BC,outitemp);
	F = (outitemp & 0x80 ? FLAG_N : 0 ) | sz53_table[B];
	/* C,H and P/V flags not implemented */
	if(B) {
	  contend_io( BC, 1 );
	  contend( PC, 1 ); contend( PC, 1 ); contend( PC, 1 );
	  contend( PC, 1 ); contend( PC, 1 ); contend( PC, 1 );
	  contend( PC - 1, 1 );
	  PC-=2;
	} else {
	  contend_io( BC, 3 );
	}
      }
CODE
}

sub outi_outd ($) {

    my( $opcode ) = @_;

    my $modifier = ( $opcode eq 'OUTI' ? '++' : '--' );

    print << "CODE";
      {
	WORD outitemp=readbyte(HL);
	B--;	/* This does happen first, despite what the specs say */
	tstates++; contend( HL, 4 ); contend_io( BC, 3 );
	HL$modifier;
	writeport(BC,outitemp);
	F = (outitemp & 0x80 ? FLAG_N : 0 ) | sz53_table[B];
	/* C,H and P/V flags not implemented */
      }
CODE
}

sub push_pop ($$) {

    my( $opcode, $regpair ) = @_;

    my( $high, $low );

    if( $regpair eq 'REGISTER' ) {
	( $high, $low ) = ( 'REGISTERH', 'REGISTERL' );
    } else {
	( $high, $low ) = ( $regpair =~ /^(.)(.)$/ );
    }

    print "      ${opcode}16($low,$high);\n";
}

sub res_set_hexmask ($$) {

    my( $opcode, $bit ) = @_;

    my $mask = 1 << $bit;
    $mask = 0xff - $mask if $opcode eq 'RES';

    sprintf '0x%02x', $mask;
}

sub res_set ($$$) {

    my( $opcode, $bit, $register ) = @_;

    my $operator = ( $opcode eq 'RES' ? '&' : '|' );

    my $hex_mask = res_set_hexmask( $opcode, $bit );

    if( length $register == 1 ) {
	print "      $register $operator= $hex_mask;\n";
    } elsif( $register eq '(HL)' ) {
	print << "CODE";
      contend( HL, 4 ); contend( HL, 3 );
      writebyte(HL, readbyte(HL) $operator $hex_mask);
CODE
    } elsif( $register eq '(REGISTER+dd)' ) {
	print << "CODE";
      tstates += 8;
      writebyte(tempaddr, readbyte(tempaddr) $operator $hex_mask);
CODE
    }
}

sub rotate_shift ($$) {

    my( $opcode, $register ) = @_;

    if( length $register == 1 ) {
	print "      $opcode($register);\n";
    } elsif( $register eq '(HL)' ) {
	print << "CODE";
      {
	BYTE bytetemp = readbyte(HL);
	contend( HL, 4 ); contend( HL, 3 );
	$opcode(bytetemp);
	writebyte(HL,bytetemp);
      }
CODE
    } elsif( $register eq '(REGISTER+dd)' ) {
	print << "CODE";
      tstates += 8;
      {
	BYTE bytetemp = readbyte(tempaddr);
	$opcode(bytetemp);
	writebyte(tempaddr,bytetemp);
      }
CODE
    }
}

# Individual opcode routines

sub opcode_ADC (@) { arithmetic_logical( 'ADC', $_[0], $_[1] ); }

sub opcode_ADD (@) { arithmetic_logical( 'ADD', $_[0], $_[1] ); }

sub opcode_AND (@) { arithmetic_logical( 'AND', $_[0], $_[1] ); }

sub opcode_BIT (@) {

    my( $bit, $register ) = @_;

    my $macro = ( $bit == 7 ? '7(' : "($bit," );

    if( length $register == 1 ) {
	print "      BIT$macro$register);\n";
    } elsif( $register eq '(REGISTER+dd)' ) {
	print << "BIT";
      tstates += 5;
      {
	BYTE bytetemp=readbyte(tempaddr);
	BIT${macro}bytetemp);
      }
BIT
    } else {
	print << "BIT";
      {
	BYTE bytetemp = readbyte(HL);
	contend( HL, 4 );
	BIT${macro}bytetemp);
      }
BIT
    }
}

sub opcode_CALL (@) { call_jp( 'CALL', $_[0], $_[1] ); }

sub opcode_CCF (@) {
    print << "CCF";
      F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	( ( F & FLAG_C ) ? FLAG_H : FLAG_C ) | ( A & ( FLAG_3 | FLAG_5 ) );
CCF
}

sub opcode_CP (@) { arithmetic_logical( 'CP', $_[0], $_[1] ); }

sub opcode_CPD (@) { cpi_cpd( 'CPD' ); }

sub opcode_CPDR (@) { cpir_cpdr( 'CPDR' ); }

sub opcode_CPI (@) { cpi_cpd( 'CPI' ); }

sub opcode_CPIR (@) { cpir_cpdr( 'CPIR' ); }

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
      if( PC == 0x04d1 || PC == 0x0077 ) {
	if( tape_save_trap() == 0 ) break;
      }

      {
	WORD wordtemp=AF; AF=AF_; AF_=wordtemp;
      }
EX
    } elsif( $arg1 eq '(SP)' and ( $arg2 eq 'HL' or $arg2 eq 'REGISTER' ) ) {

	my( $high, $low );

	if( $arg2 eq 'HL' ) {
	    ( $high, $low ) = qw( H L );
	} else {
	    ( $high, $low ) = qw( REGISTERH REGISTERL );
	}

	print << "EX";
      {
	BYTE bytetempl=readbyte(SP), bytetemph=readbyte(SP+1);
	contend( SP, 3 ); contend( SP+1, 4 );
	contend( SP, 3 ); contend( SP+1, 5 );
	writebyte(SP,$low); writebyte(SP+1,$high);
	$low=bytetempl; $high=bytetemph;
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

sub opcode_IM (@) {

    my( $mode ) = @_;

    print "      IM=$mode;\n";
}

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
    } elsif( $register eq 'F' and $port eq '(C)' ) {
	print << "IN";
      tstates += 1;
      {
	BYTE bytetemp;
	IN(bytetemp,BC);
      }
IN
    } elsif( length $register == 1 and $port eq '(C)' ) {
	print << "IN";
      tstates += 1;
      IN($register,BC);
IN
    }
}

sub opcode_INC (@) { inc_dec( 'INC', $_[0] ); }

sub opcode_IND (@) { ini_ind( 'IND' ); }

sub opcode_INDR (@) { inir_indr( 'INDR' ); }

sub opcode_INI (@) { ini_ind( 'INI' ); }

sub opcode_INIR (@) { inir_indr( 'INIR' ); }

sub opcode_JP (@) {

    my( $condition, $offset ) = @_;

    if( $condition eq 'HL' or $condition eq 'REGISTER' ) {
	print "      PC=$condition;\t\t/* NB: NOT INDIRECT! */\n";
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

    if( length $dest == 1 or $dest =~ /^REGISTER[HL]$/ ) {

	if( length $src == 1 or $src =~ /^REGISTER[HL]$/ ) {

	    if( $dest eq 'R' and $src eq 'A' ) {
		print << "LD";
      tstates += 1;
      /* Keep the RZX instruction counter right */
      rzx_instructions_offset += ( R - A );
      R=R7=A;
LD
            } elsif( $dest eq 'A' and $src eq 'R' ) {
		print << "LD";
      tstates += 1;
      A=(R&0x7f) | (R7&0x80);
      F = ( F & FLAG_C ) | sz53_table[A] | ( IFF2 ? FLAG_V : 0 );
LD
	    } else {
		print "      tstates += 1;\n" if $src eq 'I' or $dest eq 'I';
		print "      $dest=$src;\n" if $dest ne $src;
		if( $dest eq 'A' and $src eq 'I' ) {
		    print "      F = ( F & FLAG_C ) | sz53_table[A] | ( IFF2 ? FLAG_V : 0 );\n";
		}
	    }
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
        } elsif( $src eq '(REGISTER+dd)' ) {
	    print << "LD";
      tstates += 11;		/* FIXME: how is this contended? */
      $dest=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
LD
        }

    } elsif( length $dest == 2 or $dest eq 'REGISTER' ) {

	my( $high, $low );

	if( $dest eq 'SP' or $dest eq 'REGISTER' ) {
	    ( $high, $low ) = ( "${dest}H", "${dest}L" );
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
        } elsif( $src eq 'HL' or $src eq 'REGISTER' ) {
	    print "      tstates += 2;\n      SP=$src;\n";
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
        } elsif( $src =~ /^(.)(.)$/ or $src eq 'REGISTER' ) {

	    my( $high, $low );

	    if( $src eq 'SP' or $src eq 'REGISTER' ) {
		( $high, $low ) = ( "${src}H", "${src}L" );
	    } else {
		( $high, $low ) = ( $1, $2 );
	    }

	    print "      LD16_NNRR($low,$high);\n";
	}
    } elsif( $dest eq '(REGISTER+dd)' ) {

	if( length $src == 1 ) {
	print << "LD";
      tstates += 11;		/* FIXME: how is this contended? */
      writebyte( REGISTER + (SBYTE)readbyte(PC++), $src);
LD
        } elsif( $src eq 'nn' ) {
	    print << "LD";
      tstates += 11;		/* FIXME: how is this contended? */
      {
	WORD wordtemp=REGISTER+(SBYTE)readbyte(PC++);
	writebyte(wordtemp,readbyte(PC++));
      }
LD
        }
    }

}

sub opcode_LDD (@) { ldi_ldd( 'LDD' ); }

sub opcode_LDDR (@) { ldir_lddr( 'LDDR' ); }

sub opcode_LDI (@) { ldi_ldd( 'LDI' ); }

sub opcode_LDIR (@) { ldir_lddr( 'LDIR' ); }

sub opcode_NEG (@) {
    print << "NEG";
      {
	BYTE bytetemp=A;
	A=0;
	SUB(bytetemp);
      }
NEG
}

sub opcode_NOP (@) { }

sub opcode_OR (@) { arithmetic_logical( 'OR', $_[0], $_[1] ); }

sub opcode_OTDR (@) { otir_otdr( 'OTDR' ); }

sub opcode_OTIR (@) { otir_otdr( 'OTIR' ); }

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
    } elsif( $port eq '(C)' and length $register == 1 ) {
	print << "OUT";
      tstates += 1;
      OUT(BC,$register);
OUT
    }
}


sub opcode_OUTD (@) { outi_outd( 'OUTD' ); }

sub opcode_OUTI (@) { outi_outd( 'OUTI' ); }

sub opcode_POP (@) { push_pop( 'POP', $_[0] ); }

sub opcode_PUSH (@) {

    my( $regpair ) = @_;

    print "      tstates++;\n";
    push_pop( 'PUSH', $regpair );
}

sub opcode_RES (@) { res_set( 'RES', $_[0], $_[1] ); }

sub opcode_RET (@) {

    my( $condition ) = @_;

    if( not defined $condition ) {
	print "      RET();\n";
    } else {
	print "      tstates++;\n";
	
	if( $condition eq 'NZ' ) {
	    print << "RET";
      if( PC==0x056c || PC == 0x0112 ) {
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

sub opcode_RETN (@) { 

    print << "RETN";
      IFF1=IFF2;
      RET();
RETN
}

sub opcode_RL (@) { rotate_shift( 'RL', $_[0] ); }

sub opcode_RLC (@) { rotate_shift( 'RLC', $_[0] ); }

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

sub opcode_RLD (@) {
    print << "RLD";
      {
	BYTE bytetemp=readbyte(HL);
	contend( HL, 7 ); contend( HL, 3 );
	writebyte(HL, (bytetemp << 4 ) | ( A & 0x0f ) );
	A = ( A & 0xf0 ) | ( bytetemp >> 4 );
	F = ( F & FLAG_C ) | sz53p_table[A];
      }
RLD
}

sub opcode_RR (@) { rotate_shift( 'RR', $_[0] ); }

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

sub opcode_RRC (@) { rotate_shift( 'RRC', $_[0] ); }

sub opcode_RRCA (@) {
    print << "RRCA";
      F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) | ( A & FLAG_C );
      A = ( A >> 1) | ( A << 7 );
      F |= ( A & ( FLAG_3 | FLAG_5 ) );
RRCA
}

sub opcode_RRD (@) {
    print << "RRD";
      {
	BYTE bytetemp=readbyte(HL);
	contend( HL, 7 ); contend( HL, 3 );
	writebyte(HL,  ( A << 4 ) | ( bytetemp >> 4 ) );
	A = ( A & 0xf0 ) | ( bytetemp & 0x0f );
	F = ( F & FLAG_C ) | sz53p_table[A];
      }
RRD
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

sub opcode_SET (@) { res_set( 'SET', $_[0], $_[1] ); }

sub opcode_SLA (@) { rotate_shift( 'SLA', $_[0] ); }

sub opcode_SLL (@) { rotate_shift( 'SLL', $_[0] ); }

sub opcode_SRA (@) { rotate_shift( 'SRA', $_[0] ); }

sub opcode_SRL (@) { rotate_shift( 'SRL', $_[0] ); }

sub opcode_SUB (@) { arithmetic_logical( 'SUB', $_[0], $_[1] ); }

sub opcode_XOR (@) { arithmetic_logical( 'XOR', $_[0], $_[1] ); }

sub opcode_slttrap ($) {
    print << "slttrap";
      if( settings_current.slt_traps ) {
	if( slt_length[A] ) {
	  WORD base = HL;
	  BYTE *data = slt[A];
	  size_t length = slt_length[A];
	  while( length-- ) writebyte( base++, *data++ );
	}
      }
slttrap
}

sub opcode_shift (@) {

    my( $opcode ) = @_;

    my $lc_opcode = lc $opcode;

    if( $opcode eq 'DDFDCB' ) {

	print << "shift";
      /* FIXME: contention here is just a guess */
      {
	WORD tempaddr; BYTE opcode3;
	contend( PC, 3 );
	tempaddr = REGISTER + (SBYTE)readbyte_internal( PC++ );
	contend( PC, 4 );
	opcode3 = readbyte_internal( PC++ );
#ifdef HAVE_ENOUGH_MEMORY
	switch(opcode3) {
#include "z80_ddfdcb.c"
	}
#else			/* #ifdef HAVE_ENOUGH_MEMORY */
	z80_ddfdcbxx(opcode3,tempaddr);
#endif			/* #ifdef HAVE_ENOUGH_MEMORY */
      }
shift
    } else {
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
        } elsif( $opcode eq 'CB' or $opcode eq 'ED' ) {
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
}

# Description of each file

my %description = (

    'opcodes_cb.dat'     => 'opcodes_cb.c: Z80 CBxx opcodes',
    'opcodes_ddfd.dat'   => 'opcodes_ddfd.c Z80 {DD,FD}xx opcodes',
    'opcodes_ddfdcb.dat' => 'opcodes_ddfdcb.c Z80 {DD,FD}CBxx opcodes',
    'opcodes_ed.dat'     => 'opcodes_ed.c: Z80 CBxx opcodes',
    'opcodes_base.dat'   => 'opcodes_base.c: unshifted Z80 opcodes',

);

# Main program

my $data_file = $ARGV[0];

print Fuse::GPL( $description{ $data_file }, '1999-2003 Philip Kendall' );

print << "COMMENT";

/* NB: this file is autogenerated by '$0' from '$data_file',
   and included in 'z80_ops.c' */

COMMENT

while(<>) {

    # Remove comments
    s/#.*//;

    # Skip (now) blank lines
    next if /^\s*$/;

    chomp;

    my( $number, $opcode, $arguments, $extra ) = split;

    if( not defined $opcode ) {
	print "    case $number:\n";
	next;
    }

    $arguments = '' if not defined $arguments;
    my @arguments = split ',', $arguments;

    print "    case $number:\t\t/* $opcode";

    print ' ', join ',', @arguments if @arguments;
    print " $extra" if defined $extra;

    print " */\n";

    # Handle the undocumented rotate-shift-or-bit and store-in-register
    # opcodes specially

    if( defined $extra ) {

	my( $register, $opcode ) = @arguments;

	if( $opcode eq 'RES' or $opcode eq 'SET' ) {

	    my( $bit ) = split ',', $extra;

	    my $operator = ( $opcode eq 'RES' ? '&' : '|' );
	    my $hexmask = res_set_hexmask( $opcode, $bit );

	    print << "CODE";
      tstates += 8;
      $register=readbyte(tempaddr) $operator $hexmask;
      writebyte(tempaddr, $register);
      break;
CODE
	} else {

	    print << "CODE";
      tstates += 8;
      $register=readbyte(tempaddr);
      $opcode($register);
      writebyte(tempaddr, $register);
      break;
CODE
	}
	next;
    }

    {
	no strict qw( refs );

	if( exists &{ "opcode_$opcode" } ) {
	    "opcode_$opcode"->( @arguments );
	}
    }

    print "      break;\n";
}

if( $data_file eq 'opcodes_ddfd.dat' ) {

    print << "CODE";
    default:		/* Instruction did not involve H or L, so backtrack
			   one instruction and parse again */
      PC--;		/* FIXME: will be contended again */
      R--;		/* Decrement the R register as well */
      break;
CODE

} elsif( $data_file eq 'opcodes_ed.dat' ) {
    print << "NOPD";
    default:		/* All other opcodes are NOPD */
      break;
NOPD
}
