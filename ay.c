/* ay.c: AY-8-3912 routines
   Copyright (c) 1999-2001 Philip Kendall

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

#include <config.h>

#include "machine.h"
#include "sound.h"

/* What happens when the AY register port (traditionally 0xfffd on the 128K
   machines) is read from */
BYTE ay_registerport_read(WORD port)
{
  return machine_current->ay.registers[ machine_current->ay.current_register ];
}

/* And when it's written to */
void ay_registerport_write(WORD port, BYTE b)
{
  if(b < 15) machine_current->ay.current_register = b;
}

/* What happens when the AY data port (traditionally 0xbffd on the 128K
   machines) is written to; no corresponding read function as this
   always returns 0xff */
void ay_dataport_write(WORD port, BYTE b)
{
  machine_current->ay.registers[ machine_current->ay.current_register ] = b;
  sound_ay_write( machine_current->ay.current_register, b );
}
