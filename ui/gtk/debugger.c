/* debugger.c: the GTK+ debugger
   Copyright (c) 2002 Philip Kendall

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

#ifdef UI_GTK		/* Use this file iff we're using GTK+ */

#include <stdio.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "fuse.h"
#include "gtkui.h"
#include "machine.h"
#include "scld.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

static int create_dialog( void );
static int activate_debugger( void );
static int update_disassembly( void );
static int deactivate_debugger( void );

static void move_disassembly( GtkAdjustment *adjustment, gpointer user_data );

static void evaluate_command( GtkWidget *widget, gpointer user_data );
static void gtkui_debugger_done_step( GtkWidget *widget, gpointer user_data );
static void gtkui_debugger_done_continue( GtkWidget *widget,
					  gpointer user_data );
static void gtkui_debugger_break( GtkWidget *widget, gpointer user_data );
static void gtkui_debugger_done_close( GtkWidget *widget, gpointer user_data );

static GtkWidget *dialog,		/* The debugger dialog box */
  *continue_button, *break_button,	/* Two of its buttons */
  *registers[18],			/* The register display */
  *breakpoints,				/* The breakpoint display */
  *disassembly,				/* The disassembly */
  *stack;				/* The stack display */

static GtkObject *disassembly_scrollbar_adjustment;

/* The top line of the current disassembly */
static WORD disassembly_top;

/* Have we created the above yet? */
static int dialog_created = 0;

/* Is the debugger window active (as opposed to the debugger itself)? */
static int debugger_active;

int
ui_debugger_activate( void )
{
  fuse_emulation_pause();

  /* Create the dialog box if it doesn't already exist */
  if( !dialog_created ) if( create_dialog() ) return 1;

  gtk_widget_show_all( dialog );

  gtk_widget_set_sensitive( continue_button, 1 );
  gtk_widget_set_sensitive( break_button, 0 );
  if( !debugger_active ) activate_debugger();

  return 0;
}

int
ui_debugger_deactivate( int interruptable )
{
  if( debugger_active ) deactivate_debugger();

  gtk_widget_set_sensitive( continue_button, !interruptable );
  gtk_widget_set_sensitive( break_button,     interruptable );

  return 0;
}

