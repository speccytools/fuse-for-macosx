/* snapshot.c: snapshot handling routines
   Copyright (c) 1999-2004 Philip Kendall

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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libspectrum.h>

#include "dck.h"
#include "display.h"
#include "fuse.h"
#include "if2.h"
#include "machine.h"
#include "machines/scorpion.h"
#include "machines/spec128.h"
#include "memory.h"
#include "scld.h"
#include "settings.h"
#include "snapshot.h"
#include "sound.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "utils.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"
#include "zxatasp.h"
#include "zxcf.h"

int snapshot_read( const char *filename )
{
  utils_file file;
  libspectrum_snap *snap;
  int error;

  error = libspectrum_snap_alloc( &snap ); if( error ) return error;

  error = utils_read_file( filename, &file );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  error = libspectrum_snap_read( snap, file.buffer, file.length,
				 LIBSPECTRUM_ID_UNKNOWN, filename );
  if( error ) {
    utils_close_file( &file ); libspectrum_snap_free( snap );
    return error;
  }

  if( utils_close_file( &file ) ) {
    libspectrum_snap_free( snap );
    return 1;
  }

  error = snapshot_copy_from( snap );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  error = libspectrum_snap_free( snap ); if( error ) return error;

  return 0;
}

int
snapshot_read_buffer( const unsigned char *buffer, size_t length,
		      libspectrum_id_t type )
{
  libspectrum_snap *snap; int error;

  error = libspectrum_snap_alloc( &snap ); if( error ) return error;

  error = libspectrum_snap_read( snap, buffer, length, type, NULL );
  if( error ) { libspectrum_snap_free( snap ); return error; }
    
  error = snapshot_copy_from( snap );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  error = libspectrum_snap_free( snap ); if( error ) return error;

  return 0;
}

static int
try_fallback_machine( libspectrum_machine machine )
{
  int error;

  /* If we failed on a +3 snapshot, try falling back to +2A (in case
     we were compiled without lib765) */
  if( machine == LIBSPECTRUM_MACHINE_PLUS3 ) {

    error = machine_select( LIBSPECTRUM_MACHINE_PLUS2A );
    if( error ) {
      ui_error( UI_ERROR_ERROR,
		"Loading a %s snapshot, but neither that nor %s available",
		libspectrum_machine_name( machine ),
		libspectrum_machine_name( LIBSPECTRUM_MACHINE_PLUS2A )    );
      return 1;
    } else {
      ui_error( UI_ERROR_WARNING,
		"Loading a %s snapshot, but that's not available. "
		"Using %s instead",
		libspectrum_machine_name( machine ),
		libspectrum_machine_name( LIBSPECTRUM_MACHINE_PLUS2A )  );
    }

  } else {			/* Not trying a +3 snapshot */
    ui_error( UI_ERROR_ERROR,
	      "Loading a %s snapshot, but that's not available",
	      libspectrum_machine_name( machine ) );
  }
  
  return 0;
}

static int
copy_z80_from( libspectrum_snap *snap )
{
  A  = libspectrum_snap_a ( snap ); F  = libspectrum_snap_f ( snap );
  A_ = libspectrum_snap_a_( snap ); F_ = libspectrum_snap_f_( snap );

  BC  = libspectrum_snap_bc ( snap ); DE  = libspectrum_snap_de ( snap );
  HL  = libspectrum_snap_hl ( snap ); BC_ = libspectrum_snap_bc_( snap );
  DE_ = libspectrum_snap_de_( snap ); HL_ = libspectrum_snap_hl_( snap );

  IX = libspectrum_snap_ix( snap ); IY = libspectrum_snap_iy( snap );
  I  = libspectrum_snap_i ( snap ); R   = libspectrum_snap_r( snap );
  SP = libspectrum_snap_sp( snap ); PC = libspectrum_snap_pc( snap );

  IFF1 = libspectrum_snap_iff1( snap ); IFF2 = libspectrum_snap_iff2( snap );
  IM = libspectrum_snap_im( snap );

  z80.halted = libspectrum_snap_halted( snap );

  z80.interrupts_enabled_at =
    libspectrum_snap_last_instruction_ei( snap ) ? tstates : -1;

  return 0;
}

