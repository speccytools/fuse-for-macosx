/* sdlui.c: Routines for dealing with the SDL user interface
   Copyright (c) 2000-2002 Philip Kendall, Matan Ziv-Av, Fredrick Meunier

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

#ifdef UI_SDL			/* Use this iff we're using SDL */

#include <stdio.h>
#include <SDL.h>

#include "fuse.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "sdldisplay.h"
#include "sdlkeyboard.h"
#include "ui/scaler/scaler.h"

void
atexit_proc( void )
{ 
  SDL_ShowCursor(SDL_ENABLE);
  SDL_Quit();
}

int 
ui_init( int *argc, char ***argv )
{
  int error;

/* Comment out to Work around a bug in OS X 10.1 related to OpenGL in windowed
   mode */
  atexit(atexit_proc);

  error = SDL_Init( SDL_INIT_VIDEO );
  if ( error )
    return error;

  scaler_register_clear();

  scaler_register( SCALER_NORMAL );
  scaler_register( SCALER_DOUBLESIZE );
  scaler_register( SCALER_TRIPLESIZE );
  scaler_register( SCALER_2XSAI );
  scaler_register( SCALER_SUPER2XSAI );
  scaler_register( SCALER_SUPEREAGLE );
  scaler_register( SCALER_ADVMAME2X );

  return 0;
}

int 
ui_event( void )
{
  SDL_Event event;

  while ( SDL_PollEvent( &event ) ) {
    switch ( event.type ) {
    case SDL_KEYDOWN:
      sdlkeyboard_keypress( &(event.key) );
      break;
    case SDL_KEYUP:
      sdlkeyboard_keyrelease( &(event.key) );
      break;
    case SDL_QUIT:
      fuse_exiting = 1;
      break;
    default:
      break;
    }
  }

  return 0;
}

int 
ui_end( void )
{
  int error;

  error = uidisplay_end();
  if ( error )
    return error;

  SDL_Quit();

  return 0;
}

#endif				/* #ifdef UI_SDL */
