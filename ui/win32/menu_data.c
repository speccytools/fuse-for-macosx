/* menu_data.c: menu structure for Fuse
   Copyright (c) 2004-2007 Philip Kendall, Stuart Brady, Marek Januszewski

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include "menu_data.h"
#include "../../menu.h"

void handle_menu( DWORD cmd, HWND okno )
{
  switch( cmd )
  {
    case IDM_MENU_FILE_OPEN:
      menu_file_open( 0 ); break;
    case IDM_MENU_FILE_SAVESNAPSHOT:
      menu_file_savesnapshot( 0 ); break;
    case IDM_MENU_FILE_RECORDING_RECORD:
      menu_file_recording_record( 0 ); break;
    case IDM_MENU_FILE_RECORDING_RECORDFROMSNAPSHOT:
      menu_file_recording_recordfromsnapshot( 0 ); break;
    case IDM_MENU_FILE_RECORDING_INSERTSNAPSHOT:
      menu_file_recording_insertsnapshot( 0 ); break;
    case IDM_MENU_FILE_RECORDING_ROLLBACK:
      menu_file_recording_rollback( 0 ); break;
    case IDM_MENU_FILE_RECORDING_ROLLBACKTO:
      menu_file_recording_rollbackto( 0 ); break;
    case IDM_MENU_FILE_RECORDING_PLAY:
      menu_file_recording_play( 0 ); break;
    case IDM_MENU_FILE_RECORDING_STOP:
      menu_file_recording_stop( 0 ); break;
    case IDM_MENU_FILE_AYLOGGING_RECORD:
      menu_file_aylogging_record( 0 ); break;
    case IDM_MENU_FILE_AYLOGGING_STOP:
      menu_file_aylogging_stop( 0 ); break;
    case IDM_MENU_FILE_OPENSCRSCREENSHOT:
      menu_file_openscrscreenshot( 0 ); break;
    case IDM_MENU_FILE_SAVESCREENASSCR:
      menu_file_savescreenasscr( 0 ); break;
    case IDM_MENU_FILE_SAVESCREENASPNG:
      menu_file_savescreenaspng( 0 ); break;
    case IDM_MENU_FILE_MOVIES_RECORDMOVIEASSCR:
      menu_file_movies_recordmovieasscr( 0 ); break;
    case IDM_MENU_FILE_MOVIES_RECORDMOVIEASPNG:
      menu_file_movies_recordmovieaspng( 0 ); break;
    case IDM_MENU_FILE_MOVIES_STOPMOVIERECORDING:
      menu_file_movies_stopmovierecording( 0 ); break;
    case IDM_MENU_FILE_LOADBINARYDATA:
      menu_file_loadbinarydata( 0 ); break;
    case IDM_MENU_FILE_SAVEBINARYDATA:
      menu_file_savebinarydata( 0 ); break;
    case IDM_MENU_FILE_EXIT:
      menu_file_exit( 0 ); break;
    case IDM_MENU_OPTIONS_GENERAL:
      menu_options_general( 0 ); break;
    case IDM_MENU_OPTIONS_SOUND:
      menu_options_sound( 0 ); break;
    case IDM_MENU_OPTIONS_PERIPHERALS:
      menu_options_peripherals( 0 ); break;
    case IDM_MENU_OPTIONS_RZX:
      menu_options_rzx( 0 ); break;
    case IDM_MENU_OPTIONS_JOYSTICKS_JOYSTICK1:
      menu_options_joysticks_select( 1 ); break;
    case IDM_MENU_OPTIONS_JOYSTICKS_JOYSTICK2:
      menu_options_joysticks_select( 2 ); break;
    case IDM_MENU_OPTIONS_JOYSTICKS_KEYBOARD:
      menu_options_joysticks_select( 3 ); break;
    case IDM_MENU_OPTIONS_SELECTROMS_SPECTRUM16K:
      menu_options_selectroms_select( 1 ); break;
    case IDM_MENU_OPTIONS_SELECTROMS_SPECTRUM48K:
      menu_options_selectroms_select( 2 ); break;
    case IDM_MENU_OPTIONS_SELECTROMS_SPECTRUM128K:
      menu_options_selectroms_select( 3 ); break;
    case IDM_MENU_OPTIONS_SELECTROMS_SPECTRUM2:
      menu_options_selectroms_select( 4 ); break;
    case IDM_MENU_OPTIONS_SELECTROMS_SPECTRUM2A:
      menu_options_selectroms_select( 5 ); break;
    case IDM_MENU_OPTIONS_SELECTROMS_SPECTRUM3:
      menu_options_selectroms_select( 6 ); break;
    case IDM_MENU_OPTIONS_SELECTROMS_SPECTRUM3E:
      menu_options_selectroms_select( 11 ); break;
    case IDM_MENU_OPTIONS_SELECTROMS_TIMEXTC2048:
      menu_options_selectroms_select( 7 ); break;
    case IDM_MENU_OPTIONS_SELECTROMS_TIMEXTC2068:
      menu_options_selectroms_select( 8 ); break;
    case IDM_MENU_OPTIONS_SELECTROMS_TIMEXTS2068:
      menu_options_selectroms_select( 14 ); break;
    case IDM_MENU_OPTIONS_SELECTROMS_PENTAGON128K:
      menu_options_selectroms_select( 9 ); break;
    case IDM_MENU_OPTIONS_SELECTROMS_SCORPIONZS256:
      menu_options_selectroms_select( 10 ); break;
    case IDM_MENU_OPTIONS_SELECTROMS_SPECTRUMSE:
      menu_options_selectroms_select( 12 ); break;
    case IDM_MENU_OPTIONS_SELECTROMS_INTERFACEI:
      menu_options_selectroms_select( 13 ); break;
    case IDM_MENU_OPTIONS_FILTER:
      menu_options_filter( 0 ); break;
    case IDM_MENU_MACHINE_PAUSE:
      menu_machine_pause( 0 ); break;
    case IDM_MENU_MACHINE_RESET:
      menu_machine_reset( 0 ); break;
    case IDM_MENU_MACHINE_HARDRESET:
      menu_machine_reset( 1 ); break;
    case IDM_MENU_MACHINE_SELECT:
      menu_machine_select( 0 ); break;
    case IDM_MENU_MACHINE_DEBUGGER:
      menu_machine_debugger( 0 ); break;
    case IDM_MENU_MACHINE_POKEFINDER:
      menu_machine_pokefinder( 0 ); break;
    case IDM_MENU_MACHINE_MEMORYBROWSER:
      menu_machine_memorybrowser( 0 ); break;
    case IDM_MENU_MACHINE_PROFILER_START:
      menu_machine_profiler_start( 0 ); break;
    case IDM_MENU_MACHINE_PROFILER_STOP:
      menu_machine_profiler_stop( 0 ); break;
    case IDM_MENU_MACHINE_NMI:
      menu_machine_nmi( 0 ); break;
    case IDM_MENU_MEDIA_TAPE_OPEN:
      menu_media_tape_open( 0 ); break;
    case IDM_MENU_MEDIA_TAPE_PLAY:
      menu_media_tape_play( 0 ); break;
    case IDM_MENU_MEDIA_TAPE_BROWSE:
      menu_media_tape_browse( 0 ); break;
    case IDM_MENU_MEDIA_TAPE_REWIND:
      menu_media_tape_rewind( 0 ); break;
    case IDM_MENU_MEDIA_TAPE_CLEAR:
      menu_media_tape_clear( 0 ); break;
    case IDM_MENU_MEDIA_TAPE_WRITE:
      menu_media_tape_write( 0 ); break;
    case IDM_MENU_MEDIA_TAPE_RECORDSTART:
      menu_media_tape_recordstart( 0 ); break;
    case IDM_MENU_MEDIA_TAPE_RECORDSTOP:
      menu_media_tape_recordstop( 0 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE1_INSERTNEW:
      menu_media_mdr_new( 1 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE1_INSERT:
      menu_media_mdr_insert( 1 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE1_SYNC:
      menu_media_mdr_sync( 1 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE1_EJECT:
      menu_media_mdr_eject( 1 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE1_WRITEPROTECT_SET:
      menu_media_mdr_writep( 0x11 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE1_WRITEPROTECT_REMOVE:
      menu_media_mdr_writep( 0x01 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE2_INSERTNEW:
      menu_media_mdr_new( 2 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE2_INSERT:
      menu_media_mdr_insert( 2 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE2_SYNC:
      menu_media_mdr_sync( 2 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE2_EJECT:
      menu_media_mdr_eject( 2 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE2_WRITEPROTECT_SET:
      menu_media_mdr_writep( 0x12 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE2_WRITEPROTECT_REMOVE:
      menu_media_mdr_writep( 0x02 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE3_INSERTNEW:
      menu_media_mdr_new( 3 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE3_INSERT:
      menu_media_mdr_insert( 3 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE3_SYNC:
      menu_media_mdr_sync( 3 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE3_EJECT:
      menu_media_mdr_eject( 3 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE3_WRITEPROTECT_SET:
      menu_media_mdr_writep( 0x13 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE3_WRITEPROTECT_REMOVE:
      menu_media_mdr_writep( 0x03 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE4_INSERTNEW:
      menu_media_mdr_new( 4 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE4_INSERT:
      menu_media_mdr_insert( 4 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE4_SYNC:
      menu_media_mdr_sync( 4 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE4_EJECT:
      menu_media_mdr_eject( 4 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE4_WRITEPROTECT_SET:
      menu_media_mdr_writep( 0x14 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE4_WRITEPROTECT_REMOVE:
      menu_media_mdr_writep( 0x04 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE5_INSERTNEW:
      menu_media_mdr_new( 5 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE5_INSERT:
      menu_media_mdr_insert( 5 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE5_SYNC:
      menu_media_mdr_sync( 5 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE5_EJECT:
      menu_media_mdr_eject( 5 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE5_WRITEPROTECT_SET:
      menu_media_mdr_writep( 0x15 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE5_WRITEPROTECT_REMOVE:
      menu_media_mdr_writep( 0x05 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE6_INSERTNEW:
      menu_media_mdr_new( 6 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE6_INSERT:
      menu_media_mdr_insert( 6 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE6_SYNC:
      menu_media_mdr_sync( 6 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE6_EJECT:
      menu_media_mdr_eject( 6 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE6_WRITEPROTECT_SET:
      menu_media_mdr_writep( 0x16 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE6_WRITEPROTECT_REMOVE:
      menu_media_mdr_writep( 0x06 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE7_INSERTNEW:
      menu_media_mdr_new( 7 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE7_INSERT:
      menu_media_mdr_insert( 7 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE7_SYNC:
      menu_media_mdr_sync( 7 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE7_EJECT:
      menu_media_mdr_eject( 7 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE7_WRITEPROTECT_SET:
      menu_media_mdr_writep( 0x17 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE7_WRITEPROTECT_REMOVE:
      menu_media_mdr_writep( 0x07 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE8_INSERTNEW:
      menu_media_mdr_new( 8 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE8_INSERT:
      menu_media_mdr_insert( 8 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE8_SYNC:
      menu_media_mdr_sync( 8 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE8_EJECT:
      menu_media_mdr_eject( 8 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE8_WRITEPROTECT_SET:
      menu_media_mdr_writep( 0x18 ); break;
    case IDM_MENU_MEDIA_INTERFACEI_MICRODRIVE8_WRITEPROTECT_REMOVE:
      menu_media_mdr_writep( 0x08 ); break;
    case IDM_MENU_MEDIA_DISK_3_DRIVEA_INSERT:
      menu_media_disk_insert( 1 ); break;
    case IDM_MENU_MEDIA_DISK_3_DRIVEA_EJECT:
      menu_media_disk_eject( 1 ); break;
    case IDM_MENU_MEDIA_DISK_3_DRIVEA_EJECTANDWRITE:
      menu_media_disk_eject( 17 ); break;
    case IDM_MENU_MEDIA_DISK_3_DRIVEB_INSERT:
      menu_media_disk_insert( 2 ); break;
    case IDM_MENU_MEDIA_DISK_3_DRIVEB_EJECT:
      menu_media_disk_eject( 2 ); break;
    case IDM_MENU_MEDIA_DISK_3_DRIVEB_EJECTANDWRITE:
      menu_media_disk_eject( 18 ); break;
    case IDM_MENU_MEDIA_DISK_TRDOS_DRIVEA_INSERT:
      menu_media_disk_insert( 5 ); break;
    case IDM_MENU_MEDIA_DISK_TRDOS_DRIVEA_EJECT:
      menu_media_disk_eject( 5 ); break;
    case IDM_MENU_MEDIA_DISK_TRDOS_DRIVEA_EJECTANDWRITE:
      menu_media_disk_eject( 21 ); break;
    case IDM_MENU_MEDIA_DISK_TRDOS_DRIVEB_INSERT:
      menu_media_disk_insert( 6 ); break;
    case IDM_MENU_MEDIA_DISK_TRDOS_DRIVEB_EJECT:
      menu_media_disk_eject( 6 ); break;
    case IDM_MENU_MEDIA_DISK_TRDOS_DRIVEB_EJECTANDWRITE:
      menu_media_disk_eject( 22 ); break;
    case IDM_MENU_MEDIA_DISK_D_DRIVE1_INSERT:
      menu_media_disk_insert( 9 ); break;
    case IDM_MENU_MEDIA_DISK_D_DRIVE1_EJECT:
      menu_media_disk_eject( 9 ); break;
    case IDM_MENU_MEDIA_DISK_D_DRIVE1_EJECTANDWRITE:
      menu_media_disk_eject( 25 ); break;
    case IDM_MENU_MEDIA_DISK_D_DRIVE2_INSERT:
      menu_media_disk_insert( 10 ); break;
    case IDM_MENU_MEDIA_DISK_D_DRIVE2_EJECT:
      menu_media_disk_eject( 10 ); break;
    case IDM_MENU_MEDIA_DISK_D_DRIVE2_EJECTANDWRITE:
      menu_media_disk_eject( 26 ); break;
    case IDM_MENU_MEDIA_CARTRIDGE_TIMEXDOCK_INSERT:
      menu_media_cartridge_timexdock_insert( 0 ); break;
    case IDM_MENU_MEDIA_CARTRIDGE_TIMEXDOCK_EJECT:
      menu_media_cartridge_timexdock_eject( 0 ); break;
    case IDM_MENU_MEDIA_CARTRIDGE_INTERFACEII_INSERT:
      menu_media_cartridge_interfaceii_insert( 0 ); break;
    case IDM_MENU_MEDIA_CARTRIDGE_INTERFACEII_EJECT:
      menu_media_cartridge_interfaceii_eject( 0 ); break;
    case IDM_MENU_MEDIA_IDE_SIMPLE8BIT_MASTER_INSERT:
      menu_media_ide_insert( 1 ); break;
    case IDM_MENU_MEDIA_IDE_SIMPLE8BIT_MASTER_COMMIT:
      menu_media_ide_commit( 1 ); break;
    case IDM_MENU_MEDIA_IDE_SIMPLE8BIT_MASTER_EJECT:
      menu_media_ide_eject( 1 ); break;
    case IDM_MENU_MEDIA_IDE_SIMPLE8BIT_SLAVE_INSERT:
      menu_media_ide_insert( 2 ); break;
    case IDM_MENU_MEDIA_IDE_SIMPLE8BIT_SLAVE_COMMIT:
      menu_media_ide_commit( 2 ); break;
    case IDM_MENU_MEDIA_IDE_SIMPLE8BIT_SLAVE_EJECT:
      menu_media_ide_eject( 2 ); break;
    case IDM_MENU_MEDIA_IDE_ZXATASP_MASTER_INSERT:
      menu_media_ide_insert( 3 ); break;
    case IDM_MENU_MEDIA_IDE_ZXATASP_MASTER_COMMIT:
      menu_media_ide_commit( 3 ); break;
    case IDM_MENU_MEDIA_IDE_ZXATASP_MASTER_EJECT:
      menu_media_ide_eject( 3 ); break;
    case IDM_MENU_MEDIA_IDE_ZXATASP_SLAVE_INSERT:
      menu_media_ide_insert( 4 ); break;
    case IDM_MENU_MEDIA_IDE_ZXATASP_SLAVE_COMMIT:
      menu_media_ide_commit( 4 ); break;
    case IDM_MENU_MEDIA_IDE_ZXATASP_SLAVE_EJECT:
      menu_media_ide_eject( 4 ); break;
    case IDM_MENU_MEDIA_IDE_ZXCFCOMPACTFLASH_INSERT:
      menu_media_ide_insert( 5 ); break;
    case IDM_MENU_MEDIA_IDE_ZXCFCOMPACTFLASH_COMMIT:
      menu_media_ide_commit( 5 ); break;
    case IDM_MENU_MEDIA_IDE_ZXCFCOMPACTFLASH_EJECT:
      menu_media_ide_eject( 5 ); break;
    case IDM_MENU_MEDIA_IDE_DIVIDE_MASTER_INSERT:
      menu_media_ide_insert( 6 ); break;
    case IDM_MENU_MEDIA_IDE_DIVIDE_MASTER_COMMIT:
      menu_media_ide_commit( 6 ); break;
    case IDM_MENU_MEDIA_IDE_DIVIDE_MASTER_EJECT:
      menu_media_ide_eject( 6 ); break;
    case IDM_MENU_MEDIA_IDE_DIVIDE_SLAVE_INSERT:
      menu_media_ide_insert( 7 ); break;
    case IDM_MENU_MEDIA_IDE_DIVIDE_SLAVE_COMMIT:
      menu_media_ide_commit( 7 ); break;
    case IDM_MENU_MEDIA_IDE_DIVIDE_SLAVE_EJECT:
      menu_media_ide_eject( 7 ); break;
    case IDM_MENU_HELP_KEYBOARD:
      menu_help_keyboard( 0 ); break;
  }
}
