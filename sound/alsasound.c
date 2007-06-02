/* alsasound.c: ALSA (Linux) sound I/O
   Copyright (c) 2006 Gergely Szasz

   $Id: $

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

#ifdef SOUND_ALSA

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <alsa/asoundlib.h>

#include "settings.h"
#include "sfifo.h"
#include "sound.h"
#include "spectrum.h"
#include "ui/ui.h"

/* Number of Spectrum frames audio latency to use */
#define NUM_FRAMES 2

static snd_pcm_t *pcm_handle;
static snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
static int ch, framesize;
static const char *pcm_name = NULL;
static int verb;
static snd_pcm_uframes_t exact_framesiz, exact_bsize;

void
sound_lowlevel_end( void )
{
/* Stop PCM device and drop pending frames */
  settings_current.sound = 0;
  snd_pcm_drop( pcm_handle );
  snd_pcm_close( pcm_handle );
}

int
sound_lowlevel_init( const char *device, int *freqptr, int *stereoptr )
{
  unsigned int exact_rate, periods;
  unsigned int val, n;
  snd_pcm_hw_params_t *hw_params;
  snd_pcm_sw_params_t *sw_params;
  snd_pcm_uframes_t sound_framesiz, bsize = 0;
  static int first_init = 1;
  const char *option;
  char tmp;
  int err, dir;

  float hz;

/* select a default device if we weren't explicitly given one */

  option = device;
  while( option && *option ) {
    tmp = '*';
    if( ( ( err = sscanf( option, " buffer=%i %n%c", &val, &n, &tmp ) == 2 ) &&
		tmp == ',' ) || ( err == 1 && strlen( option ) == n ) ) {
      bsize = val;
      if( bsize < 1 ) {
	ui_error( UI_ERROR_WARNING, "bad value for ALSA buffer size %i, using default",
		    val );
	bsize = 0;
      }
    } else if( ( ( err = sscanf( option, " verbose %n%c", &n, &tmp ) == 1 ) &&
		tmp == ',' ) || strlen( option ) == n ) {
      verb = 1;
    } else {					/* try as device name */
	while( isspace(*option) )
          option++;
	if( *option == '\'' )		/* force device... */
	  option++;
	pcm_name = option;
	n = strlen( pcm_name );
    }
    option += n + ( tmp == ',' );
  }

/* Open the sound device
 */
  if( pcm_name == NULL || *pcm_name == '\0' )
    pcm_name = "default";
  if( snd_pcm_open( &pcm_handle, pcm_name , stream, 0 ) < 0 ) {
    if( strcmp( pcm_name, "default" ) == 0 ) {
    /* we try a last one: plughw:0,0 but what a weired ALSA conf.... */
      if( snd_pcm_open( &pcm_handle, "plughw:0,0", stream, 0 ) < 0 ) {
        settings_current.sound = 0;
        ui_error( UI_ERROR_ERROR,
                  "couldn't open sound device 'default' and 'plughw:0,0' check ALSA configuration."
                  );
        return 1;
      } else {
        if( first_init )
          ui_error( UI_ERROR_WARNING,
                    "couldn't open sound device 'default', using 'plughw:0,0' check ALSA configuration."
                    );
      }
    }
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "couldn't open sound device '%s'.", pcm_name );
    return 1;
  }

/* Allocate the snd_pcm_hw_params_t structure on the stack. */
  snd_pcm_hw_params_alloca( &hw_params );

/* Init hw_params with full configuration space */
  if( snd_pcm_hw_params_any( pcm_handle, hw_params ) < 0 ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR,
              "couldn't get configuration space on sound device '%s'.",
              pcm_name );
    snd_pcm_close( pcm_handle );
    return 1;
  }

  if( snd_pcm_hw_params_set_access( pcm_handle, hw_params,
                                    SND_PCM_ACCESS_RW_INTERLEAVED ) < 0) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "couldn't set access interleaved on '%s'.",
              pcm_name );
    snd_pcm_close( pcm_handle );
    return 1;
  }
  
    /* Set sample format */
  if( snd_pcm_hw_params_set_format( pcm_handle, hw_params, 
#if defined WORDS_BIGENDIAN
				    SND_PCM_FORMAT_S16_BE
#else
				    SND_PCM_FORMAT_S16_LE
#endif
						    ) < 0 ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "couldn't set format on '%s'.", pcm_name );
    snd_pcm_close( pcm_handle );
    return 1;
  }

  ch = *stereoptr ? 2 : 1;

/* Set number of channels, but only if not the same as we require */
#if SND_LIB_MAJOR == 1
  snd_pcm_hw_params_get_channels( hw_params, &val );
#elif SND_LIB_MAJOR == 0 && SND_LIB_MINOR == 9
  val = snd_pcm_hw_params_get_channels( hw_params );
#else
#error Alsa lib incompatible! not 0.9.x nor 1.x.x ...
#endif
  if( val != ch ) {
    if( snd_pcm_hw_params_set_channels( pcm_handle, hw_params, ch )
	    < 0 ) {
      settings_current.sound = 0;
      ui_error( UI_ERROR_ERROR, "couldn't set %s channel on '%s' using %s.",
    		    (*stereoptr ? "stereo" : "mono"), 
		    (*stereoptr ? "mono" : "stereo"), pcm_name );
    }
    *stereoptr = val == 2 ? 1 : 0;
  }

  framesize = ch << 1;			/* we always use 16 bit sorry :-( */
