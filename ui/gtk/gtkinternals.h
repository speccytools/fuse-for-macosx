/* gtkinternals.h: stuff internal to the GTK+ UI
   Copyright (c) 2003-2004 Philip Kendall

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

#ifndef FUSE_GTKINTERNALS_H
#define FUSE_GTKINTERNALS_H

#include <gtk/gtk.h>
#include <libspectrum.h>

/*
 * Display routines (gtkdisplay.c)
 */

/* The colour palette in use */
extern libspectrum_dword gtkdisplay_colours[ 16 ];

int gtkdisplay_init( void );
int gtkdisplay_end( void );

/*
 * Keyboard routines (gtkkeyboard.c)
 */

int gtkkeyboard_keypress( GtkWidget *widget, GdkEvent *event,
			  gpointer data);
int gtkkeyboard_keyrelease( GtkWidget *widget, GdkEvent *event,
			    gpointer data);
int gtkkeyboard_release_all( GtkWidget *widget, GdkEvent *event,
			     gpointer data );

/*
 * Statusbar routines (statusbar.c)
 */

int gtkstatusbar_create( GtkBox *parent );
int gtkstatusbar_set_visibility( int visible );

/*
 * The icon pixmaps (pixmaps.c)
 */
extern char *gtkpixmap_tape_inactive[];
extern char *gtkpixmap_tape_active[];
extern char *gtkpixmap_disk_inactive[];
extern char *gtkpixmap_disk_active[];
extern char *gtkpixmap_pause_inactive[];
extern char *gtkpixmap_pause_active[];
extern char *gtkpixmap_tape_marker[];

/*
 * Poke finder routines (pokefinder.c)
 */

void gtkui_pokefinder( GtkWidget *widget, gpointer data );

/*
 * Memory browser routines (memory.c)
 */

void gtkui_memory_browser( void );

/*
 * General user interface routines (gtkui.c)
 */

extern GtkWidget *gtkui_window;
extern GtkWidget *gtkui_drawing_area;

void gtkui_destroy_widget_and_quit( GtkWidget *widget, gpointer data );
char* gtkui_fileselector_get_filename( const char *title );

void gtkui_load_binary_data( GtkWidget *widget, gpointer data );
void gtkui_save_binary_data( GtkWidget *widget, gpointer data );
int gtkui_confirm( const char *string );
void gtk_tape_browse( GtkWidget *widget, gpointer data );
void gtkui_roms( gpointer callback_data, guint callback_action,
		 GtkWidget *widget );

int gtkui_picture( const char *filename, int border );

extern void gtkui_popup_menu(void);

#ifdef UI_GTK2
typedef PangoFontDescription *gtkui_font;
#else				/* #ifdef UI_GTK2 */
typedef GtkStyle *gtkui_font;
#endif				/* #ifdef UI_GTK2 */

int gtkui_get_monospaced_font( gtkui_font *font );
void gtkui_free_font( gtkui_font font );
void gtkui_set_font( GtkWidget *widget, gtkui_font font );

#endif				/* #ifndef FUSE_GTKINTERNALS_H */
