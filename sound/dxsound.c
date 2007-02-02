/* dxsound.c: DirectX sound I/O
   Copyright (c) 2003-2004 Marek Januszewski, Philip Kendall

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

*/

#include <config.h>

#include "lowlevel.h"

#ifdef SOUND_DX

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#include "sound.h"
#include "ui/ui.h"

/* same as for SDL Sound */
#define MAX_AUDIO_BUFFER 8192*5

LPDIRECTSOUND lpDS; /* DirectSound object */
LPDIRECTSOUNDBUFFER lpDSBuffer; /* sound buffer */

DWORD nextpos; /* next position is circular buffer */

int
sound_lowlevel_init( const char *device, int *freqptr, int *stereoptr )
{

  WAVEFORMATEX pcmwf; /* waveformat struct */ 
  DSBUFFERDESC dsbd; /* buffer description */

  /* Initialize COM */
  CoInitialize(NULL);

  /* create DirectSound object */
  if( CoCreateInstance( &CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
			&IID_IDirectSound, (void**)&lpDS ) != DS_OK ) {
    ui_error( UI_ERROR_ERROR, "Couldn't create DirectSound object.");
    return 1;
  }
  
  /* initialize it */    
  if( IDirectSound_Initialize( lpDS, NULL ) != DS_OK ) {
    ui_error( UI_ERROR_ERROR, "Couldn't initialize DirectSound." );
    return 1;
  }
  
  /* set normal cooperative level */
  if( IDirectSound_SetCooperativeLevel( lpDS, GetDesktopWindow(),
					DSSCL_NORMAL ) != DS_OK ) {
    ui_error( UI_ERROR_ERROR, "Couldn't set DirectSound cooperation level." );
    return 1;
  }
  
  /* create wave format description */
  memset( &pcmwf, 0, sizeof( WAVEFORMATEX ) );

  pcmwf.cbSize = 0;
  pcmwf.nChannels = *stereoptr ? 2 : 1;
  pcmwf.nBlockAlign = pcmwf.nChannels; /* number of channels *
					  number of bytes per channel */
  pcmwf.nSamplesPerSec = *freqptr;

  pcmwf.nAvgBytesPerSec = pcmwf.nSamplesPerSec * pcmwf. nBlockAlign;

  pcmwf.wBitsPerSample = 8;
  pcmwf.wFormatTag = WAVE_FORMAT_PCM;

  /* create sound buffer description */
  memset( &dsbd, 0, sizeof( DSBUFFERDESC ) );
  dsbd.dwBufferBytes = MAX_AUDIO_BUFFER;

  dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME | 
                 DSBCAPS_CTRLFREQUENCY | DSBCAPS_STATIC | DSBCAPS_LOCSOFTWARE;

  dsbd.dwSize = sizeof( DSBUFFERDESC );
  dsbd.lpwfxFormat = &pcmwf;
  
  /* attempt to create the buffer */  
  if( IDirectSound_CreateSoundBuffer( lpDS, &dsbd, &lpDSBuffer, NULL )
      != DS_OK ) {
    ui_error( UI_ERROR_ERROR, "Couldn't create DirectSound buffer." );
    return 1;
  }
  
  /* play buffer */
  if( IDirectSoundBuffer_Play( lpDSBuffer, 0, 0, DSBPLAY_LOOPING ) != DS_OK ) {
    ui_error( UI_ERROR_ERROR, "Couldn't play sound." );
    return 1;
  }

  nextpos = 0;

  return 0;      
}

void
sound_lowlevel_end( void )
{
  if( IDirectSoundBuffer_Stop( lpDSBuffer ) != DS_OK ) {
    ui_error( UI_ERROR_ERROR, "Couldn't stop sound." );
  }

  IDirectSoundBuffer_Release( lpDSBuffer );
  IDirectSound_Release( lpDS );
  CoUninitialize();
}

/* Copying data to the buffer */
void
sound_lowlevel_frame( unsigned char *data, int len )
{
  HRESULT hres;
  int i;

  /* two pair because of circular buffer */
  UCHAR * ucbuffer1, *ucbuffer2;
  DWORD length1, length2;
  
  /* lock the buffer */
  hres = IDirectSoundBuffer_Lock( lpDSBuffer, nextpos, (DWORD)len,
				  (void **)&ucbuffer1, &length1,
				  (void **)&ucbuffer2, &length2,
				  DSBLOCK_ENTIREBUFFER );
  if ( hres != DS_OK ) return; /* couldn't get a lock on the buffer */

  /* write to the first part of buffer */
  for( i = 0; i < length1 && i < len; i++ ) {
    ucbuffer1[ i ] = *data++;
    nextpos++;
  }

  /* write to the second part of buffer */
  for( i = 0; i < length2 && i + length1 < len; i++ ) {
    ucbuffer2[ i ] = *data++;
    nextpos++;
  }
    
  if(nextpos >= MAX_AUDIO_BUFFER) nextpos -= MAX_AUDIO_BUFFER;
      
  /* unlock the buffer */
  IDirectSoundBuffer_Unlock( lpDSBuffer, ucbuffer1, length1, ucbuffer2,
			     length2 );
}

#endif		/* #ifdef SOUND_DX */
