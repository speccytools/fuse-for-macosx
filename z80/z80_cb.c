/* z80_cb.c: z80 CBxx opcodes
   Copyright (c) 1999 Philip Kendall

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

   E-mail: pak21@cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

case 0x00:	/* RLC B */
tstates+=8;
RLC(B);
break;

case 0x01:	/* RLC C */
tstates+=8;
RLC(C);
break;

case 0x02:	/* RLC D */
tstates+=8;
RLC(D);
break;

case 0x03:	/* RLC E */
tstates+=8;
RLC(E);
break;

case 0x04:	/* RLC H */
tstates+=8;
RLC(H);
break;

case 0x05:	/* RLC L */
tstates+=8;
RLC(L);
break;

case 0x06:	/* RLC (HL) */
tstates+=15;
{
  BYTE bytetemp = readbyte(HL);
  RLC(bytetemp);
  writebyte(HL,bytetemp);
}
break;

case 0x07:	/* RLC A */
tstates+=8;
RLC(A);
break;

case 0x08:	/* RRC B */
tstates+=8;
RRC(B);
break;

case 0x09:	/* RRC C */
tstates+=8;
RRC(C);
break;

case 0x0a:	/* RRC D */
tstates+=8;
RRC(D);
break;

case 0x0b:	/* RRC E */
tstates+=8;
RRC(E);
break;

case 0x0c:	/* RRC H */
tstates+=8;
RRC(H);
break;

case 0x0d:	/* RRC L */
tstates+=8;
RRC(L);
break;

case 0x0e:	/* RRC (HL) */
tstates+=15;
{
  BYTE bytetemp = readbyte(HL);
  RRC(bytetemp);
  writebyte(HL,bytetemp);
}
break;

case 0x0f:	/* RRC A */
tstates+=8;
RRC(A);
break;

case 0x10:	/* RL B */
tstates+=8;
RL(B);
break;

case 0x11:	/* RL C */
tstates+=8;
RL(C);
break;

case 0x12:	/* RL D */
tstates+=8;
RL(D);
break;

case 0x13:	/* RL E */
tstates+=8;
RL(E);
break;

case 0x14:	/* RL H */
tstates+=8;
RL(H);
break;

case 0x15:	/* RL L */
tstates+=8;
RL(L);
break;

case 0x16:	/* RL (HL) */
tstates+=15;
{
  BYTE bytetemp = readbyte(HL);
  RL(bytetemp);
  writebyte(HL,bytetemp);
}
break;

case 0x17:	/* RL A */
tstates+=8;
RL(A);
break;

case 0x18:	/* RR B */
tstates+=8;
RR(B);
break;

case 0x19:	/* RR C */
tstates+=8;
RR(C);
break;

case 0x1a:	/* RR D */
tstates+=8;
RR(D);
break;

case 0x1b:	/* RR E */
tstates+=8;
RR(E);
break;

case 0x1c:	/* RR H */
tstates+=8;
RR(H);
break;

case 0x1d:	/* RR L */
tstates+=8;
RR(L);
break;

case 0x1e:	/* RR (HL) */
tstates+=15;
{
  BYTE bytetemp = readbyte(HL);
  RR(bytetemp);
  writebyte(HL,bytetemp);
}
break;

case 0x1f:	/* RR A */
tstates+=8;
RR(A);
break;

case 0x20:	/* SLA B */
tstates+=8;
SLA(B);
break;

case 0x21:	/* SLA C */
tstates+=8;
SLA(C);
break;

case 0x22:	/* SLA D */
tstates+=8;
SLA(D);
break;

case 0x23:	/* SLA E */
tstates+=8;
SLA(E);
break;

case 0x24:	/* SLA H */
tstates+=8;
SLA(H);
break;

case 0x25:	/* SLA L */
tstates+=8;
SLA(L);
break;

case 0x26:	/* SLA (HL) */
tstates+=15;
{
  BYTE bytetemp = readbyte(HL);
  SLA(bytetemp);
  writebyte(HL,bytetemp);
}
break;

case 0x27:	/* SLA A */
tstates+=8;
SLA(A);
break;

case 0x28:	/* SRA B */
tstates+=8;
SRA(B);
break;

case 0x29:	/* SRA C */
tstates+=8;
SRA(C);
break;

case 0x2a:	/* SRA D */
tstates+=8;
SRA(D);
break;

