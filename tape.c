/* tape.c: tape handling routines
   Copyright (c) 1999-2001 Philip Kendall

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

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#include "spectrum.h"
#include "tape.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

FILE *tape_file=0;

int tape_trap(void)
{
  BYTE parity,*buffer,*ptr; WORD i; int load;

  buffer=(BYTE*)malloc(0x10000*sizeof(BYTE));
  if(!buffer) return 2;
  ptr=buffer;

  if(!tape_file) if(tape_open()) { free(buffer); return 1; }

  load= ( F_ & FLAG_C );

  /* All returns made via the RET at the #05E2 */
  PC=0x5e2;

  i=fgetc(tape_file)+0x100*fgetc(tape_file);
  
  /* If the block's too short, give up and go home (with carry reset
     to indicate error */
  if(i<DE) { 
    fseek(tape_file,i,SEEK_CUR);
    F = ( F & ~FLAG_C );
    free(buffer);
    return 0;
  }
    
  fread(buffer,i,1,tape_file);
  if(feof(tape_file) || ferror(tape_file) ) {
    fclose(tape_file); tape_file=0;
    F = ( F & ~FLAG_C );
    free(buffer);
    return 3;
  }

  parity=*ptr;

  /* If the flag byte does not match, reset carry and return */
  if( *ptr++ != A_ ) { F = ( F & ~FLAG_C ); free(buffer); return 0; }

  if(load) {
    for(i=0;i<DE;i++) { writebyte(IX+i,*ptr); parity^=*ptr++; }
  } else {		/* verifying */
    for(i=0;i<DE;i++) {
      parity^=*ptr;
      if(*ptr++!=readbyte(IX+i)) {
	F = ( F & ~FLAG_C );
	free(buffer);
	return 0;
      }
    }
  }

  /* If the parity byte does not match, reset carry and return */
  if(*ptr++!=parity) {
    F = ( F & ~FLAG_C );
    free(buffer);
    return 0;
  }

  /* Else return with carry set */
  F |= FLAG_C;

  free(buffer);

  return 0;

}

int tape_open(void)
{
  if(tape_file) fclose(tape_file);
  tape_file=fopen("tape.tap","rb");
  return !tape_file;
}
