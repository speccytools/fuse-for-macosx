/* menu.c: general menu callbacks
   Copyright (c) 2004 Philip Kendall

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

   Philip Kendall <pak21-fuse@srcf.ucam.org>
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <libspectrum.h>

#include "dck.h"
#include "event.h"
#include "fuse.h"
#include "if2.h"
#include "menu.h"
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

  filename = menu_get_filename( "Fuse - Open Spectrum File" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  utils_open_file( filename, settings_current.auto_load, NULL );

  free( filename );

  display_refresh_all();

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_recording_play )
{
  char *recording;

  if( rzx_playback || rzx_recording ) return;

  fuse_emulation_pause();

  recording = menu_get_filename( "Fuse - Start Replay" );
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

  filename = menu_get_filename( "Fuse - Open SCR Screenshot" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  screenshot_scr_read( filename );

  free( filename );

  fuse_emulation_unpause();
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

MENU_CALLBACK( menu_machine_nmi )
{
  WIDGET_END;
  event_add( 0, EVENT_TYPE_NMI );
}

MENU_CALLBACK( menu_media_tape_open )
{
  char *filename;

  fuse_emulation_pause();

  filename = menu_get_filename( "Fuse - Open Tape" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  tape_open_default_autoload( filename, NULL );

  free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_media_tape_play )
{
  WIDGET_END;
  tape_toggle_play();
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

MENU_CALLBACK_WITH_ACTION( menu_media_disk_insert )
{
  char *filename;
  int which;

  which = action - 1;

  fuse_emulation_pause();

  filename = menu_get_filename( "Fuse - Insert disk" );
  if( !filename ) { fuse_emulation_unpause(); return; }

#ifdef HAVE_765_H
  if( machine_current->machine == LIBSPECTRUM_MACHINE_PLUS3 ) {
    specplus3_disk_insert( which, filename );
  } else
#endif				/* #ifdef HAVE_765_H */
    trdos_disk_insert( which, filename );

  free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK_WITH_ACTION( menu_media_disk_eject )
{
  int which, write;

  WIDGET_END;

  action--;
  which = action & 0x01;
  write = action & 0x02;

#ifdef HAVE_765_H
  if( machine_current->capabilities &
      LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_DISK ) {
    specplus3_disk_eject( which, write );
    return;
  }
#endif			/* #ifdef HAVE_765_H */

  trdos_disk_eject( which, write );
}

MENU_CALLBACK( menu_media_cartridge_timexdock_insert )
{
  char *filename;

  fuse_emulation_pause();

  filename = menu_get_filename( "Fuse - Insert Timex Dock Cartridge" );
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

  filename = menu_get_filename( "Fuse - Insert Interface II Cartridge" );
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

  filename = menu_get_filename( "Fuse - Insert hard disk file" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  switch( action ) {
  case 1: simpleide_insert( filename, LIBSPECTRUM_IDE_MASTER ); break;
  case 2: simpleide_insert( filename, LIBSPECTRUM_IDE_SLAVE  ); break;
  case 3: zxatasp_insert( filename, LIBSPECTRUM_IDE_MASTER ); break;
  case 4: zxatasp_insert( filename, LIBSPECTRUM_IDE_SLAVE  ); break;
  case 5: zxcf_insert( filename ); break;
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

int
menu_open_snap( void )
{
  char *filename;
  int error;

  filename = menu_get_filename( "Fuse - Load Snapshot" );
  if( !filename ) return -1;

  error = snapshot_read( filename );
  free( filename );
  return error;
}