/* Set sample rate. If the exact rate is not supported */
/* by the hardware, use nearest possible rate.         */ 
  exact_rate = *freqptr;
  if( snd_pcm_hw_params_set_rate_near( pcm_handle, hw_params, &exact_rate,
							NULL ) < 0) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "couldn't set rate %d on '%s'.",
						*freqptr, pcm_name );
    snd_pcm_close( pcm_handle );
    return 1;
  }
  if( first_init && *freqptr != exact_rate ) {
    ui_error( UI_ERROR_WARNING,
              "The rate %d Hz is not supported by your hardware. "
              "Using %d Hz instead.\n", *freqptr, exact_rate );
    *freqptr = exact_rate;
  }

  if( bsize != 0 ) {
    exact_framesiz = sound_framesiz = bsize / NUM_FRAMES;
    if( bsize < 1 ) {
      ui_error( UI_ERROR_WARNING,
                "bad value for ALSA buffer size %i, using default",
		val );
      bsize = 0;
    }
  }

  if( bsize == 0 ) {
    hz = (float)machine_current->timings.processor_speed /
              machine_current->timings.tstates_per_frame;
    exact_framesiz = sound_framesiz = *freqptr / hz;
  }

  dir = -1;
  if( snd_pcm_hw_params_set_period_size_near( pcm_handle, hw_params,
					    &exact_framesiz, &dir ) < 0 ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "couldn't set period size %d on '%s'.",
                              (int)sound_framesiz, pcm_name );
    snd_pcm_close( pcm_handle );
    return 1;
  }

  if( first_init && sound_framesiz != exact_framesiz ) {
    ui_error( UI_ERROR_WARNING,
              "The period size %d is not supported by your hardware. "
              "Using %d instead.\n", (int)sound_framesiz,
              (int)exact_framesiz );
  }

  periods = NUM_FRAMES;
/* Set number of periods. Periods used to be called fragments. */
  if( snd_pcm_hw_params_set_periods_near( pcm_handle, hw_params, &periods,
                                          NULL ) < 0 ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "couldn't set periods on '%s'.", pcm_name );
    snd_pcm_close( pcm_handle );
    return 1;
  }

  if( first_init && periods != NUM_FRAMES ) {
    ui_error( UI_ERROR_WARNING, "%d periods is not supported by your hardware. "
                    	     "Using %d instead.\n", NUM_FRAMES, periods );
  }

  snd_pcm_hw_params_get_buffer_size( hw_params, &exact_bsize );

  /* Apply HW parameter settings to */
  /* PCM device and prepare device  */
  if( snd_pcm_hw_params( pcm_handle, hw_params ) < 0 ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR,"couldn't set hw_params on %s", pcm_name );
    snd_pcm_close( pcm_handle );
    return 1;
  }

  snd_pcm_sw_params_alloca( &sw_params );
  if( ( err = snd_pcm_sw_params_current( pcm_handle, sw_params ) ) < 0 ) {
    ui_error( UI_ERROR_ERROR,"couldn't get sw_params from %s: %s", pcm_name,
              snd_strerror ( err ) );
    snd_pcm_close( pcm_handle );
    return 1;
  }

  if( ( err = snd_pcm_sw_params_set_start_threshold( pcm_handle,
		     sw_params, exact_framesiz ) ) < 0 ) {
    ui_error( UI_ERROR_ERROR,"couldn't set start_treshold on %s: %s", pcm_name,
              snd_strerror ( err ) );
    snd_pcm_close( pcm_handle );
    return 1;
  }

  if( ( err = snd_pcm_sw_params_set_xfer_align( pcm_handle, sw_params, 1 ) ) < 0 ) {
    ui_error( UI_ERROR_ERROR,"couldn't set xfer_allign on %s: %s", pcm_name,
              snd_strerror ( err ) );
    return 1;
  }
  
  if( ( err = snd_pcm_sw_params( pcm_handle, sw_params ) ) < 0 ) {
    ui_error( UI_ERROR_ERROR,"couldn't set sw_params on %s: %s", pcm_name,
              snd_strerror ( err ) );
    return 1;
  }

  first_init = 0;
  return 0;	/* success */
}



void
sound_lowlevel_frame( libspectrum_signed_word *data, int len )
{
  int ret = 0;
  len /= ch;	/* now in frames */

/*	to measure sound lag :-)
  snd_pcm_status_t *status;
  snd_pcm_sframes_t delay;
    
  snd_pcm_status_alloca( &status );
  snd_pcm_status( pcm_handle, status ); 
  delay = snd_pcm_status_get_delay( status );
  fprintf( stderr, "%d ", (int)delay );
*/

  while( ( ret = snd_pcm_writei( pcm_handle, data, len ) ) != len ) {
    if( ret < 0 ) {
      snd_pcm_prepare( pcm_handle );
      if( verb )
        fprintf( stderr, "ALSA: buffer underrun!\n" );
    } else {
        len -= ret;
    }
  }
}

#endif	/* #ifdef SOUND_ALSA */
