/* scld.h: Routines for handling the Timex SCLD
   Copyright (c) 2002 Fredrick Meunier

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

   E-mail: fredm@spamcop.net

*/

#ifndef FUSE_SCLD_H
#define FUSE_SCLD_H

#ifndef FUSE_TYPES_H
#include "types.h"
#endif                  /* #ifndef FUSE_TYPES_H */

#define ALTDFILE        0x01
#define EXTCOLOUR       0x02
#define HIRES           0x04
#define HIRESCOLMASK    0x38
#define INTDISABLE      0x40
#define ALTMEMBANK      0x80

#define WHITEBLACK      0x00
#define YELLOWBLUE      0x08
#define CYANRED         0x10
#define GREENMAGENTA    0x18
#define MAGENTAGREEN    0x20
#define REDCYAN         0x28
#define BLUEYELLOW      0x30
#define BLACKWHITE      0x38

#define ALTDFILE_OFFSET 0x2000

extern BYTE scld_altdfile;
extern BYTE scld_extcolour;
extern BYTE scld_hires;
extern BYTE scld_intdisable;
extern BYTE scld_altmembank;	     /* 0 = cartridge, 1 = exrom */

extern BYTE scld_screenmode;

extern BYTE scld_last_dec;           /* The last byte sent to Timex DEC port */

extern BYTE scld_last_hsr;           /* The last byte sent to Timex HSR port */

void scld_reset(void);
void scld_dec_write(WORD port, BYTE b);
BYTE scld_dec_read(WORD port);

void scld_hsr_write (WORD port, BYTE b);
BYTE scld_hsr_read (WORD port);

BYTE hires_get_attr(void);

#endif                  /* #ifndef FUSE_SCLD_H */
