/* printer.c: Printer support
   Copyright (c) 2001 Ian Collier, Russell Marks, Philip Kendall

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

#ifndef FUSE_PRINTER_H
#define FUSE_PRINTER_H

#ifndef FUSE_TYPES_H
#include "types.h"
#endif			/* #ifndef FUSE_TYPES_H */

BYTE printer_zxp_read(WORD port);
void printer_zxp_write(WORD port,BYTE b);
void printer_zxp_reset(void);
void printer_frame(void);
void printer_serial_write(BYTE b);
void printer_parallel_strobe_write(int on);
BYTE printer_parallel_read(WORD port);
void printer_parallel_write(WORD port,BYTE b);
int printer_init(void);
void printer_end(void);

#endif				/* #ifndef FUSE_PRINTER_H */