case 0x2b:	/* SRA E */
tstates+=8;
SRA(E);
break;

case 0x2c:	/* SRA H */
tstates+=8;
SRA(H);
break;

case 0x2d:	/* SRA L */
tstates+=8;
SRA(L);
break;

case 0x2e:	/* SRA (HL) */
tstates+=15;
{
  BYTE bytetemp = readbyte(HL);
  SRA(bytetemp);
  writebyte(HL,bytetemp);
}
break;

case 0x2f:	/* SRA A */
tstates+=8;
SRA(A);
break;

case 0x30:	/* SLL B */
tstates+=8;
SLL(B);
break;

case 0x31:	/* SLL C */
tstates+=8;
SLL(C);
break;

case 0x32:	/* SLL D */
tstates+=8;
SLL(D);
break;

case 0x33:	/* SLL E */
tstates+=8;
SLL(E);
break;

case 0x34:	/* SLL H */
tstates+=8;
SLL(H);
break;

case 0x35:	/* SLL L */
tstates+=8;
SLL(L);
break;

case 0x36:	/* SLL (HL) */
tstates+=15;
{
  BYTE bytetemp = readbyte(HL);
  SLL(bytetemp);
  writebyte(HL,bytetemp);
}
break;

case 0x37:	/* SLL A */
tstates+=8;
SLL(A);
break;

case 0x38:	/* SRL B */
tstates+=8;
SRL(B);
break;

case 0x39:	/* SRL C */
tstates+=8;
SRL(C);
break;

case 0x3a:	/* SRL D */
tstates+=8;
SRL(D);
break;

case 0x3b:	/* SRL E */
tstates+=8;
SRL(E);
break;

case 0x3c:	/* SRL H */
tstates+=8;
SRL(H);
break;

case 0x3d:	/* SRL L */
tstates+=8;
SRL(L);
break;

case 0x3e:	/* SRL (HL) */
tstates+=15;
{
  BYTE bytetemp = readbyte(HL);
  SRL(bytetemp);
  writebyte(HL,bytetemp);
}
break;

case 0x3f:	/* SRL A */
SRL(A);
break;

case 0x40:	/* BIT 0,B */
tstates+=8;
BIT(0,B);
break;

case 0x41:	/* BIT 0,C */
tstates+=8;
BIT(0,C);
break;

case 0x42:	/* BIT 0,D */
tstates+=8;
BIT(0,D);
break;

case 0x43:	/* BIT 0,E */
tstates+=8;
BIT(0,E);
break;

case 0x44:	/* BIT 0,H */
tstates+=8;
BIT(0,H);
break;

case 0x45:	/* BIT 0,L */
tstates+=8;
BIT(0,L);
break;

case 0x46:	/* BIT 0,(HL) */
tstates+=12;
{
  BYTE bytetemp = readbyte(HL);
  BIT(0,bytetemp);
}
break;

case 0x47:	/* BIT 0,A */
tstates+=8;
BIT(0,A);
break;

case 0x48:	/* BIT 1,B */
tstates+=8;
BIT(1,B);
break;

case 0x49:	/* BIT 1,C */
tstates+=8;
BIT(1,C);
break;

case 0x4a:	/* BIT 1,D */
tstates+=8;
BIT(1,D);
break;

case 0x4b:	/* BIT 1,E */
tstates+=8;
BIT(1,E);
break;

case 0x4c:	/* BIT 1,H */
tstates+=8;
BIT(1,H);
break;

case 0x4d:	/* BIT 1,L */
tstates+=8;
BIT(1,L);
break;

case 0x4e:	/* BIT 1,(HL) */
tstates+=12;
{
  BYTE bytetemp = readbyte(HL);
  BIT(1,bytetemp);
}
break;

case 0x4f:	/* BIT 1,A */
tstates+=8;
BIT(1,A);
break;

case 0x50:	/* BIT 2,B */
tstates+=8;
BIT(2,B);
break;

case 0x51:	/* BIT 2,C */
tstates+=8;
BIT(2,C);
break;

case 0x52:	/* BIT 2,D */
tstates+=8;
BIT(2,D);
break;

case 0x53:	/* BIT 2,E */
tstates+=8;
BIT(2,E);
break;

case 0x54:	/* BIT 2,H */
tstates+=8;
BIT(2,H);
break;

case 0x55:	/* BIT 2,L */
tstates+=8;
BIT(2,L);
break;

