/* event.h: Routines needed for dealing with the event list
   Copyright (c) 2000-2003 Philip Kendall

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

#include <libspectrum.h>

/* Information about an event */
typedef struct event_t {
  libspectrum_dword tstates;
  int type;
} event_t;

/* The various types of event which can occur */
enum event_types {

  /* This must come before EVENT_TYPE_FRAME EVENT_TYPE_INTERRUPT to
     ensure that interrupts are re-enabled before we try and process
     one if both events occur at the same tstate */
  EVENT_TYPE_ENABLE_INTERRUPTS,

  EVENT_TYPE_EDGE,
  EVENT_TYPE_FRAME,
  EVENT_TYPE_LINE,
  EVENT_TYPE_NMI,
  EVENT_TYPE_NULL,
  EVENT_TYPE_TRDOS_CMD_DONE,
  EVENT_TYPE_TRDOS_INDEX,

};

/* A large value to mean `no events due' */
extern const libspectrum_dword event_no_events;

/* When will the next event happen? */
extern libspectrum_dword event_next_event;

/* Set up the event list */
int event_init(void);

/* Add an event at the correct place in the event list */
int event_add( libspectrum_dword event_time, int type );

/* Do all events which have passed */
int event_do_events(void);

/* Called at end of frame to reduce T-state count of all entries */
int event_frame( libspectrum_dword tstates_per_frame );

/* Remove all events of a specific type from the stack */
int event_remove_type( int type );

/* Clear the event stack */
int event_reset(void);

/* Called on exit to clean up */
int event_end(void);

#endif				/* #ifndef FUSE_EVENT_H */