static int
create_dialog( void )
{
  size_t i;
  GtkWidget *hbox, *vbox;
  GtkWidget *table;
  GtkWidget *entry, *eval_button;
  GtkWidget *step_button, *close_button;
  GtkAccelGroup *accel_group;
  GtkStyle *style;

  GtkWidget *scrollbar;

  gchar *breakpoint_titles[] = { "ID", "Type", "Value", "Ignore", "Life" },
    *disassembly_titles[] = { "Address", "Instruction" },
    *stack_titles[] = { "Address", "Value" };

  /* Try and get a monospaced font */
  style = gtk_style_new();
  gdk_font_unref( style->font );

  style->font = gdk_font_load( "-*-courier-medium-r-*-*-12-*-*-*-*-*-*-*" );
  if( !style->font ) {
    ui_error( UI_ERROR_ERROR, "couldn't find a monospaced font" );
    return 1;
  }

  dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( dialog ), "Fuse - Debugger" );

  /* A couple of boxes to contain the things we want to display */
  hbox = gtk_hbox_new( FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ), hbox,
		      TRUE, TRUE, 5 );

  vbox = gtk_vbox_new( FALSE, 5 );
  gtk_box_pack_start( GTK_BOX( hbox ), vbox, TRUE, TRUE, 5 );

  /* 'table' contains the register display */
  table = gtk_table_new( 9, 2, FALSE );
  gtk_box_pack_start( GTK_BOX( vbox ), table, FALSE, FALSE, 0 );

  for( i = 0; i < 18; i++ ) {
    registers[i] = gtk_label_new( "" );
    gtk_widget_set_style( registers[i], style );
    gtk_table_attach( GTK_TABLE( table ), registers[i], i%2, i%2+1, i/2, i/2+1,
		      0, 0, 2, 2 );
  }

  /* The breakpoint CList */
  breakpoints = gtk_clist_new_with_titles( 5, breakpoint_titles );
  gtk_clist_column_titles_passive( GTK_CLIST( breakpoints ) );
  for( i = 0; i < 5; i++ )
    gtk_clist_set_column_auto_resize( GTK_CLIST( breakpoints ), i, TRUE );
  gtk_box_pack_start_defaults( GTK_BOX( vbox ), breakpoints );

  /* Create the disassembly CList itself */
  disassembly = gtk_clist_new_with_titles( 2, disassembly_titles );
  gtk_widget_set_style( disassembly, style );
  gtk_clist_column_titles_passive( GTK_CLIST( disassembly ) );
  for( i = 0; i < 2; i++ )
    gtk_clist_set_column_auto_resize( GTK_CLIST( disassembly ), i, TRUE );
  gtk_box_pack_start_defaults( GTK_BOX( hbox ), disassembly );

  /* The disassembly scrollbar */
  disassembly_scrollbar_adjustment =
    gtk_adjustment_new( 0, 0x0000, 0xffff, 0.5, 20, 20 );
  gtk_signal_connect( GTK_OBJECT( disassembly_scrollbar_adjustment ),
		      "value-changed", GTK_SIGNAL_FUNC( move_disassembly ),
		      NULL );
  scrollbar =
    gtk_vscrollbar_new( GTK_ADJUSTMENT( disassembly_scrollbar_adjustment ) );
  gtk_box_pack_start( GTK_BOX( hbox ), scrollbar, FALSE, FALSE, 0 );

  /* And the stack CList */
  stack = gtk_clist_new_with_titles( 2, stack_titles );
  gtk_widget_set_style( stack, style );
  gtk_clist_column_titles_passive( GTK_CLIST( stack ) );
  for( i = 0; i < 2; i++ )
    gtk_clist_set_column_auto_resize( GTK_CLIST( stack ), i, TRUE );
  gtk_box_pack_start( GTK_BOX( hbox ), stack, TRUE, TRUE, 5 );

  /* Another hbox to hold the command entry widget and the 'evaluate'
     button */
  hbox = gtk_hbox_new( FALSE, 5 );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ), hbox,
		      FALSE, FALSE, 0 );

  /* The command entry widget */
  entry = gtk_entry_new();
  gtk_signal_connect( GTK_OBJECT( entry ), "activate",
		      GTK_SIGNAL_FUNC( evaluate_command ), NULL );
  gtk_box_pack_start_defaults( GTK_BOX( hbox ), entry );

  /* The 'command evaluate' button */
  eval_button = gtk_button_new_with_label( "Evaluate" );
  gtk_signal_connect_object( GTK_OBJECT( eval_button ), "clicked",
			     GTK_SIGNAL_FUNC( evaluate_command ),
			     GTK_OBJECT( entry ) );
  gtk_box_pack_start( GTK_BOX( hbox ), eval_button, FALSE, FALSE, 0 );

  /* The action buttons for the dialog box */

  step_button = gtk_button_new_with_label( "Single Step" );
  gtk_signal_connect( GTK_OBJECT( step_button ), "clicked",
		      GTK_SIGNAL_FUNC( gtkui_debugger_done_step ), NULL );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     step_button );

  continue_button = gtk_button_new_with_label( "Continue" );
  gtk_signal_connect( GTK_OBJECT( continue_button ), "clicked",
		      GTK_SIGNAL_FUNC( gtkui_debugger_done_continue ), NULL );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     continue_button );

  break_button = gtk_button_new_with_label( "Break" );
  gtk_signal_connect( GTK_OBJECT( break_button ), "clicked",
		      GTK_SIGNAL_FUNC( gtkui_debugger_break ), NULL );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     break_button );

  close_button = gtk_button_new_with_label( "Close" );
  gtk_signal_connect_object( GTK_OBJECT( close_button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_debugger_done_close ),
			     GTK_OBJECT( dialog ) );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     close_button );

  /* Deleting the window is the same as pressing 'Close' */
  gtk_signal_connect( GTK_OBJECT( dialog ), "delete_event",
		      GTK_SIGNAL_FUNC( gtkui_debugger_done_close ),
		      (gpointer) NULL );

  /* Keyboard shortcuts */
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group( GTK_WINDOW( dialog ), accel_group );

  /* Return is equivalent to clicking on 'evaluate' */
  gtk_widget_add_accelerator( eval_button, "clicked", accel_group,
			      GDK_Return, 0, 0 );

  /* Esc is equivalent to clicking on 'close' */
  gtk_widget_add_accelerator( close_button, "clicked", accel_group,
                              GDK_Escape, 0, 0 );

  dialog_created = 1;

  return 0;
}

static int
activate_debugger( void )
{
  debugger_active = 1;

  ui_debugger_disassemble( PC );
  ui_debugger_update();

  gtk_main();
  return 0;
}