case 0x56:	/* BIT 2,(HL) */
tstates+=12;
{
  BYTE bytetemp = readbyte(HL);
  BIT(2,bytetemp);
}
break;

case 0x57:	/* BIT 2,A */
tstates+=8;
BIT(2,A);
break;

case 0x58:	/* BIT 3,B */
tstates+=8;
BIT(3,B);
break;

case 0x59:	/* BIT 3,C */
tstates+=8;
BIT(3,C);
break;

case 0x5a:	/* BIT 3,D */
tstates+=8;
BIT(3,D);
break;

case 0x5b:	/* BIT 3,E */
tstates+=8;
BIT(3,E);
break;

case 0x5c:	/* BIT 3,H */
tstates+=8;
BIT(3,H);
break;

case 0x5d:	/* BIT 3,L */
tstates+=8;
BIT(3,L);
break;

case 0x5e:	/* BIT 3,(HL) */
tstates+=12;
{
  BYTE bytetemp = readbyte(HL);
  BIT(3,bytetemp);
}
break;

case 0x5f:	/* BIT 3,A */
tstates+=8;
BIT(3,A);
break;

case 0x60:	/* BIT 4,B */
tstates+=8;
BIT(4,B);
break;

case 0x61:	/* BIT 4,C */
tstates+=8;
BIT(4,C);
break;

case 0x62:	/* BIT 4,D */
tstates+=8;
BIT(4,D);
break;

case 0x63:	/* BIT 4,E */
tstates+=8;
BIT(4,E);
break;

case 0x64:	/* BIT 4,H */
tstates+=8;
BIT(4,H);
break;

case 0x65:	/* BIT 4,L */
tstates+=8;
BIT(4,L);
break;

case 0x66:	/* BIT 4,(HL) */
tstates+=12;
{
  BYTE bytetemp = readbyte(HL);
  BIT(4,bytetemp);
}
break;

case 0x67:	/* BIT 4,A */
tstates+=8;
BIT(4,A);
break;

case 0x68:	/* BIT 5,B */
tstates+=8;
BIT(5,B);
break;

case 0x69:	/* BIT 5,C */
tstates+=8;
BIT(5,C);
break;

case 0x6a:	/* BIT 5,D */
tstates+=8;
BIT(5,D);
break;

case 0x6b:	/* BIT 5,E */
tstates+=8;
BIT(5,E);
break;

case 0x6c:	/* BIT 5,H */
tstates+=8;
BIT(5,H);
break;

case 0x6d:	/* BIT 5,L */
tstates+=8;
BIT(5,L);
break;

case 0x6e:	/* BIT 5,(HL) */
tstates+=12;
{
  BYTE bytetemp = readbyte(HL);
  BIT(5,bytetemp);
}
break;

case 0x6f:	/* BIT 5,A */
tstates+=8;
BIT(5,A);
break;

case 0x70:	/* BIT 6,B */
tstates+=8;
BIT(6,B);
break;

case 0x71:	/* BIT 6,C */
tstates+=8;
BIT(6,C);
break;

case 0x72:	/* BIT 6,D */
tstates+=8;
BIT(6,D);
break;

case 0x73:	/* BIT 6,E */
tstates+=8;
BIT(6,E);
break;

case 0x74:	/* BIT 6,H */
tstates+=8;
BIT(6,H);
break;

case 0x75:	/* BIT 6,L */
tstates+=8;
BIT(6,L);
break;

case 0x76:	/* BIT 6,(HL) */
tstates+=12;
{
  BYTE bytetemp = readbyte(HL);
  BIT(6,bytetemp);
}
break;

case 0x77:	/* BIT 6,A */
tstates+=8;
BIT(6,A);
break;

case 0x78:	/* BIT 7,B */
tstates+=8;
BIT7(B);
break;

case 0x79:	/* BIT 7,C */
tstates+=8;
BIT7(C);
break;

case 0x7a:	/* BIT 7,D */
tstates+=8;
BIT7(D);
break;

case 0x7b:	/* BIT 7,E */
tstates+=8;
BIT7(E);
break;

case 0x7c:	/* BIT 7,H */
tstates+=8;
BIT7(H);
break;

case 0x7d:	/* BIT 7,L */
tstates+=8;
BIT7(L);
break;

case 0x7e:	/* BIT 7,(HL) */
tstates+=12;
{
  BYTE bytetemp = readbyte(HL);
  BIT7(bytetemp);
}
break;

