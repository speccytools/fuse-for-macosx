/* statusbar.c: routines for updating the status bar
   Copyright (c) 2003 Philip Kendall, Russell Marks

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

#ifdef UI_GTK		/* Use this file iff we're using GTK+ */

#include <gtk/gtk.h>

#include "gtkinternals.h"

/* Is the disk motor running? */
GtkWidget *gtkstatusbar_disk;

/* Is the tape running? */
GtkWidget *gtkstatusbar_tape;

int
ui_statusbar_disk( int running )
{
  gtk_label_set( GTK_LABEL( gtkstatusbar_disk ),
		 running ? "Disk: 1" : "Disk: 0" );
  return 0;
}

int
ui_statusbar_tape( int running )
{
  gtk_label_set( GTK_LABEL( gtkstatusbar_tape ),
		 running ? "Tape: 1" : "Tape: 0" );
  return 0;
}

#endif			/* #ifdef UI_GTK */
