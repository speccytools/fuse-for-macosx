/* display.h: Routines for printing the Spectrum's screen
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

   E-mail: pak21-fuse.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_DISPLAY_H
#define FUSE_DISPLAY_H

#ifndef FUSE_TYPES_H
#include "types.h"
#endif			/* #ifndef FUSE_TYPES_H */

/* The width and height of the Speccy's screen */
#define DISPLAY_WIDTH  256
#define DISPLAY_HEIGHT 192

/* The width and height of the (emulated) border */
#define DISPLAY_BORDER_WIDTH  32
#define DISPLAY_BORDER_HEIGHT 24

/* The width and height of the window we'll be displaying */
#define DISPLAY_SCREEN_WIDTH  ( DISPLAY_WIDTH  + 2 * DISPLAY_BORDER_WIDTH  )
#define DISPLAY_SCREEN_HEIGHT ( DISPLAY_HEIGHT + 2 * DISPLAY_BORDER_HEIGHT )

extern BYTE display_border;

int display_init(int *argc, char ***argv);
void display_line(void);
void display_dirty(WORD address, BYTE data);
void display_set_border(int colour);
int display_frame(void);
void display_refresh_all(void);

#endif			/* #ifndef FUSE_DISPLAY_H */