case 0x7f:	/* BIT 7,A */
tstates+=8;
BIT7(A);
break;

case 0x80:	/* RES 0,B */
tstates+=8;
B &= 0xfe;
break;

case 0x81:	/* RES 0,C */
tstates+=8;
C &= 0xfe;
break;

case 0x82:	/* RES 0,D */
tstates+=8;
D &= 0xfe;
break;

case 0x83:	/* RES 0,E */
tstates+=8;
E &= 0xfe;
break;

case 0x84:	/* RES 0,H */
tstates+=8;
H &= 0xfe;
break;

case 0x85:	/* RES 0,L */
tstates+=8;
L &= 0xfe;
break;

case 0x86:	/* RES 0,(HL) */
tstates+=15;
writebyte(HL, readbyte(HL) & 0xfe);
break;

case 0x87:	/* RES 0,A */
tstates+=8;
A &= 0xfe;
break;

case 0x88:	/* RES 1,B */
tstates+=8;
B &= 0xfd;
break;

case 0x89:	/* RES 1,C */
tstates+=8;
C &= 0xfd;
break;

case 0x8a:	/* RES 1,D */
tstates+=8;
D &= 0xfd;
break;

case 0x8b:	/* RES 1,E */
tstates+=8;
E &= 0xfd;
break;

case 0x8c:	/* RES 1,H */
tstates+=8;
H &= 0xfd;
break;

case 0x8d:	/* RES 1,L */
tstates+=8;
L &= 0xfd;
break;

case 0x8e:	/* RES 1,(HL) */
tstates+=15;
writebyte(HL, readbyte(HL) & 0xfd);
break;

case 0x8f:	/* RES 1,A */
tstates+=8;
A &= 0xfd;
break;

case 0x90:	/* RES 2,B */
tstates+=8;
B &= 0xfb;
break;

case 0x91:	/* RES 2,C */
tstates+=8;
C &= 0xfb;
break;

case 0x92:	/* RES 2,D */
tstates+=8;
D &= 0xfb;
break;

case 0x93:	/* RES 2,E */
tstates+=8;
E &= 0xfb;
break;

case 0x94:	/* RES 2,H */
tstates+=8;
H &= 0xfb;
break;

case 0x95:	/* RES 2,L */
tstates+=8;
L &= 0xfb;
break;

case 0x96:	/* RES 2,(HL) */
tstates+=15;
writebyte(HL, readbyte(HL) & 0xfb);
break;

case 0x97:	/* RES 2,A */
tstates+=8;
A &= 0xfb;
break;

case 0x98:	/* RES 3,B */
tstates+=8;
B &= 0xf7;
break;

case 0x99:	/* RES 3,C */
tstates+=8;
C &= 0xf7;
break;

case 0x9a:	/* RES 3,D */
tstates+=8;
D &= 0xf7;
break;

case 0x9b:	/* RES 3,E */
tstates+=8;
E &= 0xf7;
break;

case 0x9c:	/* RES 3,H */
tstates+=8;
H &= 0xf7;
break;

case 0x9d:	/* RES 3,L */
tstates+=8;
L &= 0xf7;
break;

case 0x9e:	/* RES 3,(HL) */
tstates+=15;
writebyte(HL, readbyte(HL) & 0xf7);
break;

case 0x9f:	/* RES 3,A */
tstates+=8;
A &= 0xf7;
break;

case 0xa0:	/* RES 4,B */
tstates+=8;
B &= 0xef;
break;

case 0xa1:	/* RES 4,C */
tstates+=8;
C &= 0xef;
break;

case 0xa2:	/* RES 4,D */
tstates+=8;
D &= 0xef;
break;

case 0xa3:	/* RES 4,E */
tstates+=8;
E &= 0xef;
break;

case 0xa4:	/* RES 4,H */
tstates+=8;
H &= 0xef;
break;

case 0xa5:	/* RES 4,L */
tstates+=8;
L &= 0xef;
break;

case 0xa6:	/* RES 4,(HL) */
tstates+=15;
writebyte(HL, readbyte(HL) & 0xef);
break;

case 0xa7:	/* RES 4,A */
tstates+=8;
A &= 0xef;
break;

case 0xa8:	/* RES 5,B */
tstates+=8;
B &= 0xdf;
break;

case 0xa9:	/* RES 5,C */
tstates+=8;
C &= 0xdf;
break;