static int
copy_ula_from( libspectrum_snap *snap )
{
  spectrum_ula_write( 0x00fe, libspectrum_snap_out_ula( snap ) );
  tstates = libspectrum_snap_tstates( snap );

  return 0;
}

static int
copy_128k_from( libspectrum_snap *snap, int capabilities )
{
  size_t i;

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY )
    spec128_memoryport_write( 0x7ffd,
			      libspectrum_snap_out_128_memoryport( snap ) );

  if( ( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY ) ||
      ( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SCORP_MEMORY )    )
    specplus3_memoryport2_write(
      0x1ffd, libspectrum_snap_out_plus3_memoryport( snap )
    );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_AY ) {

    ay_registerport_write( 0xfffd,
			   libspectrum_snap_out_ay_registerport( snap ) );

    for( i=0; i<16; i++ ) {
      machine_current->ay.registers[i] =
	libspectrum_snap_ay_registers( snap, i );
      sound_ay_write( i, machine_current->ay.registers[i], 0 );
    }

  }

  return 0;
}

static int
copy_betadisk_from( libspectrum_snap *snap, int capabilities )
{
  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TRDOS_DISK ) {

    trdos_active = libspectrum_snap_beta_paged( snap );
  
    if( trdos_active ) {
      trdos_page();
    } else {
      trdos_unpage();
    }

    trdos_direction = libspectrum_snap_beta_direction( snap );

    trdos_cr_write ( 0x001f, libspectrum_snap_beta_status( snap ) );
    trdos_tr_write ( 0x003f, libspectrum_snap_beta_track ( snap ) );
    trdos_sec_write( 0x005f, libspectrum_snap_beta_sector( snap ) );
    trdos_dr_write ( 0x007f, libspectrum_snap_beta_data  ( snap ) );
    trdos_sp_write ( 0x00ff, libspectrum_snap_beta_system( snap ) );

  }

  return 0;
}

static int
copy_ram_from( libspectrum_snap *snap )
{
  size_t i;

  for( i = 0; i < 16; i++ )
    if( libspectrum_snap_pages( snap, i ) )
      memcpy( RAM[i], libspectrum_snap_pages( snap, i ), 0x4000 );

  return 0;
}

static int
copy_slt_from( libspectrum_snap *snap )
{
  size_t i;

  for( i=0; i<256; i++ ) {

    slt_length[i] = libspectrum_snap_slt_length( snap, i );

    if( slt_length[i] ) {

      slt[i] = memory_pool_allocate( slt_length[i] *
				     sizeof( libspectrum_byte ) );
      if( !slt[i] ) {
	ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
		  __LINE__ );
	return 1;
      }

      memcpy( slt[i], libspectrum_snap_slt( snap, i ),
	      libspectrum_snap_slt_length( snap, i ) );
    }
  }

  if( libspectrum_snap_slt_screen( snap ) ) {

    slt_screen = memory_pool_allocate( 6912 * sizeof( libspectrum_byte ) );
    if( !slt_screen ) {
      ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
      return 1;
    }

    memcpy( slt_screen, libspectrum_snap_slt_screen( snap ), 6912 );
    slt_screen_level = libspectrum_snap_slt_screen_level( snap );
  }

  return 0;
}

static int
copy_zxatasp_from( libspectrum_snap *snap )
{
  size_t i;

  if( libspectrum_snap_zxatasp_active( snap ) ) {

    libspectrum_byte portA, portB, portC, control;
    size_t page;

    settings_current.zxatasp_active = 1;
    settings_current.zxatasp_upload = libspectrum_snap_zxatasp_upload( snap );
    settings_current.zxatasp_wp =
      libspectrum_snap_zxatasp_writeprotect( snap );

    portA   = libspectrum_snap_zxatasp_port_a ( snap );
    portB   = libspectrum_snap_zxatasp_port_b ( snap );
    portC   = libspectrum_snap_zxatasp_port_c ( snap );
    control = libspectrum_snap_zxatasp_control( snap );
    page    = libspectrum_snap_zxatasp_current_page( snap );

    zxatasp_restore( portA, portB, portC, control, page );

    for( i = 0; i < libspectrum_snap_zxatasp_pages( snap ); i++ )
      if( libspectrum_snap_zxatasp_ram( snap, i ) )
	memcpy( zxatasp_ram( i ), libspectrum_snap_zxatasp_ram( snap, i ),
		0x4000 );
  }

  return 0;
}

