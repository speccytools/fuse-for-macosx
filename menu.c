/* menu.c: general menu callbacks
   Copyright (c) 2004-2005 Philip Kendall

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#include <config.h>

#include <libspectrum.h>

#include "dck.h"
#include "disk/beta.h"
#include "disk/plusd.h"
#include "divide.h"
#include "event.h"
#include "fuse.h"
#include "if1.h"
#include "if2.h"
#include "menu.h"
#include "profile.h"
#include "psg.h"
#include "rzx.h"
#include "simpleide.h"
#include "screenshot.h"
#include "settings.h"
#include "snapshot.h"
#include "tape.h"
#include "ui/ui.h"
#include "utils.h"
#include "widget/widget.h"
#include "zxatasp.h"
#include "zxcf.h"

#ifdef USE_WIDGET
#define WIDGET_END widget_finish()
#else				/* #ifdef USE_WIDGET */
#define WIDGET_END
#endif				/* #ifdef USE_WIDGET */

MENU_CALLBACK( menu_file_open )
{
  char *filename;

  fuse_emulation_pause();

  filename = menu_get_open_filename( "Fuse - Open Spectrum File" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  utils_open_file( filename, settings_current.auto_load, NULL );

  free( filename );

  display_refresh_all();

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_recording_insertsnapshot )
{
  libspectrum_snap *snap;
  libspectrum_error error;

  if( !rzx_recording ) return;

  WIDGET_END;

  libspectrum_rzx_stop_input( rzx );

  error = libspectrum_snap_alloc( &snap );
  if( error ) return;

  error = snapshot_copy_to( snap );
  if( error ) { libspectrum_snap_free( snap ); return; }

  libspectrum_rzx_add_snap( rzx, snap );

  libspectrum_rzx_start_input( rzx, tstates );
}

MENU_CALLBACK( menu_file_recording_rollback )
{
  libspectrum_error error;
  
  if( !rzx_recording ) return;

  WIDGET_END;

  fuse_emulation_pause();

  error = rzx_rollback();
  if( error ) { fuse_emulation_unpause(); return; }

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_recording_rollbackto )
{
  libspectrum_error error;

  if( !rzx_recording ) return;

  WIDGET_END;

  fuse_emulation_pause();

  error = rzx_rollback_to();
  if( error ) { fuse_emulation_unpause(); return; }

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_recording_play )
{
  char *recording;

  if( rzx_playback || rzx_recording ) return;

  fuse_emulation_pause();

  recording = menu_get_open_filename( "Fuse - Start Replay" );
  if( !recording ) { fuse_emulation_unpause(); return; }

  rzx_start_playback( recording );

  free( recording );

  display_refresh_all();

  ui_menu_activate( UI_MENU_ITEM_RECORDING, 1 );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_recording_stop )
{
  if( !( rzx_recording || rzx_playback ) ) return;

  WIDGET_END;

  if( rzx_recording ) rzx_stop_recording();
  if( rzx_playback  ) rzx_stop_playback( 1 );
}  

MENU_CALLBACK( menu_file_aylogging_stop )
{
  if ( !psg_recording ) return;

  WIDGET_END;

  psg_stop_recording();
  ui_menu_activate( UI_MENU_ITEM_AY_LOGGING, 0 );
}

MENU_CALLBACK( menu_file_openscrscreenshot )
{
  char *filename;

  fuse_emulation_pause();

  filename = menu_get_open_filename( "Fuse - Open SCR Screenshot" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  screenshot_scr_read( filename );

  free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_movies_stopmovierecording )
{
  WIDGET_END;

  screenshot_movie_record = 0;
  ui_menu_activate( UI_MENU_ITEM_FILE_MOVIES_RECORDING, 0 );
}

MENU_CALLBACK_WITH_ACTION( menu_options_selectroms_select )
{
  switch( action ) {

  case  1: menu_select_roms( LIBSPECTRUM_MACHINE_16,      0, 1 ); return;
  case  2: menu_select_roms( LIBSPECTRUM_MACHINE_48,      1, 1 ); return;
  case  3: menu_select_roms( LIBSPECTRUM_MACHINE_128,     2, 2 ); return;
  case  4: menu_select_roms( LIBSPECTRUM_MACHINE_PLUS2,   4, 2 ); return;
  case  5: menu_select_roms( LIBSPECTRUM_MACHINE_PLUS2A,  6, 4 ); return;
  case  6: menu_select_roms( LIBSPECTRUM_MACHINE_PLUS3,  10, 4 ); return;
  case  7: menu_select_roms( LIBSPECTRUM_MACHINE_TC2048, 14, 1 ); return;
  case  8: menu_select_roms( LIBSPECTRUM_MACHINE_TC2068, 15, 2 ); return;
  case  9: menu_select_roms( LIBSPECTRUM_MACHINE_PENT,   17, 3 ); return;
  case 10: menu_select_roms( LIBSPECTRUM_MACHINE_SCORP,  20, 4 ); return;
  case 11: menu_select_roms( LIBSPECTRUM_MACHINE_PLUS3E, 24, 4 ); return;
  case 12: menu_select_roms( LIBSPECTRUM_MACHINE_SE,     28, 2 ); return;
  case 13: menu_select_roms( LIBSPECTRUM_MACHINE_48,     30, 1 ); return;
  case 14: menu_select_roms( LIBSPECTRUM_MACHINE_TS2068, 31, 2 ); return;

  }

  ui_error( UI_ERROR_ERROR,
	    "menu_options_selectroms_select: unknown action %d", action );
  fuse_abort();
}

MENU_CALLBACK( menu_options_filter )
{
  scaler_type scaler;

  /* Stop emulation */
  fuse_emulation_pause();

  scaler = menu_get_scaler( scaler_is_supported );
  if( scaler != SCALER_NUM && scaler != current_scaler )
    scaler_select_scaler( scaler );

  /* Carry on with emulation again */
  fuse_emulation_unpause();
}

#ifdef HAVE_LIB_XML2
MENU_CALLBACK( menu_options_save )
{
  WIDGET_END;
  settings_write_config( &settings_current );
}
#endif				/* #ifdef HAVE_LIB_XML2 */

MENU_CALLBACK( menu_machine_profiler_start )
{
  WIDGET_END;
  profile_start();
}

MENU_CALLBACK( menu_machine_profiler_stop )
{
  char *filename;

  fuse_emulation_pause();

  filename = menu_get_save_filename( "Fuse - Save profile data" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  profile_finish( filename );

  free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_machine_nmi )
{
  WIDGET_END;
  event_add( 0, EVENT_TYPE_NMI );
}

MENU_CALLBACK( menu_media_tape_open )
{
  char *filename;

  fuse_emulation_pause();

  filename = menu_get_open_filename( "Fuse - Open Tape" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  tape_open_default_autoload( filename, NULL );

  free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_media_tape_play )
{
  WIDGET_END;
  tape_toggle_play( 0 );
}

MENU_CALLBACK( menu_media_tape_rewind )
{
  WIDGET_END;
  tape_select_block( 0 );
}

MENU_CALLBACK( menu_media_tape_clear )
{
  WIDGET_END;
  tape_close();
}

MENU_CALLBACK( menu_media_tape_write )
{
  ui_tape_write();
}

MENU_CALLBACK( menu_media_tape_recordstart )
{
  WIDGET_END;
  tape_record_start();
}

MENU_CALLBACK( menu_media_tape_recordstop )
{
  WIDGET_END;
  tape_record_stop();
}

MENU_CALLBACK_WITH_ACTION( menu_media_mdr_new )
{
  WIDGET_END;

  if1_mdr_new( action - 1 );

}

MENU_CALLBACK_WITH_ACTION( menu_media_mdr_insert )
{
  char *filename;

  fuse_emulation_pause();

  filename = menu_get_open_filename( "Fuse - Insert microdrive disk file" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  if1_mdr_insert( filename, action - 1 );

  free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK_WITH_ACTION( menu_media_mdr_sync )
{

  if( if1_mdr_sync( NULL, action - 1 ) ) {
    char *filename;

    fuse_emulation_pause();

    filename = menu_get_save_filename( "Fuse - Write microdrive disk to file" );
    if( !filename ) { fuse_emulation_unpause(); return; }

    if1_mdr_sync( filename, action - 1 );

    free( filename );

    fuse_emulation_unpause();
  } else {
    WIDGET_END;
  }
}

MENU_CALLBACK_WITH_ACTION( menu_media_mdr_eject )
{

  if( if1_mdr_eject( NULL, action - 1 ) ) {
    char *filename;

    fuse_emulation_pause();

    filename = menu_get_save_filename( "Fuse - Write microdrive disk to file" );
    if( !filename ) { fuse_emulation_unpause(); return; }

    if1_mdr_eject( filename, action - 1 );

    free( filename );

    fuse_emulation_unpause();
  } else {
    WIDGET_END;
  }
}

MENU_CALLBACK_WITH_ACTION( menu_media_mdr_writep )
{
  WIDGET_END;

  if1_mdr_writep( action & 0xf0, ( action & 0x0f ) - 1 );

}

MENU_CALLBACK_WITH_ACTION( menu_media_if1_rs232 )
{
  char *filename;

  fuse_emulation_pause();

  if( action & 0xf0 ) {
    WIDGET_END;
    if1_unplug( action & 0x0f );
  } else {
    filename = menu_get_open_filename( "Fuse - Select file for communication" );
    if( !filename ) { fuse_emulation_unpause(); return; }

    if1_plug( filename, action );

    free( filename );
  }
  fuse_emulation_unpause();

}

MENU_CALLBACK_WITH_ACTION( menu_media_disk_insert )
{
  char *filename;
  int which, type;
  
  action--;
  which = action & 0x03;
  type = ( action & 0x0c ) >> 2;

  fuse_emulation_pause();

  filename = menu_get_open_filename( "Fuse - Insert disk" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  switch( type ) {
  case 0:
#ifdef HAVE_765_H
    specplus3_disk_insert_default_autoload( which, filename );
#endif				/* #ifdef HAVE_765_H */
    break;
  case 1:
    beta_disk_insert_default_autoload( which, filename );
    break;
  case 2:
    plusd_disk_insert_default_autoload( which, filename );
    break;
  }

  free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK_WITH_ACTION( menu_media_disk_eject )
{
  int which, write, type;

  WIDGET_END;

  action--;
  which = action & 0x03;
  type = ( action & 0x0c ) >> 2;
  write = action & 0x10;

  switch( type ) {
  case 0:
#ifdef HAVE_765_H
    specplus3_disk_eject( which, write );
#endif			/* #ifdef HAVE_765_H */
    break;
  case 1:
    beta_disk_eject( which, write );
    break;
  case 2:
    plusd_disk_eject( which, write );
    break;
  }

}

MENU_CALLBACK( menu_media_cartridge_timexdock_insert )
{
  char *filename;

  fuse_emulation_pause();

  filename = menu_get_open_filename( "Fuse - Insert Timex Dock Cartridge" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  dck_insert( filename );

  free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_media_cartridge_timexdock_eject )
{
  WIDGET_END;
  dck_eject();
}

MENU_CALLBACK( menu_media_cartridge_interfaceii_insert )
{
  char *filename;

  fuse_emulation_pause();

  filename = menu_get_open_filename( "Fuse - Insert Interface II Cartridge" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  if2_insert( filename );

  free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_media_cartridge_interfaceii_eject )
{
  WIDGET_END;
  if2_eject();
}

MENU_CALLBACK_WITH_ACTION( menu_media_ide_insert )
{
  char *filename;

  fuse_emulation_pause();

  filename = menu_get_open_filename( "Fuse - Insert hard disk file" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  switch( action ) {
  case 1: simpleide_insert( filename, LIBSPECTRUM_IDE_MASTER ); break;
  case 2: simpleide_insert( filename, LIBSPECTRUM_IDE_SLAVE  ); break;
  case 3: zxatasp_insert( filename, LIBSPECTRUM_IDE_MASTER ); break;
  case 4: zxatasp_insert( filename, LIBSPECTRUM_IDE_SLAVE  ); break;
  case 5: zxcf_insert( filename ); break;
  case 6: divide_insert( filename, LIBSPECTRUM_IDE_MASTER ); break;
  case 7: divide_insert( filename, LIBSPECTRUM_IDE_SLAVE  ); break;
  }

  free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK_WITH_ACTION( menu_media_ide_commit )
{
  fuse_emulation_pause();

  switch( action ) {
  case 1: simpleide_commit( LIBSPECTRUM_IDE_MASTER ); break;
  case 2: simpleide_commit( LIBSPECTRUM_IDE_SLAVE  ); break;
  case 3: zxatasp_commit( LIBSPECTRUM_IDE_MASTER ); break;
  case 4: zxatasp_commit( LIBSPECTRUM_IDE_SLAVE  ); break;
  case 5: zxcf_commit(); break;
  case 6: divide_commit( LIBSPECTRUM_IDE_MASTER ); break;
  case 7: divide_commit( LIBSPECTRUM_IDE_SLAVE  ); break;
  }

  fuse_emulation_unpause();

  WIDGET_END;
}

MENU_CALLBACK_WITH_ACTION( menu_media_ide_eject )
{
  fuse_emulation_pause();

  switch( action ) {
  case 1: simpleide_eject( LIBSPECTRUM_IDE_MASTER ); break;
  case 2: simpleide_eject( LIBSPECTRUM_IDE_SLAVE  ); break;
  case 3: zxatasp_eject( LIBSPECTRUM_IDE_MASTER ); break;
  case 4: zxatasp_eject( LIBSPECTRUM_IDE_SLAVE  ); break;
  case 5: zxcf_eject(); break;
  case 6: divide_eject( LIBSPECTRUM_IDE_MASTER ); break;
  case 7: divide_eject( LIBSPECTRUM_IDE_SLAVE  ); break;
  }

  fuse_emulation_unpause();

  WIDGET_END;
}

MENU_CALLBACK( menu_media_ide_zxatasp_upload )
{
  settings_current.zxatasp_upload = !settings_current.zxatasp_upload;
  WIDGET_END;
}

MENU_CALLBACK( menu_media_ide_zxatasp_writeprotect )
{
  settings_current.zxatasp_wp = !settings_current.zxatasp_wp;
  WIDGET_END;
}

MENU_CALLBACK( menu_media_ide_zxcf_upload )
{
  settings_current.zxcf_upload = !settings_current.zxcf_upload;
  WIDGET_END;
}

MENU_CALLBACK( menu_media_ide_divide_writeprotect )
{
  settings_current.divide_wp = !settings_current.divide_wp;
  divide_refresh_page_state();
  WIDGET_END;
}

int
menu_open_snap( void )
{
  char *filename;
  int error;

  filename = menu_get_open_filename( "Fuse - Load Snapshot" );
  if( !filename ) return -1;

  error = snapshot_read( filename );
  free( filename );
  return error;
}

int
menu_check_media_changed( void )
{
  int confirm;

  confirm = tape_close(); if( confirm ) return 1;

#ifdef HAVE_765_H

  confirm = specplus3_disk_eject( SPECPLUS3_DRIVE_A, 0 );
  if( confirm ) return 1;

  confirm = specplus3_disk_eject( SPECPLUS3_DRIVE_B, 0 );
  if( confirm ) return 1;

#endif			/* #ifdef HAVE_765_H */

  confirm = beta_disk_eject( BETA_DRIVE_A, 0 );
  if( confirm ) return 1;

  confirm = beta_disk_eject( BETA_DRIVE_B, 0 );
  if( confirm ) return 1;

  confirm = beta_disk_eject( BETA_DRIVE_C, 0 );
  if( confirm ) return 1;

  confirm = beta_disk_eject( BETA_DRIVE_D, 0 );
  if( confirm ) return 1;

  confirm = plusd_disk_eject( PLUSD_DRIVE_1, 0 );
  if( confirm ) return 1;

  confirm = plusd_disk_eject( PLUSD_DRIVE_2, 0 );
  if( confirm ) return 1;

  if( settings_current.simpleide_master_file ) {
    confirm = simpleide_eject( LIBSPECTRUM_IDE_MASTER );
    if( confirm ) return 1;
  }

  if( settings_current.simpleide_slave_file ) {
    confirm = simpleide_eject( LIBSPECTRUM_IDE_SLAVE );
    if( confirm ) return 1;
  }

  if( settings_current.zxatasp_master_file ) {
    confirm = zxatasp_eject( LIBSPECTRUM_IDE_MASTER );
    if( confirm ) return 1;
  }

  if( settings_current.zxatasp_slave_file ) {
    confirm = zxatasp_eject( LIBSPECTRUM_IDE_SLAVE );
    if( confirm ) return 1;
  }

  if( settings_current.zxcf_pri_file ) {
    confirm = zxcf_eject(); if( confirm ) return 1;
  }

  if( settings_current.divide_master_file ) {
    confirm = divide_eject( LIBSPECTRUM_IDE_MASTER );
    if( confirm ) return 1;
  }

  if( settings_current.divide_slave_file ) {
    confirm = divide_eject( LIBSPECTRUM_IDE_SLAVE );
    if( confirm ) return 1;
  }

  return 0;
}