/* Update the debugger's display */
int
ui_debugger_update( void )
{
  size_t i;
  char buffer[1024], format_string[1024];
  gchar *breakpoint_text[5] = { &buffer[  0], &buffer[ 40], &buffer[80],
			        &buffer[120], &buffer[160]               },
    *disassembly_text[2] = { &buffer[0], &buffer[40] };
  WORD address;
  const char *format_16_bit, *format_8_bit;
  GSList *ptr;
  int capabilities; size_t length;
  int error;

  const char *register_name[] = { "PC", "SP",
				  "AF", "AF'",
				  "BC", "BC'",
				  "DE", "DE'",
				  "HL", "HL'",
				  "IX", "IY",
                                };

  WORD *value_ptr[] = { &PC, &SP,  &AF, &AF_,
			&BC, &BC_, &DE, &DE_,
			&HL, &HL_, &IX, &IY,
                      };

  if( debugger_output_base == 10 ) {
    format_16_bit = "%5d"; format_8_bit = "%3d";
  } else {
    format_16_bit = "0x%04X"; format_8_bit = "0x%02X";
  }

  for( i = 0; i < 12; i++ ) {
    snprintf( buffer, 5, "%3s ", register_name[i] );
    snprintf( &buffer[4], 76, format_16_bit, *value_ptr[i] );
    gtk_label_set_text( GTK_LABEL( registers[i] ), buffer );
  }

  strcpy( buffer, "  I   " ); snprintf( &buffer[6], 76, format_8_bit, I );
  gtk_label_set_text( GTK_LABEL( registers[12] ), buffer );
  strcpy( buffer, "  R   " );
  snprintf( &buffer[6], 80, format_8_bit, ( R & 0x7f ) | ( R7 & 0x80 ) );
  gtk_label_set_text( GTK_LABEL( registers[13] ), buffer );

  snprintf( buffer, 80, "T-states %5d", tstates );
  gtk_label_set_text( GTK_LABEL( registers[14] ), buffer );
  snprintf( buffer, 80, "  IM %d\nIFF1 %d\nIFF2 %d", IM, IFF1, IFF2 );
  gtk_label_set_text( GTK_LABEL( registers[15] ), buffer );

  strcpy( buffer, "SZ5H3PNC\n" );
  for( i = 0; i < 8; i++ ) buffer[i+9] = ( F & ( 0x80 >> i ) ) ? '1' : '0';
  buffer[17] = '\0';
  gtk_label_set_text( GTK_LABEL( registers[16] ), buffer );

  capabilities = libspectrum_machine_capabilities( machine_current->machine );

  sprintf( format_string, "   ULA %s", format_8_bit );
  snprintf( buffer, 1024, format_string, spectrum_last_ula );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_AY ) {
    sprintf( format_string, "\n    AY %s", format_8_bit );
    length = strlen( buffer );
    snprintf( &buffer[length], 1024-length, format_string,
	      machine_current->ay.current_register );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) {
    sprintf( format_string, "\n128Mem %s", format_8_bit );
    length = strlen( buffer );
    snprintf( &buffer[length], 1024-length, format_string,
	      machine_current->ram.last_byte );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY ) {
    sprintf( format_string, "\n+3 Mem %s", format_8_bit );
    length = strlen( buffer );
    snprintf( &buffer[length], 1024-length, format_string,
	      machine_current->ram.last_byte2 );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY ) {
    sprintf( format_string, "\nTmxDec %s", format_8_bit );
    length = strlen( buffer );
    snprintf( &buffer[length], 1024-length, format_string, scld_last_dec );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_VIDEO ) {
    sprintf( format_string, "\nTmxHsr %s", format_8_bit );
    length = strlen( buffer );
    snprintf( &buffer[length], 1024-length, format_string, scld_last_hsr );
  }

  gtk_label_set_text( GTK_LABEL( registers[17] ), buffer );

  /* Create the breakpoint list */
  gtk_clist_freeze( GTK_CLIST( breakpoints ) );
  gtk_clist_clear( GTK_CLIST( breakpoints ) );

  for( ptr = debugger_breakpoints; ptr; ptr = ptr->next ) {

    debugger_breakpoint *bp = ptr->data;

    snprintf( breakpoint_text[0], 40, "%lu", (unsigned long)bp->id );
    snprintf( breakpoint_text[1], 40, "%s",
	      debugger_breakpoint_type_text[ bp->type ] );
    snprintf( breakpoint_text[2], 40, format_16_bit, bp->value );
    snprintf( breakpoint_text[3], 40, "%lu", (unsigned long)bp->ignore );
    snprintf( breakpoint_text[4], 40, "%s",
	      debugger_breakpoint_life_text[ bp->life ] );

    gtk_clist_append( GTK_CLIST( breakpoints ), breakpoint_text );
  }

  gtk_clist_thaw( GTK_CLIST( breakpoints ) );

  /* Update the disassembly */
  error = update_disassembly(); if( error ) return error;

  /* And the stack display */
  gtk_clist_freeze( GTK_CLIST( stack ) );
  gtk_clist_clear( GTK_CLIST( stack ) );

  for( i = 0, address = SP + 38; i < 20; i++, address -= 2 ) {
    
    WORD contents = readbyte_internal( address ) +
                    0x100 * readbyte_internal( address + 1 );

    snprintf( disassembly_text[0], 40, format_16_bit, address );
    snprintf( disassembly_text[1], 40, format_16_bit, contents );
    gtk_clist_append( GTK_CLIST( stack ), disassembly_text );
  }

  gtk_clist_thaw( GTK_CLIST( stack ) );

  return 0;
}