static int
copy_zxcf_from( libspectrum_snap *snap )
{
  size_t i;

  if( libspectrum_snap_zxcf_active( snap ) ) {

    settings_current.zxcf_active = 1;
    settings_current.zxcf_upload = libspectrum_snap_zxcf_upload( snap );

    zxcf_memctl_write( 0x10bf, libspectrum_snap_zxcf_memctl( snap ) );

    for( i = 0; i < libspectrum_snap_zxcf_pages( snap ); i++ )
      if( libspectrum_snap_zxcf_ram( snap, i ) )
	memcpy( zxcf_ram( i ), libspectrum_snap_zxcf_ram( snap, i ), 0x4000 );
  }
  
  return 0;
}

static int
copy_if2_from( libspectrum_snap *snap )
{
  if( libspectrum_snap_interface2_active( snap ) ) {

    if2_active = 1;
    machine_current->ram.romcs = 1;

    if( libspectrum_snap_interface2_rom( snap, 0 ) ) {
      memory_map_romcs[0].offset = 0;
      memory_map_romcs[0].page_num = 0;
      memory_map_romcs[0].page = memory_pool_allocate(
                                  2 * MEMORY_PAGE_SIZE *
                                    sizeof( libspectrum_byte ) );
      if( !memory_map_romcs[0].page ) {
        ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
        return 1;
      }

      memcpy( memory_map_romcs[0].page,
              libspectrum_snap_interface2_rom( snap, 0 ),
              2 * MEMORY_PAGE_SIZE );

      memory_map_romcs[1].offset = MEMORY_PAGE_SIZE;
      memory_map_romcs[1].page_num = 0;
      memory_map_romcs[1].page = memory_map_romcs[0].page + MEMORY_PAGE_SIZE;
      memory_map_romcs[1].source =
        memory_map_romcs[0].source = MEMORY_SOURCE_CARTRIDGE;
    }

    ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2_EJECT, 1 );

    machine_current->memory_map();
  }

  return 0;
}

static int
copy_timex_from( libspectrum_snap *snap, int capabilities )
{
  size_t i;

  if( capabilities & ( LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY |
      LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY ) )
    scld_hsr_write( 0x00fd, libspectrum_snap_out_scld_hsr( snap ) );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_VIDEO )
    scld_dec_write( 0x00ff, libspectrum_snap_out_scld_dec( snap ) );

  if( libspectrum_snap_dock_active( snap ) ) {

    dck_active = 1;

    for( i = 0; i < 8; i++ ) {

      if( libspectrum_snap_dock_cart( snap, i ) ) {
        if( !memory_map_dock[i]->page ) {
          memory_map_dock[i]->offset = 0;
          memory_map_dock[i]->page_num = 0;
          memory_map_dock[i]->writable = libspectrum_snap_dock_ram( snap, i );
          memory_map_dock[i]->source = MEMORY_SOURCE_CARTRIDGE;
          memory_map_dock[i]->page = memory_pool_allocate(
                                      MEMORY_PAGE_SIZE *
                                      sizeof( libspectrum_byte ) );
          if( !memory_map_dock[i]->page ) {
            ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
                      __LINE__ );
            return 1;
          }
        }

        memcpy( memory_map_dock[i]->page, libspectrum_snap_dock_cart( snap, i ),
                MEMORY_PAGE_SIZE );
      }

      if( libspectrum_snap_exrom_cart( snap, i ) ) {
        if( !memory_map_dock[i]->page ) {
          memory_map_exrom[i]->offset = 0;
          memory_map_exrom[i]->page_num = 0;
          memory_map_exrom[i]->writable = libspectrum_snap_exrom_ram( snap, i );
          memory_map_exrom[i]->source = MEMORY_SOURCE_CARTRIDGE;
          memory_map_exrom[i]->page = memory_pool_allocate(
                                      MEMORY_PAGE_SIZE *
                                      sizeof( libspectrum_byte ) );
          if( !memory_map_exrom[i]->page ) {
            ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
                      __LINE__ );
            return 1;
          }
        }

        memcpy( memory_map_exrom[i]->page,
                libspectrum_snap_exrom_cart( snap, i ), MEMORY_PAGE_SIZE );
      }

    }

    if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_DOCK )
      ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_DOCK_EJECT, 1 );

    machine_current->memory_map();
  }

  return 0;
}

