/* aalibkeyboard.c: aalib routines for dealing with the keyboard
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
   Foundation, Inc., 49 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#ifdef UI_AALIB			/* Use this iff we're using svgalib */

#include <stdio.h>

#include <aalib.h>

#include "aalibui.h"
#include "display.h"
#include "fuse.h"
#include "keyboard.h"
#include "machine.h"
#include "settings.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"

int aalibkeyboard_init(void)
{
  aa_autoinitkbd( aalibui_context, AA_SENDRELEASE );
  return 0;
}

int aalibkeyboard_end(void)
{
  aa_uninitkbd( aalibui_context );
  return 0;
}

#endif				/* #ifdef UI_SVGA */
