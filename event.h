/* event.c: Routines needed for dealing with the event list
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

   E-mail: pak21-fuse.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_EVENT_H
#define FUSE_EVENT_H

#ifndef FUSE_TYPES_H
#include "types.h"
#endif			/* #ifndef FUSE_TYPES_H */

/* Information about an event */
typedef struct event_t {
  DWORD tstates;
  int type;
} event_t;

/* The various types of event which can occur */
enum event_types { EVENT_TYPE_INTERRUPT, EVENT_TYPE_LINE };

/* A large value to mean `no events due' */
extern const DWORD event_no_events;

/* When will the next event happen? */
DWORD event_next_event;

/* Set up the event list */
int event_init(void);

/* Add an event at the correct place in the event list */
int event_add(DWORD tstates, int type);

/* Do all events which have passed */
int event_do_events(void);

/* Called on interrupt to reduce T-state count of all entries */
int event_interrupt(DWORD tstates);

/* Clear the event stack */
int event_reset(void);

/* Called on exit to clean up */
int event_end(void);

#endif				/* #ifndef FUSE_EVENT_H */