int
snapshot_copy_from( libspectrum_snap *snap )
{
  int capabilities, error;
  libspectrum_machine machine;

  machine = libspectrum_snap_machine( snap );

  error = machine_select( machine );
  if( error ) {
    error = try_fallback_machine( machine ); if( error ) return error;
  }

  capabilities = machine_current->capabilities;

  error = copy_z80_from( snap ); if( error ) return error;
  error = copy_ula_from( snap ); if( error ) return error;
  error = copy_128k_from( snap, capabilities ); if( error ) return error;
  error = copy_betadisk_from( snap, capabilities ); if( error ) return error;
  error = copy_ram_from( snap ); if( error ) return error;
  error = copy_slt_from( snap ); if( error ) return error;
  error = copy_zxatasp_from( snap ); if( error ) return error;
  error = copy_zxcf_from( snap ); if( error ) return error;
  error = copy_if2_from( snap ); if( error ) return error;
  error = copy_timex_from( snap, capabilities ); if( error ) return error;

  return 0;
}

int snapshot_write( const char *filename )
{
  libspectrum_id_t type;
  libspectrum_class_t class;
  libspectrum_snap *snap;
  unsigned char *buffer; size_t length;
  int flags;

  int error;

  /* Work out what sort of file we want from the filename; default to
     .z80 if we couldn't guess */
  error = libspectrum_identify_file_with_class( &type, &class, filename, NULL,
						0 );
  if( error ) return error;

  if( class != LIBSPECTRUM_CLASS_SNAPSHOT || type == LIBSPECTRUM_ID_UNKNOWN )
    type = LIBSPECTRUM_ID_SNAPSHOT_Z80;

  error = libspectrum_snap_alloc( &snap ); if( error ) return error;

  error = snapshot_copy_to( snap );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  flags = 0;
  length = 0;
  error = libspectrum_snap_write( &buffer, &length, &flags, snap, type,
				  fuse_creator, 0 );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  if( flags & LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS ) {
    ui_error(
      UI_ERROR_WARNING,
      "A large amount of information has been lost in conversion; the snapshot probably won't work"
    );
  } else if( flags & LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS ) {
    ui_error(
      UI_ERROR_WARNING,
      "Some information has been lost in conversion; the snapshot may not work"
    );
  }

  error = libspectrum_snap_free( snap );
  if( error ) { free( buffer ); return 1; }

  error = utils_write_file( filename, buffer, length );
  if( error ) { free( buffer ); return error; }

  return 0;

}

static int
copy_z80_to( libspectrum_snap *snap )
{
  libspectrum_snap_set_a  ( snap, A   ); libspectrum_snap_set_f  ( snap, F   );
  libspectrum_snap_set_a_ ( snap, A_  ); libspectrum_snap_set_f_ ( snap, F_  );

  libspectrum_snap_set_bc ( snap, BC  ); libspectrum_snap_set_de ( snap, DE  );
  libspectrum_snap_set_hl ( snap, HL  ); libspectrum_snap_set_bc_( snap, BC_ );
  libspectrum_snap_set_de_( snap, DE_ ); libspectrum_snap_set_hl_( snap, HL_ );

  libspectrum_snap_set_ix ( snap, IX  ); libspectrum_snap_set_iy ( snap, IY  );
  libspectrum_snap_set_i  ( snap, I   ); libspectrum_snap_set_r  ( snap, R   );
  libspectrum_snap_set_sp ( snap, SP  ); libspectrum_snap_set_pc ( snap, PC  );

  libspectrum_snap_set_iff1( snap, IFF1 );
  libspectrum_snap_set_iff2( snap, IFF2 );
  libspectrum_snap_set_im( snap, IM );

  libspectrum_snap_set_halted( snap, z80.halted );
  libspectrum_snap_set_last_instruction_ei(
    snap, z80.interrupts_enabled_at == tstates
  );

  return 0;
}

