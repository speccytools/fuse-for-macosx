/* sdlsound.c: SDL sound I/O
   Copyright (c) 2002-2004 Alexander Yurchenko, Russell Marks, Philip Kendall,
			   Fredrick Meunier

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

*/

#include <config.h>

#include "lowlevel.h"

#ifdef SOUND_SDL

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <SDL.h>

#include "settings.h"
#include "sfifo.h"
#include "sound.h"
#include "ui/ui.h"

/* using (7) 32 byte frags for 8kHz, scale up for higher */
#define BASE_SOUND_FRAG_PWR	7

static void sdlwrite( void *userdata, Uint8 *stream, int len );

sfifo_t sound_fifo;

int
sound_lowlevel_init( const char *device, int *freqptr, int *stereoptr )
{
  SDL_AudioSpec requested;
  int frag;
  int error;

  if( device && ( error = setenv( "SDL_AUDIODRIVER", device, 1 ) ) ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "Couldn't set SDL_AUDIODRIVER: %s",
              strerror ( error ) );
    return 1;
  }

  SDL_InitSubSystem( SDL_INIT_AUDIO );

  memset( &requested, 0, sizeof( SDL_AudioSpec ) );

  requested.freq = *freqptr;
  requested.channels = *stereoptr ? 2 : 1;
  requested.format = AUDIO_S16SYS;
  requested.callback = sdlwrite;

  frag = BASE_SOUND_FRAG_PWR;
  if (*freqptr > 8250)   
    frag++;  
  if (*freqptr > 16500)
    frag++;      
  if (*freqptr > 33000)
    frag++;

  requested.samples = 1 << frag;

  /* Convert from 16-bit stereo samples to bytes plus some headroom */
  frag = 1 << (frag+4);
  if( ( error = sfifo_init( &sound_fifo, frag ) ) ) {
    ui_error( UI_ERROR_ERROR, "Problem initialising sound fifo: %s",
              strerror ( error ) );
    return 1;
  }

  if ( SDL_OpenAudio( &requested, NULL ) < 0 ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "Couldn't open sound device: %s",
              SDL_GetError() );
    return 1;
  }

  SDL_PauseAudio( 0 );

  return 0;
}

void
sound_lowlevel_end( void )
{
  SDL_PauseAudio( 1 );
  SDL_LockAudio();
  SDL_CloseAudio();
  SDL_QuitSubSystem( SDL_INIT_AUDIO );
  sfifo_close( &sound_fifo );
}

/* Copy data to fifo */
void
sound_lowlevel_frame( libspectrum_signed_word *data, int len )
{
  int i = 0;

  /* Convert to bytes */
  libspectrum_signed_byte* bytes = (libspectrum_signed_byte*)data;
  len <<= 1;

  while( len ) {
    if( ( i = sfifo_write( &sound_fifo, bytes, len ) ) < 0 ) {
      break;
    }
    bytes += i;
    len -= i;
  }
  if( i < 0 ) {
    ui_error( UI_ERROR_ERROR, "Couldn't write sound fifo: %s\n",
              strerror( i ) );
  }
}

/* Write len samples from fifo into stream */
void
sdlwrite( void *userdata, Uint8 *stream, int len )
{
  int f;

  /* Read input_size bytes from fifo into sound stream */
  while( ( f = sfifo_read( &sound_fifo, stream, len ) ) > 0 ) {
    stream += f;
    len -= f;
  }
  if( f < 0 ) {
    for( f=0; f<len; f++ ) {
      *stream++ = 0;
    }
    return;
  }
}

#endif			/* #ifdef SOUND_SDL */
