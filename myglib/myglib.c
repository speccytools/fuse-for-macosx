/* myglib.c: Replacements for glib routines
   Copyright (c) 2001 Matan Ziv-Av, Philip Kendall

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

#ifndef HAVE_LIB_GLIB		/* Use this iff we're not using the 
				   `proper' glib */

#include <stdlib.h>
#include "myglib.h"

static
gint	last_function		(gconstpointer	 a,
				 gconstpointer	 b);

GSList * free_list = NULL;

GSList* g_slist_insert_sorted	(GSList		*list,
				 gpointer	 data,
				 GCompareFunc	 func) {
    
  GSList *tmp_list = list;
  GSList *prev_list = NULL;
  GSList *new_list;
  gint cmp;

    if(!free_list) {
        int i;
        free_list=(GSList *)malloc(1024*sizeof(GSList));
        for(i=0;i<127;i++)
            free_list[i].next=&free_list[i+1];
        free_list[127].next=NULL;
    }


  if(!func) return list;

  if (!list)
    {
      new_list = free_list;
      free_list=free_list->next;
      new_list->data = data;
      new_list->next=NULL;
      return new_list;
    }

  cmp = (*func) (data, tmp_list->data);

  while ((tmp_list->next) && (cmp > 0))
    {
      prev_list = tmp_list;
      tmp_list = tmp_list->next;
      cmp = (*func) (data, tmp_list->data);
    }

  new_list = free_list;
  free_list=free_list->next;
  new_list->data = data;

  if ((!tmp_list->next) && (cmp > 0))
    {
      tmp_list->next = new_list;
      new_list->next=NULL;
      return list;
    }

  if (prev_list)
    {
      prev_list->next = new_list;
      new_list->next = tmp_list;
      return list;
    }
  else
    {
      new_list->next = list;
      return new_list;
    }
}

GSList* g_slist_append		(GSList		*list,
				 gpointer	 data) {

  return g_slist_insert_sorted(list, data, last_function);
}

static
gint	last_function		(gconstpointer	 a,
				 gconstpointer	 b) {

  return 1;
}

GSList* g_slist_remove		(GSList		*list,
				 gpointer	 data) {

  GSList *tmp;
  GSList *prev;

  prev = NULL;
  tmp = list;

  while (tmp)
    {
      if (tmp->data == data)
        {
          if (prev)
            prev->next = tmp->next;
          if (list == tmp)
            list = list->next;

          tmp->next = NULL;
          g_slist_free (tmp);

          break;
        }

      prev = tmp;
      tmp = tmp->next;
    }

  return list;
}

void	g_slist_foreach		(GSList		*list,
				 GFunc		 func,
				 gpointer	 user_data) {

  while (list)
    {
      (*func) (list->data, user_data);
      list = list->next;
    }
}

void	g_slist_free		(GSList		*list) {
  if (list)
    {
      list->data = list->next;
      list->next = free_list;
      free_list = list;
    }
}

GSList* g_slist_nth		(GSList		*list,
				 guint		n) {
  for( ; n; n-- ) {
    if( list == NULL ) return NULL;
    list = list->next;
  }

  return list;
}

gint	g_slist_position	(GSList		*list,
				 GSList		*llink) {
  int n;

  n = 0;

  while( list ) {
    if( list == llink ) return n;
    list = list->next;
    n++;
  }

  return -1;
}

#endif				/* #ifndef HAVE_LIB_GLIB */
