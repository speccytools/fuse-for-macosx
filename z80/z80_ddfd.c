/* z80_ddfd.c: z80 DDxx and FDxx opcodes
   Copyright (c) 1999-2001 Philip Kendall

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

/* define the macros REGISTER, REGISTERL and REGISTERH to be IX,IXL
   and IXH or IY,IYL and IYH to select which register to use
*/

#if !defined(REGISTER) || !defined(REGISTERL) || !defined(REGISTERH)
#error Macros `REGISTER', `REGISTERL' and `REGISTERH' must be defined before including `z80_ddfd.c'.
#endif

case 0x09:		/* ADD REGISTER,BC */
ADD16(REGISTER,BC);
break;

case 0x19:		/* ADD REGISTER,DE */
ADD16(REGISTER,DE);
break;

case 0x21:		/* LD REGISTER,nnnn */
contend( PC, 3 );
REGISTERL=readbyte(PC++);
contend( PC, 3 );
REGISTERH=readbyte(PC++);
break;

case 0x22:		/* LD (nnnn),REGISTER */
LD16_NNRR(REGISTERL,REGISTERH);
break;

case 0x23:		/* INC REGISTER */
tstates += 2;
REGISTER++;
break;

case 0x24:		/* INC REGISTERH */
INC(REGISTERH);
break;

case 0x25:		/* DEC REGISTERH */
DEC(REGISTERH);
break;

case 0x26:		/* LD REGISTERH,nn */
contend( PC, 3 );
REGISTERH=readbyte(PC++);
break;

case 0x29:		/* ADD REGISTER,REGISTER */
ADD16(REGISTER,REGISTER);
break;

case 0x2a:		/* LD REGISTER,(nnnn) */
LD16_RRNN(REGISTERL,REGISTERH);
break;

case 0x2b:		/* DEC REGISTER */
tstates += 2;
REGISTER--;
break;

case 0x2c:		/* INC REGISTERL */
INC(REGISTERL);
break;

case 0x2d:		/* DEC REGISTERL */
DEC(REGISTERL);
break;

case 0x2e:		/* LD REGISTERL,nn */
contend( PC, 3 );
REGISTERL=readbyte(PC++);
break;

case 0x34:		/* INC (REGISTER+dd) */
tstates += 15;		/* FIXME: how is this contended? */
{
  WORD wordtemp=REGISTER+(SBYTE)readbyte(PC++);
  BYTE bytetemp=readbyte(wordtemp);
  INC(bytetemp);
  writebyte(wordtemp,bytetemp);
}
break;

case 0x35:		/* DEC (REGISTER+dd) */
tstates += 15;		/* FIXME: how is this contended? */
{
  WORD wordtemp=REGISTER+(SBYTE)readbyte(PC++);
  BYTE bytetemp=readbyte(wordtemp);
  DEC(bytetemp);
  writebyte(wordtemp,bytetemp);
}
break;

case 0x36:		/* LD (REGISTER+dd),nn */
tstates += 11;		/* FIXME: how is this contended? */
{
  WORD wordtemp=REGISTER+(SBYTE)readbyte(PC++);
  writebyte(wordtemp,readbyte(PC++));
}
break;

case 0x39:		/* ADD REGISTER,SP */
ADD16(REGISTER,SP);
break;

case 0x44:		/* LD B,REGISTERH */
B=REGISTERH;
break;

case 0x45:		/* LD B,REGISTERL */
B=REGISTERL;
break;

case 0x46:		/* LD B,(REGISTER+dd) */
tstates += 11;		/* FIXME: how is this contended? */
B=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
break;

case 0x4c:		/* LD C,REGISTERH */
C=REGISTERH;
break;

case 0x4d:		/* LD C,REGISTERL */
C=REGISTERL;
break;

case 0x4e:		/* LD C,(REGISTER+dd) */
tstates += 11;		/* FIXME: how is this contended? */
C=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
break;

case 0x54:		/* LD D,REGISTERH */
D=REGISTERH;
break;

case 0x55:		/* LD D,REGISTERL */
D=REGISTERL;
break;

case 0x56:		/* LD D,(REGISTER+dd) */
tstates += 11;		/* FIXME: how is this contended? */
D=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
break;

case 0x5c:		/* LD E,REGISTERH */
E=REGISTERH;
break;

case 0x5d:		/* LD E,REGISTERL */
E=REGISTERL;
break;

