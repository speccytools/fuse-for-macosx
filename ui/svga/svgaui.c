/* svgaui.c: Routines for dealing with the svgalib user interface
   Copyright (c) 2000-2001 Philip Kendall, Matan Ziv-Av

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

#include <config.h>

#ifdef UI_SVGA			/* Use this iff we're using svgalib */

#include <stdio.h>

#include <vgakeyboard.h>

#include "fuse.h"
#include "svgakeyboard.h"
#include "ui.h"
#include "uidisplay.h"

int ui_init(int *argc, char ***argv, int width, int height)
{
  int error;

  error = uidisplay_init(width, height);
  if(error) return error;

  error = svgakeyboard_init();
  if(error) return error;

  return 0;
}

int ui_event()
{
  keyboard_update();
  return 0;
}

int ui_end(void)
{
  int error;

  error = svgakeyboard_end();
  if(error) return error;

  error = uidisplay_end();
  if(error) return error;

  return 0;
}

#endif				/* #ifdef UI_SVGA */
