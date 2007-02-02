/* trdos.h: Routines for handling the Betadisk interface
   Copyright (c) 2003-2004 Fredrick Meunier, Philip Kendall

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

   E-mail: philip-fuse@shadowmagic.org.uk

     Fred: fredm@spamcop.net

*/

#ifndef FUSE_TRDOS_H
#define FUSE_TRDOS_H

#include <libspectrum.h>

extern int trdos_available;	/* Is TRDOS available for use? */
extern int trdos_active;     /* TRDOS enabled? */

int trdos_init( void );

void trdos_reset( void );

void trdos_end( void );

void trdos_page( void );
void trdos_unpage( void );
void trdos_memory_map( void );

int trdos_from_snapshot( libspectrum_snap *snap, int capabilities );
int trdos_to_snapshot( libspectrum_snap *snap );

void trdos_cr_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte trdos_sr_read( libspectrum_word port, int *attached );

libspectrum_byte trdos_tr_read( libspectrum_word port, int *attached );
void trdos_tr_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte trdos_sec_read( libspectrum_word port, int *attached );
void trdos_sec_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte trdos_dr_read( libspectrum_word port, int *attached );
void trdos_dr_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte trdos_sp_read( libspectrum_word port, int *attached );
void trdos_sp_write( libspectrum_word port, libspectrum_byte b );

typedef enum trdos_drive_number {
  TRDOS_DRIVE_A = 0,
  TRDOS_DRIVE_B,
  TRDOS_DRIVE_C,
  TRDOS_DRIVE_D,
} trdos_drive_number;

int trdos_disk_insert( trdos_drive_number which, const char *filename,
                       int autoload );
int trdos_disk_insert_default_autoload( trdos_drive_number which,
                                        const char *filename );
int trdos_disk_eject( trdos_drive_number which, int write );
int trdos_disk_write( trdos_drive_number which, const char *filename );
int trdos_event_cmd_done( libspectrum_dword last_tstates );
int trdos_event_index( libspectrum_dword last_tstates );

#endif                  /* #ifndef FUSE_TRDOS_H */