case 0x5e:		/* LD E,(REGISTER+dd) */
tstates += 11;		/* FIXME: how is this contended? */
E=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
break;

case 0x60:		/* LD REGISTERH,B */
REGISTERH=B;
break;

case 0x61:		/* LD REGISTERH,C */
REGISTERH=C;
break;

case 0x62:		/* LD REGISTERH,D */
REGISTERH=D;
break;

case 0x63:		/* LD REGISTERH,E */
REGISTERH=E;
break;

case 0x64:		/* LD REGISTERH,REGISTERH */
break;

case 0x65:		/* LD REGISTERH,REGISTERL */
REGISTERH=REGISTERL;
break;

case 0x66:		/* LD H,(REGISTER+dd) */
tstates += 11;		/* FIXME: how is this contended? */
H=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
break;

case 0x67:		/* LD REGISTERH,A */
REGISTERH=A;
break;

case 0x68:		/* LD REGISTERL,B */
REGISTERL=B;
break;

case 0x69:		/* LD REGISTERL,C */
REGISTERL=C;
break;

case 0x6a:		/* LD REGISTERL,D */
REGISTERL=D;
break;

case 0x6b:		/* LD REGISTERL,E */
REGISTERL=E;
break;

case 0x6c:		/* LD REGISTERL,REGISTERH */
REGISTERL=REGISTERH;
break;

case 0x6d:		/* LD REGISTERL,REGISTERL */
break;

case 0x6e:		/* LD L,(REGISTER+dd) */
tstates += 11;		/* FIXME: how is this contended? */
L=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
break;

case 0x6f:		/* LD REGISTERL,A */
REGISTERL=A;
break;

case 0x70:		/* LD (REGISTER+dd),B */
tstates += 11;		/* FIXME: how is this contended? */
writebyte( REGISTER + (SBYTE)readbyte(PC++), B);
break;

case 0x71:		/* LD (REGISTER+dd),C */
tstates += 11;		/* FIXME: how is this contended? */
writebyte( REGISTER + (SBYTE)readbyte(PC++), C);
break;

case 0x72:		/* LD (REGISTER+dd),D */
tstates += 11;		/* FIXME: how is this contended? */
writebyte( REGISTER + (SBYTE)readbyte(PC++), D);
break;

case 0x73:		/* LD (REGISTER+dd),E */
tstates += 11;		/* FIXME: how is this contended? */
writebyte( REGISTER + (SBYTE)readbyte(PC++), E);
break;

case 0x74:		/* LD (REGISTER+dd),H */
tstates += 11;		/* FIXME: how is this contended? */
writebyte( REGISTER + (SBYTE)readbyte(PC++), H);
break;

case 0x75:		/* LD (REGISTER+dd),L */
tstates += 11;		/* FIXME: how is this contended? */
writebyte( REGISTER + (SBYTE)readbyte(PC++), L);
break;

case 0x77:		/* LD (REGISTER+dd),A */
tstates += 11;		/* FIXME: how is this contended? */
writebyte( REGISTER + (SBYTE)readbyte(PC++), A);
break;

case 0x7c:		/* LD A,REGISTERH */
A=REGISTERH;
break;

case 0x7d:		/* LD A,REGISTERL */
A=REGISTERL;
break;

case 0x7e:		/* LD A,(REGISTER+dd) */
tstates += 11;		/* FIXME: how is this contended? */
A=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
break;

case 0x84:		/* ADD A,REGISTERH */
ADD(REGISTERH);
break;

case 0x85:		/* ADD A,REGISTERL */
ADD(REGISTERL);
break;

case 0x86:		/* ADD A,(REGISTER+dd) */
tstates += 11;		/* FIXME: how is this contended? */
{
  BYTE bytetemp=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
  ADD(bytetemp);
}
break;

case 0x8c:		/* ADC A,REGISTERH */
ADC(REGISTERH);
break;

case 0x8d:		/* ADC A,REGISTERL */
ADC(REGISTERL);
break;

case 0x8e:		/* ADC A,(REGISTER+dd) */
tstates += 11;		/* FIXME: how is this contended? */
{
  BYTE bytetemp=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
  ADC(bytetemp);
}
break;

case 0x94:		/* SUB A,REGISTERH */
SUB(REGISTERH);
break;

case 0x95:		/* SUB A,REGISTERL */
SUB(REGISTERL);
break;

