/* trdos.h: Routines for handling the Betadisk interface
   Copyright (c) 2003 Fredrick Meunier

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

   E-mail: fredm@spamcop.net

*/

#ifndef FUSE_TRDOS_H
#define FUSE_TRDOS_H

extern int trdos_active;     /* TRDOS enabled? */

void trdos_reset( void );

void trdos_end( void );

void trdos_cr_write( WORD port, BYTE b );

BYTE trdos_sr_read( WORD port );

BYTE trdos_tr_read( WORD port );
void trdos_tr_write( WORD port, BYTE b );

BYTE trdos_sec_read( WORD port );
void trdos_sec_write( WORD port, BYTE b );

BYTE trdos_dr_read( WORD port );
void trdos_dr_write( WORD port, BYTE b );

BYTE trdos_sp_read( WORD port );
void trdos_sp_write( WORD port, BYTE b );

typedef enum trdos_drive_number {
  TRDOS_DRIVE_A = 0,
  TRDOS_DRIVE_B,
} trdos_drive_number;

int trdos_disk_insert( trdos_drive_number which, const char *filename );
int trdos_disk_eject( trdos_drive_number which );
int trdos_event_cmd_done( DWORD last_tstates );
int trdos_event_index( DWORD last_tstates );

#endif                  /* #ifndef FUSE_TRDOS_H */
