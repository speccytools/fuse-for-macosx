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

#include <string.h>

#include <SDL.h>

#include "settings.h"
#include "sound.h"
#include "ui/ui.h"

/* using (7) 32 byte frags for 8kHz, scale up for higher */
#define BASE_SOUND_FRAG_PWR	7

static void sdlwrite8( void *userdata, Uint8 *stream, int len );
static void sdlwrite16( void *userdata, Uint8 *stream, int len );

/* ring buffer for SDL audio thread, make larger than standard buffer to *
 * try and avoid any buffer overruns, ideally should be dynamically set  *
 * based on the obtained sample rate */
#define MAX_AUDIO_RB 8192*5
static libspectrum_signed_word ringbuf[MAX_AUDIO_RB];

volatile unsigned int ringbuffer_read = 0;
volatile unsigned int ringbuffer_write = 0;

int
sound_lowlevel_init( const char *device, int *freqptr, int *stereoptr )
{
  SDL_AudioSpec requested;
  int frag;

  SDL_InitSubSystem( SDL_INIT_AUDIO );

  memset( &requested, 0, sizeof( SDL_AudioSpec ) );

  requested.freq = *freqptr;
  requested.channels = *stereoptr ? 2 : 1;

  if( settings_current.sound_force_8bit ) {
    requested.format = AUDIO_U8;
    requested.callback = sdlwrite8;
  } else {
    requested.format = AUDIO_U16;
    requested.callback = sdlwrite16;
  }

  frag = BASE_SOUND_FRAG_PWR;
  if (*freqptr > 8250)   
    frag++;  
  if (*freqptr > 16500)
    frag++;      
  if (*freqptr > 33000)
    frag++;

  requested.samples = 1 << frag;

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
}

/* Copy data to ringbuffer */
void
sound_lowlevel_frame( libspectrum_signed_word *data, int len )
{
  int i;

  SDL_LockAudio();

  for( i=0; i<len; i++ ) {
    ringbuf[ ringbuffer_write++ ] = *data++;
    if( ringbuffer_write == MAX_AUDIO_RB ) ringbuffer_write = 0;
    /* Sound buffer overflow! - drop extra samples */
    if( ringbuffer_write == ringbuffer_read ) break;
  }

  SDL_UnlockAudio();
}

/* Write len samples from ringbuffer into stream */
void
sdlwrite8( void *userdata, Uint8 *stream, int len )
{
  int f;

  for( f=0; f<len; f++ ) {
    if( ringbuffer_write == ringbuffer_read ) {
      /* buffer underrun - send last sample available */
      *stream++ = ringbuf[ ringbuffer_read ] >> 8;
    } else {
      *stream++ = ringbuf[ ringbuffer_read++ ] >> 8;
      if( ringbuffer_read == MAX_AUDIO_RB ) ringbuffer_read = 0;
    }
  }
}

void
sdlwrite16( void *userdata, Uint8 *stream8, int len )
{
  int f;
  libspectrum_signed_word *stream;

  stream = (libspectrum_signed_word*)stream8;
  len /= 2;

  for( f=0; f<len; f++ ) {
    if( ringbuffer_write == ringbuffer_read ) {
      /* buffer underrun - send last sample available */
      *stream++ = ringbuf[ ringbuffer_read ];
    } else {
      *stream++ = ringbuf[ ringbuffer_read++ ];
      if( ringbuffer_read == MAX_AUDIO_RB ) ringbuffer_read = 0;
    }
  }
}

#endif			/* #ifdef SOUND_SDL */
