/* coreaudiosound.c: Mac OS X CoreAudio sound I/O
   Copyright (c) 2006 Fredrick Meunier

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

#ifdef SOUND_COREAUDIO

#include <errno.h>
#include <unistd.h>
#include <CoreAudio/AudioHardware.h>
#include <AudioUnit/AudioUnit.h>

#include "sfifo.h"
#include "sound.h"
#include "ui/ui.h"

sfifo_t sound_fifo;

static
OSStatus coreaudiowrite( void *inRefCon,
                         AudioUnitRenderActionFlags *ioActionFlags,
                         const AudioTimeStamp *inTimeStamp,
                         UInt32 inBusNumber,
                         UInt32 inNumberFrames,                       
                         AudioBufferList *ioData );

/* info about the format used for writing to output unit */
static AudioStreamBasicDescription deviceFormat;

/* converts from Fuse format to CoreAudio format */
static AudioUnit gOutputUnit;

int
sound_lowlevel_init( const char *dev, int *freqptr, int *stereoptr )
{
  OSStatus err = kAudioHardwareNoError;
  UInt32 count;
  AudioDeviceID device = kAudioDeviceUnknown; /* the default device */
  UInt32 deviceBufferSize;  /* bufferSize returned by
                               kAudioDevicePropertyBufferSize */
  int error;

  /* get the default output device for the HAL */
  count = sizeof( device );   /* it is required to pass the size of the data
                                 to be returned */
  err = AudioHardwareGetProperty( kAudioHardwarePropertyDefaultOutputDevice,
                                  &count, (void *) &device );
  if ( err != kAudioHardwareNoError ) {
    ui_error( UI_ERROR_ERROR,
              "get kAudioHardwarePropertyDefaultOutputDevice error %ld",
              err );
    return 1;
  }

  /* get the buffersize that the default device uses for IO */
  count = sizeof( deviceBufferSize ); /* it is required to pass the size of
                                         the data to be returned */
  err = AudioDeviceGetProperty( device, 0, false, kAudioDevicePropertyBufferSize,
                                &count, &deviceBufferSize );
  if( err != kAudioHardwareNoError ) {
    ui_error( UI_ERROR_ERROR, "get kAudioDevicePropertyBufferSize error %ld",
              err );
    return 1;
  }

  /* get a description of the data format used by the default device */
  count = sizeof( deviceFormat ); /* it is required to pass the size of the
                                     data to be returned */
  err = AudioDeviceGetProperty( device, 0, false,
                                kAudioDevicePropertyStreamFormat, &count,
                                &deviceFormat );
  if( err != kAudioHardwareNoError ) {
    ui_error( UI_ERROR_ERROR,
              "get kAudioDevicePropertyStreamFormat error %ld", err );
    return 1;
  }

  *freqptr = deviceFormat.mSampleRate;

  deviceFormat.mFormatFlags =  kLinearPCMFormatFlagIsSignedInteger
#ifdef WORDS_BIGENDIAN
                    | kLinearPCMFormatFlagIsBigEndian
#endif      /* #ifdef WORDS_BIGENDIAN */
                    | kLinearPCMFormatFlagIsPacked
                    | kAudioFormatFlagIsNonInterleaved;
  deviceFormat.mBytesPerPacket = 2;
  deviceFormat.mFramesPerPacket = 1;
  deviceFormat.mBytesPerFrame = 2;
  deviceFormat.mBitsPerChannel = 16;
  deviceFormat.mChannelsPerFrame = *stereoptr ? 2 : 1;

  /* Open the default output unit */
  ComponentDescription desc;
  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = kAudioUnitSubType_DefaultOutput;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;

  Component comp = FindNextComponent( NULL, &desc );
  if( comp == NULL ) {
    ui_error( UI_ERROR_ERROR, "FindNextComponent" );
    return 1;
  }

  err = OpenAComponent(comp, &gOutputUnit);
  if( comp == NULL ) {
    ui_error( UI_ERROR_ERROR, "OpenAComponent=%ld", err );
    return 1;
  }

  /* Set up a callback function to generate output to the output unit */
  AURenderCallbackStruct input;
  input.inputProc = coreaudiowrite;
  input.inputProcRefCon = NULL;

  err = AudioUnitSetProperty (gOutputUnit,                       
                              kAudioUnitProperty_SetRenderCallback,
                              kAudioUnitScope_Input,
                              0,
                              &input,
                              sizeof(input));
  if( err ) {
    ui_error( UI_ERROR_ERROR, "AudioUnitSetProperty-CB=%ld", err );
    return 1;
  }

  err = AudioUnitSetProperty (gOutputUnit,
                              kAudioUnitProperty_StreamFormat,
                              kAudioUnitScope_Input,
                              0,
                              &deviceFormat,
                              sizeof(AudioStreamBasicDescription));
  if( err ) {
    ui_error( UI_ERROR_ERROR, "AudioUnitSetProperty-SF=%4.4s, %ld", (char*)&err, err );
    return 1;
  }

  err = AudioUnitInitialize(gOutputUnit);
  if( err ) {
    ui_error( UI_ERROR_ERROR, "AudioUnitInitialize=%ld", err );
    return 1;
  }

  if( ( error = sfifo_init( &sound_fifo, 2 * deviceFormat.mChannelsPerFrame
                                         * deviceBufferSize + 1 ) ) ) {
    ui_error( UI_ERROR_ERROR, "Problem initialising sound fifo: %s",
              strerror ( error ) );
    return 1;
  }

  /* Start the rendering
     The DefaultOutputUnit will do any format conversions to the format of the
     default device */
  err = AudioOutputUnitStart (gOutputUnit);
  if (err) {
    ui_error( UI_ERROR_ERROR, "AudioOutputUnitStart=%ld", err );
    return 1;
  }

  return 0;
}

