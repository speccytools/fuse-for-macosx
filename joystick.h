/* joystick.h: Joystick emulation support
   Copyright (c) 2001-2004 Russell Marks, Philip Kendall
   Copyright (c) 2003 Darren Salt

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

#ifndef FUSE_JOYSTICK_H
#define FUSE_JOYSTICK_H

#include <libspectrum.h>

/* Number of joysticks known about & initialised */
extern int joysticks_supported;

/* Init/shutdown functions. Errors aren't important here */
void fuse_joystick_init( void );
void fuse_joystick_end( void );

/* Default read function (returns data in Kempston format) */
libspectrum_byte joystick_default_read( libspectrum_word port,
					libspectrum_byte which );

/* Interface-specific read functions */
libspectrum_byte joystick_kempston_read ( libspectrum_word port,
					  int *attached );
libspectrum_byte joystick_timex_read ( libspectrum_word port,
				       libspectrum_byte which );

#endif			/* #ifndef FUSE_JOYSTICK_H */
