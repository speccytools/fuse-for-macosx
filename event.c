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

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_LIB_GLIB		/* If we're using glib */
#include <glib.h>
#else				/* #ifdef HAVE_LIB_GLIB */
#include "myglib/myglib.h"	/* if not, use the local replacement */
#endif

#include "display.h"
#include "event.h"
#include "fuse.h"
#include "rzx.h"
#include "tape.h"
#include "ui/ui.h"
#include "spectrum.h"

/* A large value to mean `no events due' */
const DWORD event_no_events = 0xffffffff;

/* The actual list of events */
static GSList* event_list;

/* Comparision function so events stay in t-state order */
gint event_add_cmp(gconstpointer a, gconstpointer b);

/* User function for event_interrupt(...) */
void event_reduce_tstates(gpointer data,gpointer user_data);

/* Free the memory used by a specific entry */
void event_free_entry(gpointer data, gpointer user_data);

/* Set up the event list */
int event_init(void)
{
  event_list=NULL;
  event_next_event=event_no_events;
  return 0;
}

/* Add an event at the correct place in the event list */
int event_add(DWORD event_time, int type)
{
  event_t *ptr;

  ptr=(event_t*)malloc(sizeof(event_t));
  if(!ptr) return 1;

  ptr->tstates= event_time;
  ptr->type=type;

  event_list=g_slist_insert_sorted(event_list,(gpointer)ptr,event_add_cmp);

  if(tstates<event_next_event) event_next_event = event_time;

  return 0;
}

/* Comparision function so events stay in t-state order */
gint event_add_cmp(gconstpointer a, gconstpointer b)
{
  const event_t *ptr1=(const event_t *)a;
  const event_t *ptr2=(const event_t *)b;

  if(ptr1->tstates < ptr2->tstates) {
    return -1;
  } else if(ptr1->tstates > ptr2->tstates) {
    return 1;
  } else {
    return 0;
  }
}

/* Do all events which have passed */
int event_do_events(void)
{
  event_t *ptr;

  while(event_next_event <= tstates) {
    ptr= ( (event_t*) (event_list->data) );

    /* Remove the event from the list *before* processing */
    event_list=g_slist_remove(event_list,ptr);

    switch(ptr->type) {
    case EVENT_TYPE_INTERRUPT:
      spectrum_interrupt();
      ui_event();
      rzx_frame();
      break;
    case EVENT_TYPE_LINE:
      display_line();
      break;
    case EVENT_TYPE_EDGE:
      tape_next_edge();
      break;
    default:
      fprintf( stderr, "%s: unknown event type %d\n", fuse_progname,
	       ptr->type );
      break;
    }
    free(ptr);
    if(event_list == NULL) {
      event_next_event = event_no_events;
    } else {
      event_next_event= ( (event_t*) (event_list->data) ) -> tstates;
    }
  }

  return 0;
}

/* Called on interrupt to reduce T-state count of all entries */
int event_interrupt( DWORD tstates_per_frame )
{
  g_slist_foreach(event_list, event_reduce_tstates, &tstates_per_frame );
  return 0;
}

/* User function for event_interrupt(...) */
void event_reduce_tstates(gpointer data,gpointer user_data)
{
  event_t *ptr=(event_t*)data;
  DWORD *tstates_per_frame = (DWORD*)user_data;

  ptr->tstates -= (*tstates_per_frame) ;
}

/* Clear the event stack */
int event_reset(void)
{
  g_slist_foreach(event_list,event_free_entry,NULL);
  g_slist_free(event_list);
  event_list=NULL;
  event_next_event=event_no_events;
  return 0;
}

/* Free the memory used by a specific entry */
void event_free_entry(gpointer data, gpointer user_data)
{
  event_t *ptr=(event_t*)data;
  free(ptr);
}

/* Tidy-up function called at end of emulation */
int event_end(void)
{
  return event_reset();
}
