/* myglib.h: Replacements for glib routines
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

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_MYGLIB_H
#define FUSE_MYGLIB_H

typedef int gint;
typedef const void * gconstpointer;
typedef void * gpointer;

typedef struct _GSList GSList;

struct _GSList {
    gpointer data;
    GSList *next;
};

typedef void		(*GFunc)		(gpointer	data,
						 gpointer	user_data);

typedef gint		(*GCompareFunc)		(gconstpointer	a,
						 gconstpointer	b);


GSList* g_slist_insert_sorted	(GSList		*list,
				 gpointer	 data,
				 GCompareFunc	 func);

GSList* g_slist_remove		(GSList		*list,
				 gpointer	 data);

void	g_slist_foreach		(GSList		*list,
				 GFunc		 func,
				 gpointer	 user_data);

void	g_slist_free		(GSList		*list);

#endif				/* #ifndef FUSE_MYGLIB_H */
