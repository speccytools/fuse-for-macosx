/* gtkdisplay.h: GTK+ routines for dealing with the Speccy screen
   Copyright (c) 2000 Philip Kendall

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

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_GTKDISPLAY_H
#define FUSE_GTKDISPLAY_H

int gtkdisplay_init(int width, int height);

int gtkdisplay_configure_notify(int width, int height);
void gtkdisplay_putpixel(int x,int y,int colour);
void gtkdisplay_line(int y);
void gtkdisplay_area(int x, int y, int width, int height);
void gtkdisplay_set_border(int line, int pixel_from, int pixel_to, int colour);

int gtkdisplay_end(void);

#endif			/* #ifndef FUSE_GTKDISPLAY_H */