case 0xaa:	/* RES 5,D */
tstates+=8;
D &= 0xdf;
break;

case 0xab:	/* RES 5,E */
tstates+=8;
E &= 0xdf;
break;

case 0xac:	/* RES 5,H */
tstates+=8;
H &= 0xdf;
break;

case 0xad:	/* RES 5,L */
tstates+=8;
L &= 0xdf;
break;

case 0xae:	/* RES 5,(HL) */
tstates+=15;
writebyte(HL, readbyte(HL) & 0xdf);
break;

case 0xaf:	/* RES 5,A */
tstates+=8;
A &= 0xdf;
break;

case 0xb0:	/* RES 6,B */
tstates+=8;
B &= 0xbf;
break;

case 0xb1:	/* RES 6,C */
tstates+=8;
C &= 0xbf;
break;

case 0xb2:	/* RES 6,D */
tstates+=8;
D &= 0xbf;
break;

case 0xb3:	/* RES 6,E */
tstates+=8;
E &= 0xbf;
break;

case 0xb4:	/* RES 6,H */
tstates+=8;
H &= 0xbf;
break;

case 0xb5:	/* RES 6,L */
tstates+=8;
L &= 0xbf;
break;

case 0xb6:	/* RES 6,(HL) */
tstates+=15;
writebyte(HL, readbyte(HL) & 0xbf);
break;

case 0xb7:	/* RES 6,A */
tstates+=8;
A &= 0xbf;
break;

case 0xb8:	/* RES 7,B */
tstates+=8;
B &= 0x7f;
break;

case 0xb9:	/* RES 7,C */
tstates+=8;
C &= 0x7f;
break;

case 0xba:	/* RES 7,D */
tstates+=8;
D &= 0x7f;
break;

case 0xbb:	/* RES 7,E */
tstates+=8;
E &= 0x7f;
break;

case 0xbc:	/* RES 7,H */
tstates+=8;
H &= 0x7f;
break;

case 0xbd:	/* RES 7,L */
tstates+=8;
L &= 0x7f;
break;

case 0xbe:	/* RES 7,(HL) */
tstates+=15;
writebyte(HL, readbyte(HL) & 0x7f);
break;

case 0xbf:	/* RES 7,A */
tstates+=8;
A &= 0x7f;
break;

case 0xc0:	/* SET 0,B */
tstates+=8;
B |= 0x01;
break;

case 0xc1:	/* SET 0,C */
tstates+=8;
C |= 0x01;
break;

case 0xc2:	/* SET 0,D */
tstates+=8;
D |= 0x01;
break;

case 0xc3:	/* SET 0,E */
tstates+=8;
E |= 0x01;
break;

case 0xc4:	/* SET 0,H */
tstates+=8;
H |= 0x01;
break;

case 0xc5:	/* SET 0,L */
tstates+=8;
L |= 0x01;
break;

case 0xc6:	/* SET 0,(HL) */
tstates+=15;
writebyte(HL, readbyte(HL) | 0x01);
break;

case 0xc7:	/* SET 0,A */
tstates+=8;
A |= 0x01;
break;

case 0xc8:	/* SET 1,B */
tstates+=8;
B |= 0x02;
break;

case 0xc9:	/* SET 1,C */
tstates+=8;
C |= 0x02;
break;

case 0xca:	/* SET 1,D */
tstates+=8;
D |= 0x02;
break;

case 0xcb:	/* SET 1,E */
tstates+=8;
E |= 0x02;
break;

case 0xcc:	/* SET 1,H */
tstates+=8;
H |= 0x02;
break;

case 0xcd:	/* SET 1,L */
tstates+=8;
L |= 0x02;
break;

case 0xce:	/* SET 1,(HL) */
tstates+=15;
writebyte(HL, readbyte(HL) | 0x02);
break;

case 0xcf:	/* SET 1,A */
tstates+=8;
A |= 0x02;
break;

case 0xd0:	/* SET 2,B */
tstates+=8;
B |= 0x04;
break;

case 0xd1:	/* SET 2,C */
tstates+=8;
C |= 0x04;
break;

case 0xd2:	/* SET 2,D */
tstates+=8;
D |= 0x04;
break;

case 0xd3:	/* SET 2,E */
tstates+=8;
E |= 0x04;
break;

case 0xd4:	/* SET 2,H */
tstates+=8;
H |= 0x04;
break;