static int
update_disassembly( void )
{
  size_t i, length; WORD address;
  char buffer[80];
  char *disassembly_text[2] = { &buffer[0], &buffer[40] };

  const char *format_16_bit =
    ( debugger_output_base == 10 ? "%5d" : "0x%04X" );

  gtk_clist_freeze( GTK_CLIST( disassembly ) );
  gtk_clist_clear( GTK_CLIST( disassembly ) );

  for( i = 0, address = disassembly_top; i < 20; i++ ) {

    snprintf( disassembly_text[0], 40, format_16_bit, address );
    debugger_disassemble( disassembly_text[1], 40, &length, address );
    address += length;

    gtk_clist_append( GTK_CLIST( disassembly ), disassembly_text );
  }
  gtk_clist_thaw( GTK_CLIST( disassembly ) );

  return 0;
}

static int
deactivate_debugger( void )
{
  gtk_main_quit();
  debugger_active = 0;
  fuse_emulation_unpause();
  return 0;
}

/* Set the disassembly to start at 'address' */
int
ui_debugger_disassemble( WORD address )
{
  GTK_ADJUSTMENT( disassembly_scrollbar_adjustment )->value =
    disassembly_top = address;
  return 0;
}

/* Called when the disassembly scrollbar is moved */
static void
move_disassembly( GtkAdjustment *adjustment, gpointer user_data GCC_UNUSED )
{
  float value = adjustment->value;
  size_t length;

  /* disassembly_top < value < disassembly_top + 1 => 'down' button pressed
     Move the disassembly on by one instruction */
  if( value > disassembly_top && value - disassembly_top < 1 ) {

    debugger_disassemble( NULL, 0, &length, disassembly_top );
    ui_debugger_disassemble( disassembly_top + length );

  /* disassembly_top - 1 < value < disassembly_top => 'up' button pressed
     
     The desired state after this is for the current top instruction
     to be the second instruction shown in the disassembly.

     Unfortunately, it's not trivial to determine where disassembly
     should now start, as we have variable length instructions of
     unbounded length (multiple DD and FD prefixes on one instruction
     are possible).

     In general, we want the _longest_ opcode which produces the
     current top in second place (consider something like LD A,nn:
     we're not interested if nn happens to represent a one-byte
     opcode), so look back a reasonable length (say, 8 bytes) and see
     what we find.

     In some cases (eg if we're currently pointing to a data byte of a
     multi-byte opcode), it will be impossible to get the current top
     second. In this case, just move back a byte.

  */
  } else if( value < disassembly_top && disassembly_top - value < 1 ) {

    size_t i, longest = 1;

    for( i = 1; i <= 8; i++ ) {

      debugger_disassemble( NULL, 0, &length, disassembly_top - i );
      if( length == i ) longest = i;

    }

    ui_debugger_disassemble( disassembly_top - longest );

  /* Anything else, just set disassembly_top to that value */
  } else {

    ui_debugger_disassemble( value );

  }

  /* And update the disassembly if the debugger is active */
  if( debugger_active ) update_disassembly();
}

/* Evaluate the command currently in the entry box */
static void
evaluate_command( GtkWidget *widget, gpointer user_data GCC_UNUSED )
{
  debugger_command_evaluate( gtk_entry_get_text( GTK_ENTRY( widget ) ) );
}

static void
gtkui_debugger_done_step( GtkWidget *widget GCC_UNUSED,
			  gpointer user_data GCC_UNUSED )
{
  debugger_step();
}

static void
gtkui_debugger_done_continue( GtkWidget *widget GCC_UNUSED,
			      gpointer user_data GCC_UNUSED )
{
  debugger_run();
}

static void
gtkui_debugger_break( GtkWidget *widget GCC_UNUSED,
		      gpointer user_data GCC_UNUSED )
{
  debugger_mode = DEBUGGER_MODE_HALTED;
  gtk_widget_set_sensitive( continue_button, 1 );
  gtk_widget_set_sensitive( break_button, 0 );
}

static void
gtkui_debugger_done_close( GtkWidget *widget, gpointer user_data GCC_UNUSED )
{
  gtk_widget_hide_all( widget );
  gtkui_debugger_done_continue( NULL, NULL );
}

#endif			/* #ifdef UI_GTK */