case 0x96:		/* SUB A,(REGISTER+dd) */
tstates += 11;		/* FIXME: how is this contended? */
{
  BYTE bytetemp=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
  SUB(bytetemp);
}
break;

case 0x9c:		/* SBC A,REGISTERH */
SBC(REGISTERH);
break;

case 0x9d:		/* SBC A,REGISTERL */
SBC(REGISTERL);
break;

case 0x9e:		/* SBC A,(REGISTER+dd) */
tstates += 11;		/* FIXME: how is this contended? */
{
  BYTE bytetemp=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
  SBC(bytetemp);
}
break;

case 0xa4:		/* AND A,REGISTERH */
AND(REGISTERH);
break;

case 0xa5:		/* AND A,REGISTERL */
AND(REGISTERL);
break;

case 0xa6:		/* AND A,(REGISTER+dd) */
tstates += 11;		/* FIXME: how is this contended? */
{
  BYTE bytetemp=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
  AND(bytetemp);
}
break;

case 0xac:		/* XOR A,REGISTERH */
XOR(REGISTERH);
break;

case 0xad:		/* XOR A,REGISTERL */
XOR(REGISTERL);
break;

case 0xae:		/* XOR A,(REGISTER+dd) */
tstates += 11;		/* FIXME: how is this contended? */
{
  BYTE bytetemp=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
  XOR(bytetemp);
}
break;

case 0xb4:		/* OR A,REGISTERH */
OR(REGISTERH);
break;

case 0xb5:		/* OR A,REGISTERL */
OR(REGISTERL);
break;

case 0xb6:		/* OR A,(REGISTER+dd) */
tstates += 11;		/* FIXME: how is this contended? */
{
  BYTE bytetemp=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
  OR(bytetemp);
}
break;

case 0xbc:		/* CP A,REGISTERH */
CP(REGISTERH);
break;

case 0xbd:		/* CP A,REGISTERL */
CP(REGISTERL);
break;

case 0xbe:		/* CP A,(REGISTER+dd) */
tstates += 11;		/* FIXME: how is this contended? */
{
  BYTE bytetemp=readbyte( REGISTER + (SBYTE)readbyte(PC++) );
  CP(bytetemp);
}
break;

/* FIXME: contention here is just a guess */
case 0xcb:		/* {DD,FD}CBxx opcodes */
{
  WORD tempaddr; BYTE opcode3;
  contend( PC, 3 );
  tempaddr = REGISTER + (SBYTE)readbyte(PC++);
  contend( PC, 4 );
  opcode3 = readbyte( PC++ );
#ifdef HAVE_ENOUGH_MEMORY
  switch(opcode3) {
#include "z80_ddfdcb.c"
  }
#else			/* #ifdef HAVE_ENOUGH_MEMORY */
  z80_ddfdcbxx(opcode3,tempaddr);
#endif			/* #ifdef HAVE_ENOUGH_MEMORY */
}
break;

case 0xe1:		/* POP REGISTER */
POP16(REGISTERL,REGISTERH);
break;

case 0xe3:		/* EX (SP),REGISTER */
{
  BYTE bytetempl=readbyte(SP), bytetemph=readbyte(SP+1);
  contend( SP, 3 ); contend( SP+1, 4 );
  writebyte(SP,REGISTERL); writebyte(SP+1,REGISTERH);
  contend( SP, 3 ); contend( SP+1, 5 );
  REGISTERL=bytetempl; REGISTERH=bytetemph;
}
break;

case 0xe5:		/* PUSH REGISTER */
tstates++;
PUSH16(REGISTERL,REGISTERH);
break;

case 0xe9:		/* JP REGISTER */
PC=REGISTER;		/* NB: NOT INDIRECT! */
break;

/* Note EB (EX DE,HL) does not get modified to use either IX or IY;
   this is because all EX DE,HL does is switch an internal flip-flop
   in the Z80 which says which way round DE and HL are, which can't
   be used with IX or IY. (This is also why EX DE,HL is very quick
   at only 4 T states).
*/

case 0xf9:		/* LD SP,REGISTER */
tstates += 2;
SP=REGISTER;
break;

default:		/* Instruction did not involve H or L, so backtrack
			   one instruction and parse again */
PC--;			/* FIXME: will be contended again */
R--; rzx_instructions--; /* Decrement the instruction counters as well */
break;