static int
copy_ula_to( libspectrum_snap *snap )
{
  libspectrum_snap_set_out_ula( snap, spectrum_last_ula );
  libspectrum_snap_set_tstates( snap, tstates );

  return 0;
}

static int
copy_128k_to( libspectrum_snap *snap )
{
  size_t i;

  /* These won't necessarily be valid in some machine configurations, but
     this shouldn't cause anything to go wrong */
  libspectrum_snap_set_out_128_memoryport( snap,
					   machine_current->ram.last_byte );
  libspectrum_snap_set_out_ay_registerport(
    snap, machine_current->ay.current_register
  );

  for( i=0; i<16; i++ )
    libspectrum_snap_set_ay_registers(
      snap, i, machine_current->ay.registers[i]
    );

  libspectrum_snap_set_out_plus3_memoryport( snap,
					     machine_current->ram.last_byte2 );

  return 0;
}

static int
copy_betadisk_to( libspectrum_snap *snap )
{
  libspectrum_snap_set_beta_paged( snap, trdos_active );
  libspectrum_snap_set_beta_direction( snap, trdos_direction );
  libspectrum_snap_set_beta_status( snap, trdos_status_register );
  libspectrum_snap_set_beta_track ( snap, trdos_system_register );
  libspectrum_snap_set_beta_sector( snap, trdos_track_register );
  libspectrum_snap_set_beta_data  ( snap, trdos_sector_register );
  libspectrum_snap_set_beta_system( snap, trdos_data_register );

  return 0;
}

static int
copy_ram_to( libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte *buffer;

  for( i = 0; i < 16; i++ ) {
    if( RAM[i] != NULL ) {

      buffer = malloc( 0x4000 * sizeof( libspectrum_byte ) );
      if( !buffer ) {
	ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
		  __LINE__ );
	return 1;
      }

      memcpy( buffer, RAM[i], 0x4000 );
      libspectrum_snap_set_pages( snap, i, buffer );
    }
  }

  return 0;
}

static int
copy_slt_to( libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte *buffer;

  for( i=0; i<256; i++ ) {

    libspectrum_snap_set_slt_length( snap, i, slt_length[i] );

    if( slt_length[i] ) {

      libspectrum_byte *buffer;

      buffer = malloc( slt_length[i] * sizeof(libspectrum_byte) );
      if( !buffer ) {
	ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
		  __LINE__ );
	return 1;
      }

      memcpy( buffer, slt[i], slt_length[i] );
      libspectrum_snap_set_slt( snap, i, buffer );
    }
  }

  if( slt_screen ) {
 
    buffer = malloc( 6912 * sizeof( libspectrum_byte ) );

    if( !buffer ) {
      ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
      return 1;
    }

    memcpy( buffer, slt_screen, 6912 );
    libspectrum_snap_set_slt_screen( snap, buffer );
    libspectrum_snap_set_slt_screen_level( snap, slt_screen_level );
  }

  return 0;
}

static int
copy_zxatasp_to( libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte *buffer, value;
  int attached;

  if( settings_current.zxatasp_active ) {

    libspectrum_snap_set_zxatasp_active( snap, 1 );

    libspectrum_snap_set_zxatasp_upload( snap,
					 settings_current.zxatasp_upload );
    libspectrum_snap_set_zxatasp_writeprotect( snap,
					       settings_current.zxatasp_wp );

    value = zxatasp_portA_read( 0x009f, &attached );
    libspectrum_snap_set_zxatasp_port_a( snap, value );

    value = zxatasp_portB_read( 0x019f, &attached );
    libspectrum_snap_set_zxatasp_port_b( snap, value );

    value = zxatasp_portC_read( 0x029f, &attached );
    libspectrum_snap_set_zxatasp_port_c( snap, value );

    value = zxatasp_control_read( 0x039f, &attached );
    libspectrum_snap_set_zxatasp_control( snap, value );

    libspectrum_snap_set_zxatasp_current_page( snap, zxatasp_current_page() );

    libspectrum_snap_set_zxatasp_pages( snap, 32 );

    for( i = 0; i < 32; i++ ) {

      buffer = malloc( 0x4000 * sizeof( libspectrum_byte ) );
      if( !buffer ) {
	ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
		  __LINE__ );
	return 1;
      }

      memcpy( buffer, zxatasp_ram( i ), 0x4000 );
      libspectrum_snap_set_zxatasp_ram( snap, i, buffer );

    }
  }

  return 0;
}

