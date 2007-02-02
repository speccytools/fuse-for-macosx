/* aalibui.c: Routines for dealing with the aalib user interface
   Copyright (c) 2001 Philip Kendall

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#ifdef UI_AALIB			/* Use this iff we're using svgalib */

#include <stdio.h>

#include <aalib.h>

#include "aalibkeyboard.h"
#include "fuse.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

aa_context *aalibui_context;

int ui_init(int *argc, char ***argv, int width, int height)
{
  int error;

  error = uidisplay_init(width, height);
  if(error) return error;

  error = aalibkeyboard_init();
  if(error) return error;

  return 0;
}

int ui_event()
{
  aa_fastrender( aalibui_context,
		 0, aa_scrwidth( aalibui_context ),
		 0, aa_scrheight( aalibui_context ) );
  aa_flush( aalibui_context );

  fprintf( stderr, "Displayed flushed\n" );

  /* FIXME: Handle keyboard */

  return 0;
}

int ui_end(void)
{
  int error;

  error = aalibkeyboard_end();
  if(error) return error;

  error = uidisplay_end();
  if(error) return error;

  return 0;
}

#endif				/* #ifdef UI_AALIB */
