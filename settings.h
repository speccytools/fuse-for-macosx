/* settings.h: Handling configuration settings
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

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#ifndef FUSE_SETTINGS_H
#define FUSE_SETTINGS_H

typedef struct settings_info {

  int issue2;		/* Issue 2 keyboard emulation? */
  int tape_traps;	/* Use tape loading traps? */

} settings_info;

extern settings_info settings_current;

int settings_init( void );
int settings_defaults( settings_info *settings );

#endif				/* #ifndef FUSE_SETTINGS_H */
