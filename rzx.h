/* rzx.h: .rzx files
   Copyright (c) 2002 Philip Kendall

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

#ifndef FUSE_RZX_H
#define FUSE_RZX_H

#ifndef LIBSPECTRUM_RZX_H
#include "libspectrum/rzx.h"
#endif			/* #ifndef LIBSPECTRUM_RZX_H */

/* The offset used to get the count of instructions from the R register */
extern size_t rzx_instructions_offset;

/* Are we currently recording a .rzx file? */
extern int rzx_recording;

/* Are we currently playing back a .rzx file? */
extern int rzx_playback;

/* The .rzx frame we're currently playing */
extern size_t rzx_current_frame;

/* The actual RZX data */
extern libspectrum_rzx rzx;

int rzx_init( void );

int rzx_start_recording( const char *filename );
int rzx_stop_recording( void );

int rzx_start_playback( const char *filename );
int rzx_stop_playback( void );

int rzx_frame( void );

int rzx_end( void );

#endif			/* #ifndef FUSE_RZX_H */
