/* snapshot.c: snapshot handling routines
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

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "display.h"
#include "spec128.h"
#include "spectrum.h"
#include "z80.h"
#include "z80_macros.h"

int snapshot_read(void)
{
  FILE *f; BYTE buffer[27],buffer2[0x4000]; struct stat file_info;

  if(stat("snapshot.sna",&file_info)) return 1;

  switch(file_info.st_size) {
    case 49179:
      machine.machine=SPECTRUM_MACHINE_48;
      break;
    case 131103:
    case 147487:
      machine.machine=SPECTRUM_MACHINE_128;
      break;
    default: return 3;
  }
  spectrum_init(); machine.reset();

  f=fopen("snapshot.sna","rb");
  if(!f) return 2;

  fread(buffer,27,1,f);

  z80.halted=0;
  I   =buffer[ 0];
  L_  =buffer[ 1];      H_ =buffer[ 2];
  E_  =buffer[ 3];      D_ =buffer[ 4];
  C_  =buffer[ 5];      B_ =buffer[ 6];
  F_  =buffer[ 7];      A_ =buffer[ 8];
  L   =buffer[ 9];      H  =buffer[10];
  E   =buffer[11];      D  =buffer[12];
  C   =buffer[13];      B  =buffer[14];
  IYL =buffer[15];      IYH=buffer[16];
  IXL =buffer[17];      IXH=buffer[18];
  IFF1=IFF2=(buffer[19]&0x04)>>2;
  R  =buffer[20];
  F  =buffer[21];        A  =buffer[22];
  SPL=buffer[23];        SPH=buffer[24];
  IM =buffer[25];

  display_set_border(buffer[26]);

  fread(RAM[5],0x4000,1,f);
  fread(RAM[2],0x4000,1,f);
  if(machine.machine==SPECTRUM_MACHINE_48) {
    fread(RAM[0],0x4000,1,f);
    PCL=readbyte(SP++); PCH=readbyte(SP++);
  } else {
    int i,page;
    fread(buffer2,0x4000,1,f);
    fread(buffer,4,1,f); PCL=buffer[0]; PCH=buffer[1];

    /* Write to the 128K's memory; the port number here is ignored */
    spec128_memoryport_write( 0x7ffd, buffer[2]);

    page=buffer[2]&0x07;
    memcpy(RAM[page],buffer2,0x4000);
    for(i=0;i<8;i++) {
      if( i==2 || i==5 || i==page ) continue;
      fread(RAM[i],0x4000,1,f);
    }
  }    

  fclose(f);

  return 0;

}

int snapshot_write(void)
{
  FILE *f; BYTE buffer[0xc000]; int i;
  WORD stackpointer;

  f=fopen("snapshot.sna","wb");
  if(!f) return 1;

  if(machine.machine==SPECTRUM_MACHINE_48) {
    stackpointer = SP-2;
  } else {
    stackpointer = SP;
  }

  fputc(I  ,f);
  fputc(L_ ,f); fputc(H_ ,f);
  fputc(E_ ,f); fputc(D_ ,f);
  fputc(C_ ,f); fputc(B_ ,f);
  fputc(F_ ,f); fputc(A_ ,f);
  fputc(L  ,f); fputc(H  ,f);
  fputc(E  ,f); fputc(D  ,f);
  fputc(C  ,f); fputc(B  ,f);
  fputc(IYL,f); fputc(IYH,f);
  fputc(IXL,f); fputc(IXH,f);
  fputc( IFF1 << 2 ,f);
  fputc(R  ,f);
  fputc(F  ,f); fputc(A  ,f);
  fputc( (stackpointer & 0xff ),f); fputc( (stackpointer >> 8 ),f);
  fputc(IM ,f);
  fputc(display_border,f);

  memcpy(&buffer[     0],RAM[5],0x4000);
  memcpy(&buffer[0x4000],RAM[2],0x4000);

  if(machine.machine==SPECTRUM_MACHINE_48) {
    memcpy(&buffer[0x8000],RAM[0],0x4000);
    buffer[((stackpointer+1)-0x4000)]=PCH;
    buffer[((stackpointer  )-0x4000)]=PCL;
    fwrite(buffer,0xc000,1,f);
  } else {
    memcpy(&buffer[0x8000],RAM[machine.ram.current_page],0x4000);
    fwrite(buffer,0xc000,1,f);
    fputc(PCL,f); fputc(PCH,f);
    fputc(machine.ram.last_byte,f); fputc(0,f);
    for(i=0;i<8;i++) {
      if( i==2 || i==5 || i==machine.ram.current_page ) continue;
      fwrite(RAM[i],0x4000,1,f);
    }
  }

  fclose(f);

  return 0;
}