static int
copy_zxcf_to( libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte *buffer;

  if( settings_current.zxcf_active ) {

    libspectrum_snap_set_zxcf_upload( snap, settings_current.zxcf_upload );
    libspectrum_snap_set_zxcf_active( snap, 1 );
    libspectrum_snap_set_zxcf_memctl( snap, zxcf_last_memctl() );
    libspectrum_snap_set_zxcf_pages( snap, 64 );

    for( i = 0; i < 64; i++ ) {

      buffer = malloc( 0x4000 * sizeof( libspectrum_byte ) );
      if( !buffer ) {
	ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
		  __LINE__ );
	return 1;
      }

      memcpy( buffer, zxcf_ram( i ), 0x4000 );
      libspectrum_snap_set_zxcf_ram( snap, i, buffer );

    }
  }

  return 0;
}

static int
copy_if2_to( libspectrum_snap *snap )
{
  libspectrum_byte *buffer;

  if( if2_active ) {

    libspectrum_snap_set_interface2_active( snap, 1 );

    buffer = malloc( 0x4000 * sizeof( libspectrum_byte ) );
    if( !buffer ) {
      ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
                __LINE__ );
      return 1;
    }

    memcpy( buffer, memory_map_romcs[0].page, MEMORY_PAGE_SIZE );
    memcpy( buffer + MEMORY_PAGE_SIZE,
            memory_map_romcs[1].page,
            MEMORY_PAGE_SIZE );
    libspectrum_snap_set_interface2_rom( snap, 0, buffer );

  }

  return 0;
}

static int
copy_timex_to( libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte *buffer;

  libspectrum_snap_set_out_scld_hsr( snap, scld_last_hsr );
  libspectrum_snap_set_out_scld_dec( snap, scld_last_dec.byte );

  if( dck_active ) {

    libspectrum_snap_set_dock_active( snap, 1 );

    for( i = 0; i < 8; i++ ) {

      if( memory_map_exrom[i]->source == MEMORY_SOURCE_CARTRIDGE ||
	  memory_map_exrom[i]->writable ) {
        buffer = malloc( MEMORY_PAGE_SIZE * sizeof( libspectrum_byte ) );
        if( !buffer ) {
          ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
                    __LINE__ );
          return 1;
        }

        libspectrum_snap_set_exrom_ram( snap, i,
                                        memory_map_exrom[i]->writable );
        memcpy( buffer, memory_map_exrom[i]->page, MEMORY_PAGE_SIZE );
        libspectrum_snap_set_exrom_cart( snap, i, buffer );
      }

      if( memory_map_dock[i]->source == MEMORY_SOURCE_CARTRIDGE ||
	  memory_map_dock[i]->writable ) {
        buffer = malloc( MEMORY_PAGE_SIZE * sizeof( libspectrum_byte ) );
        if( !buffer ) {
          ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
                    __LINE__ );
          return 1;
        }

        libspectrum_snap_set_dock_ram( snap, i, memory_map_dock[i]->writable );
        memcpy( buffer, memory_map_dock[i]->page, MEMORY_PAGE_SIZE );
        libspectrum_snap_set_dock_cart( snap, i, buffer );
      }

    }

  }

  return 0;
}

int
snapshot_copy_to( libspectrum_snap *snap )
{
  int error;

  libspectrum_snap_set_machine( snap, machine_current->machine );

  error = copy_z80_to( snap ); if( error ) return error;
  error = copy_ula_to( snap ); if( error ) return error;
  error = copy_128k_to( snap ); if( error ) return error;
  error = copy_betadisk_to( snap ); if( error ) return error;
  error = copy_ram_to( snap ); if( error ) return error;
  error = copy_slt_to( snap ); if( error ) return error;
  error = copy_zxatasp_to( snap ); if( error ) return error;
  error = copy_zxcf_to( snap ); if( error ) return error;
  error = copy_if2_to( snap ); if( error ) return error;
  error = copy_timex_to( snap ); if( error ) return error;

  return 0;
}