void
sound_lowlevel_end( void )
{
  OSStatus err;

  verify_noerr (AudioOutputUnitStop (gOutputUnit));

  err = AudioUnitUninitialize (gOutputUnit);
  if( err ) {
    printf( "AudioUnitUninitialize=%ld", err );
  }

  CloseComponent( gOutputUnit );

  sfifo_flush( &sound_fifo );
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
    } else if (!i) {
      usleep(10000);
    }
    bytes += i;
    len -= i;
  }
  if( i < 0 ) {
    ui_error( UI_ERROR_ERROR, "Couldn't write sound fifo: %s",
              strerror( i ) );
  }
}

/* This is the audio processing callback. */
OSStatus coreaudiowrite( void *inRefCon,
                         AudioUnitRenderActionFlags *ioActionFlags,
                         const AudioTimeStamp *inTimeStamp,
                         UInt32 inBusNumber,
                         UInt32 inNumberFrames,                       
                         AudioBufferList *ioData )
{
  int f;
  int len = deviceFormat.mBytesPerFrame * inNumberFrames;

  uint8_t* out_l = ioData->mBuffers[0].mData;
  uint8_t* out_r = deviceFormat.mChannelsPerFrame > 1 ?
                    ioData->mBuffers[1].mData : 0;

  if( out_r ) {
    /* Deinterleave the left and right stereo channels into their approptiate
       buffers */
    while( sfifo_used( &sound_fifo ) && len ) {
      f = sfifo_read( &sound_fifo, out_l, deviceFormat.mBytesPerFrame );
      out_l += deviceFormat.mBytesPerFrame;
      f = sfifo_read( &sound_fifo, out_r, deviceFormat.mBytesPerFrame );
      out_r += deviceFormat.mBytesPerFrame;
      len -= deviceFormat.mBytesPerFrame;
    }

    /* If we ran out of sound, make do with silence :( */
    if( len ) {
      for( f=0; f<len; f++ ) {
        *out_l++ = 0;
        *out_r++ = 0;
      }
    }
  } else {
    /* Read input_size bytes from fifo into sound stream */
    while( ( f = sfifo_read( &sound_fifo, out_l, len ) ) > 0 ) {
      out_l += f;
      len -= f;
    }

    /* If we ran out of sound, make do with silence :( */
    if( f < 0 ) {
      for( f=0; f<len; f++ ) {
        *out_l++ = 0;
      }
    }
  }

  return noErr;
}

#endif			/* #ifdef SOUND_COREAUDIO */
