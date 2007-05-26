/* menu_data.c: menu structure for Fuse
   Copyright (c) 2004 Marek Januszewski

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

void handle_menu( DWORD cmd, HWND okno )
{
  switch( cmd )
  {
    case IDM_MENU_FILE_OPEN: 
      menu_file_open( 0 ); break; 
    case IDM_MENU_FILE_SAVE_SNAPSHOT: 
      menu_file_savesnapshot( 0 ); break; 
    case IDM_MENU_FILE_RECORDING_RECORD: 
      menu_file_recording_record( 0 ); break; 
    case IDM_MENU_FILE_RECORDING_RECORD_FROM_SNAPSHOT: 
      menu_file_recording_recordfromsnapshot( 0 ); break;
    case IDM_MENU_FILE_RECORDING_PLAY: 
      menu_file_recording_play( 0 ); break; 
    case IDM_MENU_FILE_RECORDING_STOP: 
        menu_file_recording_stop( 0 ); break; 
    case IDM_MENU_FILE_AY_LOGGING_RECORD: 
        menu_file_aylogging_record( 0 ); break; 
    case IDM_MENU_FILE_AY_LOGGING_STOP: 
        menu_file_aylogging_stop( 0 ); break;
    case IDM_MENU_FILE_OPEN_SCR_SCREENSHOT: 
        menu_file_openscrscreenshot( 0 ); break;
    case IDM_MENU_FILE_SAVE_SCREEN_AS_SCR: 
        menu_file_savescreenasscr( 0 ); break;
    case IDM_MENU_FILE_SAVE_SCREEN_AS_PNG: 
        menu_file_savescreenaspng( 0 ); break;
    case IDM_MENU_FILE_LOAD_BINARY_DATA: 
/* TODO: 
        menu_file_loadbinarydata( 0 ); */ break;
    case IDM_MENU_FILE_SAVE_BINARY_DATA: 
/* TODO: 
        menu_file_savebinarydata( 0 ); */ break;
    case IDM_MENU_FILE_EXIT: 
        menu_file_exit( 0 ); break;
    case IDM_MENU_OPTIONS_GENERAL: 
        menu_options_general( 0 ); break;
    case IDM_MENU_OPTIONS_SOUND: 
        menu_options_sound( 0 ); break;
    case IDM_MENU_OPTIONS_PERIPHERIALS: 
        menu_options_peripherals( 0 ); break;
    case IDM_MENU_OPTIONS_RZX: 
        menu_options_rzx( 0 ); break;
    case IDM_MENU_OPTIONS_JOYSTICKS_JOYSTICK_1: 
        menu_options_joysticks_select( 1 ); break;
    case IDM_MENU_OPTIONS_JOYSTICKS_JOYSTICK_2: 
        menu_options_joysticks_select( 2 ); break;
    case IDM_MENU_OPTIONS_JOYSTICKS_KEYBOARD: 
        menu_options_joysticks_select( 3 ); break;
    case IDM_MENU_OPTIONS_SELECT_ROMS_SPECTRUM_16K: 
        menu_options_selectroms_select( 1 ); break;
    case IDM_MENU_OPTIONS_SELECT_ROMS_SPECTRUM_48K: 
        menu_options_selectroms_select( 2 ); break;
    case IDM_MENU_OPTIONS_SELECT_ROMS_SPECTRUM_128K: 
        menu_options_selectroms_select( 3 ); break;
    case IDM_MENU_OPTIONS_SELECT_ROMS_SPECTRUM_2: 
        menu_options_selectroms_select( 4 ); break;
    case IDM_MENU_OPTIONS_SELECT_ROMS_SPECTRUM_2A: 
        menu_options_selectroms_select( 5 ); break;
    case IDM_MENU_OPTIONS_SELECT_ROMS_SPECTRUM_3: 
        menu_options_selectroms_select( 6 ); break;
    case IDM_MENU_OPTIONS_SELECT_ROMS_SPECTRUM_3E: 
        menu_options_selectroms_select( 11 ); break;
    case IDM_MENU_OPTIONS_SELECT_ROMS_TIMEX_TC2048: 
        menu_options_selectroms_select( 7 ); break;
    case IDM_MENU_OPTIONS_SELECT_ROMS_TIMEX_TC2068: 
        menu_options_selectroms_select( 8 ); break;
    case IDM_MENU_OPTIONS_SELECT_ROMS_PENTAGON_128K: 
        menu_options_selectroms_select( 9 ); break;
    case IDM_MENU_OPTIONS_SELECT_ROMS_SCORPION_ZS_256: 
        menu_options_selectroms_select( 10 ); break;
    case IDM_MENU_OPTIONS_SELECT_ROMS_SPECTRUM_SE: 
        menu_options_selectroms_select( 12 ); break;
    case IDM_MENU_OPTIONS_FILTER: 
        menu_options_filter( 0 ); break;
    case IDM_MENU_OPTIONS_SAVE: 
        menu_options_save( 0 ); break;
    case IDM_MENU_MACHINE_PAUSE: 
