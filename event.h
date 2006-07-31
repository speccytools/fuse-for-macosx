/* event.h: Routines needed for dealing with the event list
   Copyright (c) 2000-2006 Philip Kendall

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

#ifndef FUSE_EVENT_H
#define FUSE_EVENT_H

#ifdef HAVE_LIB_GLIB
#include <glib.h>
#endif				/* #ifdef HAVE_LIB_GLIB */

#include <libspectrum.h>

/* Information about an event */
typedef struct event_t {
  libspectrum_dword tstates;
  int type;
} event_t;

/* The various types of event which can occur */
typedef enum event_type {

  EVENT_TYPE_BREAKPOINT,
  EVENT_TYPE_EDGE,
  EVENT_TYPE_FRAME,
  EVENT_TYPE_INTERRUPT,
  EVENT_TYPE_NMI,
  EVENT_TYPE_NULL,
  EVENT_TYPE_TRDOS_CMD_DONE,
  EVENT_TYPE_TRDOS_INDEX,
  EVENT_TYPE_TIMER,
  EVENT_TYPE_DISPLAY_WRITE,

} event_type;

/* A large value to mean `no events due' */
extern const libspectrum_dword event_no_events;

/* When will the next event happen? */
extern libspectrum_dword event_next_event;

/* Set up the event list */
int event_init(void);

/* Add an event at the correct place in the event list */
int event_add( libspectrum_dword event_time, event_type type );

/* Do all events which have passed */
int event_do_events(void);

/* Called at end of frame to reduce T-state count of all entries */
int event_frame( libspectrum_dword tstates_per_frame );

/* Remove all events of a specific type from the stack */
int event_remove_type( event_type type );

/* Clear the event stack */
int event_reset(void);

/* Call a user-supplied function for every event in the current list */
int event_foreach( void (*function)( gpointer data, gpointer user_data ),
		   gpointer user_data );

/* A textual representation of each event type */
const char *event_name( event_type type );

/* Called on exit to clean up */
int event_end(void);

#endif				/* #ifndef FUSE_EVENT_H */
