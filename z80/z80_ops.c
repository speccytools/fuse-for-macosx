/* z80_ops.c: Process the next opcode
   Copyright (c) 1999-2000 Philip Kendall

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <stdio.h>

#include "../event.h"
#include "../spectrum.h"
#include "../tape.h"
#include "z80.h"

#include "z80_macros.h"

#ifndef HAVE_ENOUGH_MEMORY
static void z80_cbxx(BYTE opcode2);
static void z80_ddxx(BYTE opcode2);
static void z80_edxx(BYTE opcode2);
static void z80_fdxx(BYTE opcode2);
static void z80_ddfdcbxx(BYTE opcode3, WORD tempaddr);
#endif				/* #ifndef HAVE_ENOUGH_MEMORY */

/* Execute Z80 opcodes until the next event */
void z80_do_opcodes()
{

  while(tstates < event_next_event ) {

    BYTE opcode;

    /* If the z80 is HALTed, execute a NOP-equivalent and loop again */
    if(z80.halted) {
      tstates+=4;
      continue;
    }

    opcode=readbyte(PC++); R++;

    switch(opcode) {
    case 0x00:		/* NOP */
      tstates+=4;
      break;
    case 0x01:		/* LD BC,nnnn */
      tstates+=10;
      C=readbyte(PC++);
      B=readbyte(PC++);
      break;
    case 0x02:		/* LD (BC),A */
      tstates+=7;
      writebyte(BC,A);
      break;
    case 0x03:		/* INC BC */
      tstates+=6;
      BC++;
      break;
    case 0x04:		/* INC B */
      tstates+=4;
      INC(B);
      break;
    case 0x05:		/* DEC B */
      tstates+=4;
      DEC(B);
      break;
    case 0x06:		/* LB B,nn */
      tstates+=7;
      B=readbyte(PC++);
      break;
    case 0x07:		/* RLCA */
      tstates+=4;
      A = ( A << 1 ) | ( A >> 7 );
      F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	( A & ( FLAG_C | FLAG_3 | FLAG_5 ) );
      break;
    case 0x08:		/* EX AF,AF' */
      tstates+=4;
      {
	WORD wordtemp=AF; AF=AF_; AF_=wordtemp;
      }
      break;
    case 0x09:		/* ADD HL,BC */
      tstates+=11;
      ADD16(HL,BC);
      break;
    case 0x0a:		/* LD A,(BC) */
      tstates+=7;
      A=readbyte(BC);
      break;
    case 0x0b:		/* DEC BC */
      tstates+=6;
      BC--;
      break;
    case 0x0c:		/* INC C */
      tstates+=4;
      INC(C);
      break;
    case 0x0d:		/* DEC C */
      tstates+=4;
      DEC(C);
      break;
    case 0x0e:		/* LD C,nn */
      tstates+=7;
      C=readbyte(PC++);
      break;
    case 0x0f:		/* RRCA */
      tstates+=4;
      F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) | ( A & FLAG_C );
      A = ( A >> 1) | ( A << 7 );
      F |= ( A & ( FLAG_3 | FLAG_5 ) );
      break;
    case 0x10:		/* DJNZ offset */
      tstates+=8;
      B--;
      if(B) { tstates+=5; JR(); }
      PC++;
      break;
    case 0x11:		/* LD DE,nnnn */
      tstates+=10;
      E=readbyte(PC++);
      D=readbyte(PC++);
      break;
    case 0x12:		/* LD (DE),A */
      tstates+=7;
      writebyte(DE,A);
      break;
    case 0x13:		/* INC DE */
      tstates+=6;
      DE++;
      break;
    case 0x14:		/* INC D */
      tstates+=4;
      INC(D);
      break;
    case 0x15:		/* DEC D */
      tstates+=4;
      DEC(D);
      break;
    case 0x16:		/* LD D,nn */
      tstates+=7;
      D=readbyte(PC++);
      break;
    case 0x17:		/* RLA */
      tstates+=4;
      {
	BYTE bytetemp = A;
	A = ( A << 1 ) | ( F & FLAG_C );
	F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	  ( A & ( FLAG_3 | FLAG_5 ) ) | ( bytetemp >> 7 );
      }
      break;
    case 0x18:		/* JR offset */
      tstates+=12;
      JR();
      PC++;
      break;
    case 0x19:		/* ADD HL,DE */
      tstates+=11;
      ADD16(HL,DE);
      break;
    case 0x1a:		/* LD A,(DE) */
      tstates+=7;
      A=readbyte(DE);
      break;
    case 0x1b:		/* DEC DE */
      tstates+=6;
      DE--;
      break;
    case 0x1c:		/* INC E */
      tstates+=4;
      INC(E);
      break;
    case 0x1d:		/* DEC E */
      tstates+=4;
      DEC(E);
      break;
    case 0x1e:		/* LD E,nn */
      tstates+=7;
      E=readbyte(PC++);
      break;
    case 0x1f:		/* RRA */
      tstates+=4;
      {
	BYTE bytetemp = A;
	A = ( A >> 1 ) | ( F << 7 );
	F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	  ( A & ( FLAG_3 | FLAG_5 ) ) | ( bytetemp & FLAG_C ) ;
      }
      break;
    case 0x20:		/* JR NZ,offset */
      tstates+=7;
      if( ! ( F & FLAG_Z ) ) { tstates+=5; JR(); }
      PC++;
      break;
    case 0x21:		/* LD HL,nnnn */
      tstates+=10;
      L=readbyte(PC++);
      H=readbyte(PC++);
      break;
    case 0x22:		/* LD (nnnn),HL */
      tstates+=16;
      LD16_NNRR(L,H);
      break;
    case 0x23:		/* INC HL */
      tstates+=6;
      HL++;
      break;
    case 0x24:		/* INC H */
      tstates+=4;
      INC(H);
      break;
    case 0x25:		/* DEC H */
      tstates+=4;
      DEC(H);
      break;
    case 0x26:		/* LD H,nn */
      tstates+=7;
      H=readbyte(PC++);
      break;
    case 0x27:		/* DAA */
      tstates+=4;
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
      break;
    case 0x28:		/* JR Z,offset */
      tstates+=7;
      if( F & FLAG_Z ) { tstates+=5; JR(); }
      PC++;
      break;
    case 0x29:		/* ADD HL,HL */
      tstates+=11;
      ADD16(HL,HL);
      break;
    case 0x2a:		/* LD HL,(nnnn) */
      tstates+=16;
      LD16_RRNN(L,H);
      break;
    case 0x2b:		/* DEC HL */
      tstates+=6;
      HL--;
      break;
    case 0x2c:		/* INC L */
      tstates+=4;
      INC(L);
      break;
    case 0x2d:		/* DEC L */
      tstates+=4;
      DEC(L);
      break;
    case 0x2e:		/* LD L,nn */
      tstates+=7;
      L=readbyte(PC++);
      break;
    case 0x2f:		/* CPL */
      tstates+=4;
      A ^= 0xff;
      F = ( F & ( FLAG_C | FLAG_P | FLAG_Z | FLAG_S ) ) |
	( A & ( FLAG_3 | FLAG_5 ) ) | ( FLAG_N | FLAG_H );
      break;
    case 0x30:		/* JR NC,offset */
      tstates+=7;
      if( ! ( F & FLAG_C ) ) { tstates+=5; JR(); }
      PC++;
      break;
    case 0x31:		/* LD SP,nnnn */
      tstates+=10;
      SPL=readbyte(PC++);
      SPH=readbyte(PC++);
      break;
    case 0x32:		/* LD (nnnn),A */
      tstates+=13;
      {
	WORD wordtemp=readbyte(PC++);
	wordtemp|=readbyte(PC++) << 8;
	writebyte(wordtemp,A);
      }
      break;
    case 0x33:		/* INC SP */
      tstates+=6;
      SP++;
      break;
    case 0x34:		/* INC (HL) */
      tstates+=11;
      {
	BYTE bytetemp=readbyte(HL);
	INC(bytetemp);
	writebyte(HL,bytetemp);
      }
      break;
    case 0x35:		/* DEC (HL) */
      tstates+=11;
      {
	BYTE bytetemp=readbyte(HL);
	DEC(bytetemp);
	writebyte(HL,bytetemp);
      }
      break;
    case 0x36:		/* LD (HL),nn */
      tstates+=10;
      writebyte(HL,readbyte(PC++));
      break;
    case 0x37:		/* SCF */
      tstates+=4;
      F = F | FLAG_C;
      break;
    case 0x38:		/* JR C,offset */
      tstates+=7;
      if( F & FLAG_C ) { tstates+=5; JR(); }
      PC++;
      break;
    case 0x39:		/* ADD HL,SP */
      tstates+=11;
      ADD16(HL,SP);
      break;
    case 0x3a:		/* LD A,(nnnn) */
      tstates+=13;
      {
	WORD wordtemp=readbyte(PC++);
	wordtemp|= ( readbyte(PC++) << 8 );
	A=readbyte(wordtemp);
      }
      break;
    case 0x3b:		/* DEC SP */
      tstates+=6;
      SP--;
      break;
    case 0x3c:		/* INC A */
      tstates+=4;
      INC(A);
      break;
    case 0x3d:		/* DEC A */
      tstates+=4;
      DEC(A);
      break;
    case 0x3e:		/* LD A,nn */
      tstates+=7;
      A=readbyte(PC++);
      break;
    case 0x3f:		/* CCF */
      tstates+=4;
      F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	( ( F & FLAG_C ) ? FLAG_H : FLAG_C ) | ( A & ( FLAG_3 | FLAG_5 ) );
      break;
    case 0x40:		/* LD B,B */
      tstates+=4;
      break;
    case 0x41:		/* LD B,C */
      tstates+=4;
      B=C;
      break;
    case 0x42:		/* LD B,D */
      tstates+=4;
      B=D;
      break;
    case 0x43:		/* LD B,E */
      tstates+=4;
      B=E;
      break;
    case 0x44:		/* LD B,H */
      tstates+=4;
      B=H;
      break;
    case 0x45:		/* LD B,L */
      tstates+=4;
      B=L;
      break;
    case 0x46:		/* LD B,(HL) */
      tstates+=7;
      B=readbyte(HL);
      break;
    case 0x47:		/* LD B,A */
      tstates+=4;
      B=A;
      break;
    case 0x48:		/* LD C,B */
      tstates+=4;
      C=B;
      break;
    case 0x49:		/* LD C,C */
      tstates+=4;
      break;
    case 0x4a:		/* LD C,D */
      tstates+=4;
      C=D;
      break;
    case 0x4b:		/* LD C,E */
      tstates+=4;
      C=E;
      break;
    case 0x4c:		/* LD C,H */
      tstates+=4;
      C=H;
      break;
    case 0x4d:		/* LD C,L */
      tstates+=4;
      C=L;
      break;
    case 0x4e:		/* LD C,(HL) */
      tstates+=7;
      C=readbyte(HL);
      break;
    case 0x4f:		/* LD C,A */
      tstates+=4;
      C=A;
      break;
    case 0x50:		/* LD D,B */
      tstates+=4;
      D=B;
      break;
    case 0x51:		/* LD D,C */
      tstates+=4;
      D=C;
      break;
    case 0x52:		/* LD D,D */
      tstates+=4;
      break;
    case 0x53:		/* LD D,E */
      tstates+=4;
      D=E;
      break;
    case 0x54:		/* LD D,H */
      tstates+=4;
      D=H;
      break;
    case 0x55:		/* LD D,L */
      tstates+=4;
      D=L;
      break;
    case 0x56:		/* LD D,(HL) */
      tstates+=7;
      D=readbyte(HL);
      break;
    case 0x57:		/* LD D,A */
      tstates+=4;
      D=A;
      break;
    case 0x58:		/* LD E,B */
      tstates+=4;
      E=B;
      break;
    case 0x59:		/* LD E,C */
      tstates+=4;
      E=C;
      break;
    case 0x5a:		/* LD E,D */
      tstates+=4;
      E=D;
      break;
    case 0x5b:		/* LD E,E */
      tstates+=4;
      break;
    case 0x5c:		/* LD E,H */
      tstates+=4;
      E=H;
      break;
    case 0x5d:		/* LD E,L */
      tstates+=4;
      E=L;
      break;
    case 0x5e:		/* LD E,(HL) */
      tstates+=7;
      E=readbyte(HL);
      break;
    case 0x5f:		/* LD E,A */
      tstates+=4;
      E=A;
      break;
    case 0x60:		/* LD H,B */
      tstates+=4;
      H=B;
      break;
    case 0x61:		/* LD H,C */
      tstates+=4;
      H=C;
      break;
    case 0x62:		/* LD H,D */
      tstates+=4;
      H=D;
      break;
    case 0x63:		/* LD H,E */
      tstates+=4;
      H=E;
      break;
    case 0x64:		/* LD H,H */
      tstates+=4;
      break;
    case 0x65:		/* LD H,L */
      tstates+=4;
      H=L;
      break;
    case 0x66:		/* LD H,(HL) */
      tstates+=7;
      H=readbyte(HL);
      break;
    case 0x67:		/* LD H,A */
      tstates+=4;
      H=A;
      break;
    case 0x68:		/* LD L,B */
      tstates+=4;
      L=B;
      break;
    case 0x69:		/* LD L,C */
      tstates+=4;
      L=C;
      break;
    case 0x6a:		/* LD L,D */
      tstates+=4;
      L=D;
      break;
    case 0x6b:		/* LD L,E */
      tstates+=4;
      L=E;
      break;
    case 0x6c:		/* LD L,H */
      tstates+=4;
      L=H;
      break;
    case 0x6d:		/* LD L,L */
      tstates+=4;
      break;
    case 0x6e:		/* LD L,(HL) */
      tstates+=7;
      L=readbyte(HL);
      break;
    case 0x6f:		/* LD L,A */
      tstates+=4;
      L=A;
      break;
    case 0x70:		/* LD (HL),B */
      tstates+=7;
      writebyte(HL,B);
      break;
    case 0x71:		/* LD (HL),C */
      tstates+=7;
      writebyte(HL,C);
      break;
    case 0x72:		/* LD (HL),D */
      tstates+=7;
      writebyte(HL,D);
      break;
    case 0x73:		/* LD (HL),E */
      tstates+=7;
      writebyte(HL,E);
      break;
    case 0x74:		/* LD (HL),H */
      tstates+=7;
      writebyte(HL,H);
      break;
    case 0x75:		/* LD (HL),L */
      tstates+=7;
      writebyte(HL,L);
      break;
    case 0x76:		/* HALT */
      tstates+=4;
      z80.halted=1;
      break;
    case 0x77:		/* LD (HL),A */
      tstates+=7;
      writebyte(HL,A);
      break;
    case 0x78:		/* LD A,B */
      tstates+=4;
      A=B;
      break;
    case 0x79:		/* LD A,C */
      tstates+=4;
      A=C;
      break;
    case 0x7a:		/* LD A,D */
      tstates+=4;
      A=D;
      break;
    case 0x7b:		/* LD A,E */
      tstates+=4;
      A=E;
      break;
    case 0x7c:		/* LD A,H */
      tstates+=4;
      A=H;
      break;
    case 0x7d:		/* LD A,L */
      tstates+=4;
      A=L;
      break;
    case 0x7e:		/* LD A,(HL) */
      tstates+=7;
      A=readbyte(HL);
      break;
    case 0x7f:		/* LD A,A */
      tstates+=4;
      break;
    case 0x80:		/* ADD A,B */
      tstates+=4;
      ADD(B);
      break;
    case 0x81:		/* ADD A,C */
      tstates+=4;
      ADD(C);
      break;
    case 0x82:		/* ADD A,D */
      tstates+=4;
      ADD(D);
      break;
    case 0x83:		/* ADD A,E */
      tstates+=4;
      ADD(E);
      break;
    case 0x84:		/* ADD A,H */
      tstates+=4;
      ADD(H);
      break;
    case 0x85:		/* ADD A,L */
      tstates+=4;
      ADD(L);
      break;
    case 0x86:		/* ADD A,(HL) */
      tstates+=7;
      {
	BYTE bytetemp=readbyte(HL);
	ADD(bytetemp);
      }
      break;
    case 0x87:		/* ADD A,A */
      tstates+=4;
      ADD(A);
      break;
    case 0x88:		/* ADC A,B */
      tstates+=4;
      ADC(B);
      break;
    case 0x89:		/* ADC A,C */
      tstates+=4;
      ADC(C);
      break;
    case 0x8a:		/* ADC A,D */
      tstates+=4;
      ADC(D);
      break;
    case 0x8b:		/* ADC A,E */
      tstates+=4;
      ADC(E);
      break;
    case 0x8c:		/* ADC A,H */
      tstates+=4;
      ADC(H);
      break;
    case 0x8d:		/* ADC A,L */
      tstates+=4;
      ADC(L);
      break;
    case 0x8e:		/* ADC A,(HL) */
      tstates+=7;
      {
	BYTE bytetemp=readbyte(HL);
	ADC(bytetemp);
      }
      break;
    case 0x8f:		/* ADC A,A */
      tstates+=4;
      ADC(A);
      break;
    case 0x90:		/* SUB A,B */
      tstates+=4;
      SUB(B);
      break;
    case 0x91:		/* SUB A,C */
      tstates+=4;
      SUB(C);
      break;
    case 0x92:		/* SUB A,D */
      tstates+=4;
      SUB(D);
      break;
    case 0x93:		/* SUB A,E */
      tstates+=4;
      SUB(E);
      break;
    case 0x94:		/* SUB A,H */
      tstates+=4;
      SUB(H);
      break;
    case 0x95:		/* SUB A,L */
      tstates+=4;
      SUB(L);
      break;
    case 0x96:		/* SUB A,(HL) */
      tstates+=7;
      {
	BYTE bytetemp=readbyte(HL);
	SUB(bytetemp);
      }
      break;
    case 0x97:		/* SUB A,A */
      tstates+=4;
      SUB(A);
      break;
    case 0x98:		/* SBC A,B */
      tstates+=4;
      SBC(B);
      break;
    case 0x99:		/* SBC A,C */
      tstates+=4;
      SBC(C);
      break;
    case 0x9a:		/* SBC A,D */
      tstates+=4;
      SBC(D);
      break;
    case 0x9b:		/* SBC A,E */
      tstates+=4;
      SBC(E);
      break;
    case 0x9c:		/* SBC A,H */
      tstates+=4;
      SBC(H);
      break;
    case 0x9d:		/* SBC A,L */
      tstates+=4;
      SBC(L);
      break;
    case 0x9e:		/* SBC A,(HL) */
      tstates+=7;
      {
	BYTE bytetemp=readbyte(HL);
	SBC(bytetemp);
      }
      break;
    case 0x9f:		/* SBC A,A */
      tstates+=4;
      SBC(A);
      break;
    case 0xa0:		/* AND A,B */
      tstates+=4;
      AND(B);
      break;
    case 0xa1:		/* AND A,C */
      tstates+=4;
      AND(C);
      break;
    case 0xa2:		/* AND A,D */
      tstates+=4;
      AND(D);
      break;
    case 0xa3:		/* AND A,E */
      tstates+=4;
      AND(E);
      break;
    case 0xa4:		/* AND A,H */
      tstates+=4;
      AND(H);
      break;
    case 0xa5:		/* AND A,L */
      tstates+=4;
      AND(L);
      break;
    case 0xa6:		/* AND A,(HL) */
      tstates+=7;
      {
	BYTE bytetemp=readbyte(HL);
	AND(bytetemp);
      }
      break;
    case 0xa7:		/* AND A,A */
      tstates+=4;
      AND(A);
      break;
    case 0xa8:		/* XOR A,B */
      tstates+=4; 
      XOR(B);
      break;
    case 0xa9:		/* XOR A,C */
      tstates+=4;
      XOR(C);
      break;
    case 0xaa:		/* XOR A,D */
      tstates+=4;
      XOR(D);
      break;
    case 0xab:		/* XOR A,E */
      tstates+=4;
      XOR(E);
      break;
    case 0xac:		/* XOR A,H */
      tstates+=4;
      XOR(H);
      break;
    case 0xad:		/* XOR A,L */
      tstates+=4;
      XOR(L);
      break;
    case 0xae:		/* XOR A,(HL) */
      tstates+=7;
      {
	BYTE bytetemp=readbyte(HL);
	XOR(bytetemp);
      }
      break;
    case 0xaf:		/* XOR A,A */
      tstates+=4;
      XOR(A);
      break;
    case 0xb0:		/* OR A,B */
      tstates+=4;
      OR(B);
      break;
    case 0xb1:		/* OR A,C */
      tstates+=4;
      OR(C);
      break;
    case 0xb2:		/* OR A,D */
      tstates+=4;
      OR(D);
      break;
    case 0xb3:		/* OR A,E */
      tstates+=4;
      OR(E);
      break;
    case 0xb4:		/* OR A,H */
      tstates+=4;
      OR(H);
      break;
    case 0xb5:		/* OR A,L */
      tstates+=4;
      OR(L);
      break;
    case 0xb6:		/* OR A,(HL) */
      tstates+=7;
      {
	BYTE bytetemp=readbyte(HL);
	OR(bytetemp);
      }
      break;
    case 0xb7:		/* OR A,A */
      tstates+=4;
      OR(A);
      break;
    case 0xb8:		/* CP B */
      tstates+=4;
      CP(B);
      break;
    case 0xb9:		/* CP C */
      tstates+=4;
      CP(C);
      break;
    case 0xba:		/* CP D */
      tstates+=4;
      CP(D);
      break;
    case 0xbb:		/* CP E */
      tstates+=4;
      CP(E);
      break;
    case 0xbc:		/* CP H */
      tstates+=4;
      CP(H);
      break;
    case 0xbd:		/* CP L */
      tstates+=4;
      CP(L);
      break;
    case 0xbe:		/* CP (HL) */
      tstates+=7;
      {
	BYTE bytetemp=readbyte(HL);
	CP(bytetemp);
      }
      break;
    case 0xbf:		/* CP A */
      tstates+=4;
      CP(A);
      break;
    case 0xc0:		/* RET NZ */
      tstates+=5;
      if(PC==0x056c) { tape_trap(); }
      else if( ! ( F & FLAG_Z ) ) { tstates+=6; RET(); }
      break;
    case 0xc1:		/* POP BC */
      tstates+=10;
      POP16(C,B);
      break;
    case 0xc2:		/* JP NZ,nnnn */
      tstates+=10;
      if ( ! ( F & FLAG_Z ) ) { JP(); }
      else PC+=2;
      break;
    case 0xc3:		/* JP nnnn */
      tstates+=10;
      JP();
      break;
    case 0xc4:		/* CALL NZ,nnnn */
      tstates+=10;
      if ( ! (F & FLAG_Z ) ) { tstates+=7; CALL(); }
      else PC+=2;
      break;
    case 0xc5:		/* PUSH BC */
      tstates+=11;
      PUSH16(C,B);
      break;
    case 0xc6:		/* ADD A,nn */
      tstates+=7;
      {
	BYTE bytetemp=readbyte(PC++);
	ADD(bytetemp);
      }
      break;
    case 0xc7:		/* RST 00 */
      tstates+=11;
      RST(0x00);
      break;
    case 0xc8:		/* RET Z */
      tstates+=5;
      if( F & FLAG_Z ) { tstates+=6; RET(); }
      break;
    case 0xc9:		/* RET */
      tstates+=10;
      RET();
      break;
    case 0xca:		/* JP Z,nnnn */
      tstates+=10;
      if ( F & FLAG_Z ) { JP(); }
      else PC+=2;
      break;
    case 0xcb:		/* CBxx opcodes */
      {
	BYTE opcode2=readbyte(PC++);
	R++;
#ifdef HAVE_ENOUGH_MEMORY
	switch(opcode2) {
#include "z80_cb.c"
	}
#else			/* #ifdef HAVE_ENOUGH_MEMORY */
	z80_cbxx(opcode2);
#endif			/* #ifdef HAVE_ENOUGH_MEMORY */
      }
      break;
    case 0xcc:		/* CALL Z,nnnn */
      tstates+=10;
      if ( F & FLAG_Z ) { tstates+=7; CALL(); }
      else PC+=2;
      break;
    case 0xcd:		/* CALL nnnn */
      tstates+=17;
      CALL();
      break;
    case 0xce:		/* ADC A,nn */
      tstates+=7;
      {
	BYTE bytetemp=readbyte(PC++);
	ADC(bytetemp);
      }
      break;
    case 0xcf:		/* RST 8 */
      tstates+=11;
      RST(0x08);
      break;
    case 0xd0:		/* RET NC */
      tstates+=5;
      if( ! ( F & FLAG_C ) ) { tstates+=6; RET(); }
      break;
    case 0xd1:		/* POP DE */
      tstates+=10;
      POP16(E,D);
      break;
    case 0xd2:		/* JP NC,nnnn */
      tstates+=10;
      if ( ! ( F & FLAG_C ) ) { JP(); }
      else PC+=2;
      break;
    case 0xd3:		/* OUT (nn),A */
      tstates+=11;
      writeport( readbyte(PC++) + ( A << 8 ) , A);
      break;
    case 0xd4:		/* CALL NC,nnnn */
      tstates+=10;
      if ( ! (F & FLAG_C ) ) { tstates+=7; CALL(); }
      else PC+=2;
      break;
    case 0xd5:		/* PUSH DE */
      tstates+=11;
      PUSH16(E,D);
      break;
    case 0xd6:		/* SUB nn */
      tstates+=7;
      {
	BYTE bytetemp=readbyte(PC++);
	SUB(bytetemp);
      }
      break;
    case 0xd7:		/* RST 10 */
      tstates+=11;
      RST(0x10);
      break;
    case 0xd8:		/* RET C */
      tstates+=5;
      if( F & FLAG_C ) { tstates+=6; RET(); }
      break;
    case 0xd9:		/* EXX */
      tstates+=4;
      {
	WORD wordtemp=BC; BC=BC_; BC_=wordtemp;
	wordtemp=DE; DE=DE_; DE_=wordtemp;
	wordtemp=HL; HL=HL_; HL_=wordtemp;
      }
      break;
    case 0xda:		/* JP C,nnnn */
      tstates+=10;
      if ( F & FLAG_C ) { JP(); }
      else PC+=2;
      break;
    case 0xdb:		/* IN A,(nn) */
      tstates+=11;
      A=readport( readbyte(PC++) + ( A << 8 ) );
      break;
    case 0xdc:		/* CALL C,nnnn */
      tstates+=10;
      if ( F & FLAG_C ) { tstates+=10; CALL(); }
      else PC+=2;
      break;
    case 0xdd:		/* DDxx opcodes */
      {
	BYTE opcode2=readbyte(PC++);
	R++;
#ifdef HAVE_ENOUGH_MEMORY
	switch(opcode2) {
#define REGISTER  IX
#define REGISTERL IXL
#define REGISTERH IXH
#include "z80_ddfd.c"
#undef REGISTERH
#undef REGISTERL
#undef REGISTER
	}
#else			/* #ifdef HAVE_ENOUGH_MEMORY */
	z80_ddxx(opcode2);
#endif			/* #ifdef HAVE_ENOUGH_MEMORY */
      }
      break;
    case 0xde:		/* SBC A,nn */
      tstates+=4;
      {
	BYTE bytetemp=readbyte(PC++);
	SBC(bytetemp);
      }
      break;
    case 0xdf:		/* RST 18 */
      tstates+=11;
      RST(0x18);
      break;
    case 0xe0:		/* RET PO */
      tstates+=5;
      if( ! ( F & FLAG_P ) ) { tstates+=6; RET(); }
      break;
    case 0xe1:		/* POP HL */
      tstates+=10;
      POP16(L,H);
      break;
    case 0xe2:		/* JP PO,nnnn */
      tstates+=10;
      if ( ! ( F & FLAG_P ) ) { JP(); }
      else PC+=2;
      break;
    case 0xe3:		/* EX (SP),HL */
      tstates+=19;
      {
	BYTE bytetempl=readbyte(SP), bytetemph=readbyte(SP+1);
	writebyte(SP,L); writebyte(SP+1,H);
	L=bytetempl; H=bytetemph;
      }
      break;
    case 0xe4:		/* CALL PO,nnnn */
      tstates+=10;
      if ( ! (F & FLAG_P ) ) { tstates+=7; CALL(); }
      else PC+=2;
      break;
    case 0xe5:		/* PUSH HL */
      tstates+=11;
      PUSH16(L,H);
      break;
    case 0xe6:		/* AND nn */
      tstates+=7;
      {
	BYTE bytetemp=readbyte(PC++);
	AND(bytetemp);
      }
      break;
    case 0xe7:		/* RST 20 */
      tstates+=11;
      RST(0x20);
      break;
    case 0xe8:		/* RET PE */
      tstates+=10;
      if( F & FLAG_P ) { tstates+=7; RET(); }
      break;
    case 0xe9:		/* JP HL */
      tstates+=4;
      PC=HL;		/* NB: NOT INDIRECT! */
      break;
    case 0xea:		/* JP PE,nnnn */
      tstates+=10;
      if ( F & FLAG_P ) { JP(); }
      else PC+=2;
      break;
    case 0xeb:		/* EX DE,HL */
      tstates+=4;
      {
	WORD wordtemp=DE; DE=HL; HL=wordtemp;
      }
      break;
    case 0xec:		/* CALL PE,nnnn */
      tstates+=10;
      if ( F & FLAG_P ) { tstates+=7; CALL(); }
      else PC+=2;
      break;
    case 0xed:		/* EDxx opcodes */
      {
	BYTE opcode2=readbyte(PC++);
	R++;
#ifdef HAVE_ENOUGH_MEMORY
	switch(opcode2) {
#include "z80_ed.c"
	}
#else			/* #ifdef HAVE_ENOUGH_MEMORY */
	z80_edxx(opcode2);
#endif			/* #ifdef HAVE_ENOUGH_MEMORY */
      }
      break;
    case 0xee:		/* XOR A,nn */
      tstates+=7;
      {
	BYTE bytetemp=readbyte(PC++);
	XOR(bytetemp);
      }
      break;
    case 0xef:		/* RST 28 */
      tstates+=11;
      RST(0x28);
      break;
    case 0xf0:		/* RET P */
      tstates+=5;
      if( ! ( F & FLAG_S ) ) { tstates+=6; RET(); }
      break;
    case 0xf1:		/* POP AF */
      tstates+=10;
      POP16(F,A);
      break;
    case 0xf2:		/* JP P,nnnn */
      tstates+=10;
      if ( ! ( F & FLAG_S ) ) { JP(); }
      else PC+=2;
      break;
    case 0xf3:		/* DI */
      tstates+=4;
      IFF1=IFF2=0;
      break;
    case 0xf4:		/* CALL P,nnnn */
      tstates+=10;
      if ( ! (F & FLAG_S ) ) { tstates+=7; CALL(); }
      else PC+=2;
      break;
    case 0xf5:		/* PUSH AF */
      tstates+=11;
      PUSH16(F,A);
      break;
    case 0xf6:		/* OR nn */
      tstates+=7;
      {
	BYTE bytetemp=readbyte(PC++);
	OR(bytetemp);
      }
      break;
    case 0xf7:		/* RST 30 */
      tstates+=11;
      RST(0x30);
      break;
    case 0xf8:		/* RET M */
      tstates+=5;
      if( F & FLAG_S ) { tstates+=6; RET(); }
      break;
    case 0xf9:		/* LD SP,HL */
      tstates+=6;
      SP=HL;
      break;
    case 0xfa:		/* JP M,nnnn */
      tstates+=10;
      if ( F & FLAG_S ) { JP(); }
      else PC+=2;
      break;
    case 0xfb:		/* EI */
      tstates+=4;
      IFF1=IFF2=1;
      break;
    case 0xfc:		/* CALL M,nnnn */
      tstates+=10;
      if ( F & FLAG_S ) { tstates+=7; CALL(); }
      else PC+=2;
      break;
    case 0xfd:		/* FDxx opcodes */
      {
	BYTE opcode2=readbyte(PC++);
	R++;
#ifdef HAVE_ENOUGH_MEMORY
	switch(opcode2) {
#define REGISTER  IY
#define REGISTERL IYL
#define REGISTERH IYH
#include "z80_ddfd.c"
#undef REGISTERH
#undef REGISTERL
#undef REGISTER
	}
#else			/* #ifdef HAVE_ENOUGH_MEMORY */
	z80_fdxx(opcode2);
#endif			/* #ifdef HAVE_ENOUGH_MEMORY */
      }
      break;
    case 0xfe:		/* CP nn */
      tstates+=7;
      {
	BYTE bytetemp=readbyte(PC++);
	CP(bytetemp);
      }
      break;
    case 0xff:		/* RST 38 */
      tstates+=11;
      RST(0x38);
      break;

    }			/* Matches switch(opcode) { */

  }			/* Matches while( tstates < event_next_event ) { */

}

#ifndef HAVE_ENOUGH_MEMORY

static void z80_cbxx(BYTE opcode2)
{
  switch(opcode2) {
#include "z80_cb.c"
  }
}

static void z80_ddxx(BYTE opcode2)
{
  switch(opcode2) {
#define REGISTER  IX
#define REGISTERL IXL
#define REGISTERH IXH
#include "z80_ddfd.c"
#undef REGISTERH
#undef REGISTERL
#undef REGISTER
  }
}

static void z80_edxx(BYTE opcode2)
{
  switch(opcode2) {
#include "z80_ed.c"
  }
}

static void z80_fdxx(BYTE opcode2)
{
  switch(opcode2) {
#define REGISTER  IY
#define REGISTERL IYL
#define REGISTERH IYH
#include "z80_ddfd.c"
#undef REGISTERH
#undef REGISTERL
#undef REGISTER
  }
}

static void z80_ddfdcbxx(BYTE opcode3, WORD tempaddr)
{
  switch(opcode3) {
#include "z80_ddfdcb.c"
  }
}

#endif			/* #ifndef HAVE_ENOUGH_MEMORY */