/* TODO:
        menu_machine_pause( 0 ); */ break;
    case IDM_MENU_MACHINE_RESET: 
        menu_machine_reset( 0 ); break;
    case IDM_MENU_MACHINE_SELECT: 
        menu_machine_select( 0 ); break;
    case IDM_MENU_MACHINE_DEBUGGER: 
        menu_machine_debugger( 0 ); break;
    case IDM_MENU_MACHINE_POKE_FINDER: 
        menu_machine_pokefinder( 0 ); break;
    case IDM_MENU_MACHINE_MEMORY_BROWSER: 
/* TODO:
        menu_machine_memorybrowser( 0 ); */ break;
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
    case IDM_MENU_MEDIA_DISK_DRIVE_A_INSERT: 
        menu_media_disk_insert( 1 ); break;
    case IDM_MENU_MEDIA_DISK_DRIVE_A_EJECT: 
        menu_media_disk_eject( 1 ); break;
    case IDM_MENU_MEDIA_DISK_DRIVE_A_EJECT_AND_WRITE: 
        menu_media_disk_eject( 3 ); break;
    case IDM_MENU_MEDIA_DISK_DRIVE_B_INSERT: 
        menu_media_disk_insert( 2 ); break;
    case IDM_MENU_MEDIA_DISK_DRIVE_B_EJECT: 
        menu_media_disk_eject( 2 ); break;
    case IDM_MENU_MEDIA_DISK_DRIVE_B_EJECT_AND_WRITE: 
        menu_media_disk_eject( 4 ); break;
    case IDM_MENU_MEDIA_CARTRIDGE_TIMEX_DOCK_INSERT: 
        menu_media_cartridge_timexdock_insert( 0 ); break;
    case IDM_MENU_MEDIA_CARTRIDGE_TIMEX_DOCK_EJECT: 
        menu_media_cartridge_timexdock_eject( 0 ); break;
    case IDM_MENU_MEDIA_CARTRIDGE_INTERFACE_II_INSERT: 
        menu_media_cartridge_interfaceii_insert( 0 ); break;
    case IDM_MENU_MEDIA_CARTRIDGE_INTERFACE_II_EJECT: 
        menu_media_cartridge_interfaceii_eject( 0 ); break;
    case IDM_MENU_MEDIA_IDE_SIMPLE_8_BIT_MASTER_INSERT: 
        menu_media_ide_insert( 1 ); break;
    case IDM_MENU_MEDIA_IDE_SIMPLE_8_BIT_MASTER_COMMIT: 
        menu_media_ide_commit( 1 ); break;
    case IDM_MENU_MEDIA_IDE_SIMPLE_8_BIT_MASTER_EJECT: 
        menu_media_ide_eject( 1 ); break;
    case IDM_MENU_MEDIA_IDE_SIMPLE_8_BIT_SLAVE_INSERT: 
        menu_media_ide_insert( 2 ); break;
    case IDM_MENU_MEDIA_IDE_SIMPLE_8_BIT_SLAVE_COMMIT: 
        menu_media_ide_commit( 2 ); break;
    case IDM_MENU_MEDIA_IDE_SIMPLE_8_BIT_SLAVE_EJECT: 
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
    case IDM_MENU_MEDIA_IDE_ZXCF_COMPACTFLASH_INSERT: 
        menu_media_ide_insert( 5 ); break;
    case IDM_MENU_MEDIA_IDE_ZXCF_COMPACTFLASH_COMMIT: 
        menu_media_ide_commit( 5 ); break;
    case IDM_MENU_MEDIA_IDE_ZXCF_COMPACTFLASH_EJECT: 
        menu_media_ide_eject( 5 ); break;
    case IDM_MENU_HELP_KEYBOARD: 
        menu_help_keyboard( 0 ); break;
  }    
}