case 0xd5:	/* SET 2,L */
tstates+=8;
L |= 0x04;
break;

case 0xd6:	/* SET 2,(HL) */
tstates+=15;
writebyte(HL, readbyte(HL) | 0x04);
break;

case 0xd7:	/* SET 2,A */
tstates+=8;
A |= 0x04;
break;

case 0xd8:	/* SET 3,B */
tstates+=8;
B |= 0x08;
break;

case 0xd9:	/* SET 3,C */
tstates+=8;
C |= 0x08;
break;

case 0xda:	/* SET 3,D */
tstates+=8;
D |= 0x08;
break;

case 0xdb:	/* SET 3,E */
tstates+=8;
E |= 0x08;
break;

case 0xdc:	/* SET 3,H */
tstates+=8;
H |= 0x08;
break;

case 0xdd:	/* SET 3,L */
tstates+=8;
L |= 0x08;
break;

case 0xde:	/* SET 3,(HL) */
tstates+=15;
writebyte(HL, readbyte(HL) | 0x08);
break;

case 0xdf:	/* SET 3,A */
tstates+=8;
A |= 0x08;
break;

case 0xe0:	/* SET 4,B */
tstates+=8;
B |= 0x10;
break;

case 0xe1:	/* SET 4,C */
tstates+=8;
C |= 0x10;
break;

case 0xe2:	/* SET 4,D */
tstates+=8;
D |= 0x10;
break;

case 0xe3:	/* SET 4,E */
tstates+=8;
E |= 0x10;
break;

case 0xe4:	/* SET 4,H */
tstates+=8;
H |= 0x10;
break;

case 0xe5:	/* SET 4,L */
tstates+=8;
L |= 0x10;
break;

case 0xe6:	/* SET 4,(HL) */
tstates+=15;
writebyte(HL, readbyte(HL) | 0x10);
break;

case 0xe7:	/* SET 4,A */
tstates+=8;
A |= 0x10;
break;

case 0xe8:	/* SET 5,B */
tstates+=8;
B |= 0x20;
break;

case 0xe9:	/* SET 5,C */
tstates+=8;
C |= 0x20;
break;

case 0xea:	/* SET 5,D */
tstates+=8;
D |= 0x20;
break;

case 0xeb:	/* SET 5,E */
tstates+=8;
E |= 0x20;
break;

case 0xec:	/* SET 5,H */
tstates+=8;
H |= 0x20;
break;

case 0xed:	/* SET 5,L */
tstates+=8;
L |= 0x20;
break;

case 0xee:	/* SET 5,(HL) */
tstates+=15;
writebyte(HL, readbyte(HL) | 0x20);
break;

case 0xef:	/* SET 5,A */
tstates+=8;
A |= 0x20;
break;

case 0xf0:	/* SET 6,B */
tstates+=8;
B |= 0x40;
break;

case 0xf1:	/* SET 6,C */
tstates+=8;
C |= 0x40;
break;

case 0xf2:	/* SET 6,D */
tstates+=8;
D |= 0x40;
break;

case 0xf3:	/* SET 6,E */
tstates+=8;
E |= 0x40;
break;

case 0xf4:	/* SET 6,H */
tstates+=8;
H |= 0x40;
break;

case 0xf5:	/* SET 6,L */
tstates+=8;
L |= 0x40;
break;

case 0xf6:	/* SET 6,(HL) */
tstates+=15;
writebyte(HL, readbyte(HL) | 0x40);
break;

case 0xf7:	/* SET 6,A */
tstates+=8;
A |= 0x40;
break;

case 0xf8:	/* SET 7,B */
tstates+=8;
B |= 0x80;
break;

case 0xf9:	/* SET 7,C */
tstates+=8;
C |= 0x80;
break;

case 0xfa:	/* SET 7,D */
tstates+=8;
D |= 0x80;
break;

case 0xfb:	/* SET 7,E */
tstates+=8;
E |= 0x80;
break;

case 0xfc:	/* SET 7,H */
tstates+=8;
H |= 0x80;
break;

case 0xfd:	/* SET 7,L */
tstates+=8;
L |= 0x80;
break;

case 0xfe:	/* SET 7,(HL) */
tstates+=15;
writebyte(HL, readbyte(HL) | 0x80);
break;

case 0xff:	/* SET 7,A */
tstates+=8;
A |= 0x80;
break;
