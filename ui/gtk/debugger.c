/* debugger.c: the GTK+ debugger
   Copyright (c) 2002-2008 Philip Kendall

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

#include <config.h>

#include <stdio.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "event.h"
#include "fuse.h"
#include "gtkinternals.h"
#include "machine.h"
#include "memory.h"
#include "scld.h"
#include "settings.h"
#include "ui/ui.h"
#include "ula.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"
#include "zxcf.h"

/* The various debugger panes */
typedef enum debugger_pane {

  DEBUGGER_PANE_BEGIN = 1,	/* Start marker */

  DEBUGGER_PANE_REGISTERS = DEBUGGER_PANE_BEGIN,
  DEBUGGER_PANE_MEMORYMAP,
  DEBUGGER_PANE_BREAKPOINTS,
  DEBUGGER_PANE_DISASSEMBLY,
  DEBUGGER_PANE_STACK,
  DEBUGGER_PANE_EVENTS,

  DEBUGGER_PANE_END		/* End marker */

} debugger_pane;

static int create_dialog( void );
static int hide_hidden_panes( void );
static GtkCheckMenuItem* get_pane_menu_item( debugger_pane pane );
static GtkWidget* get_pane( debugger_pane pane );
static int create_menu_bar( GtkBox *parent, GtkAccelGroup *accel_group );
static void toggle_display( gpointer callback_data, guint callback_action,
			    GtkWidget *widget );
static int create_register_display( GtkBox *parent, gtkui_font font );
static int create_memory_map( GtkBox *parent );
static int create_breakpoints( GtkBox *parent );
static int create_disassembly( GtkBox *parent, gtkui_font font );
static int create_stack_display( GtkBox *parent, gtkui_font font );
static void stack_click( GtkCList *clist, gint row, gint column,
			 GdkEventButton *event, gpointer user_data );
static int create_events( GtkBox *parent );
static void events_click( GtkCList *clist, gint row, gint column,
			  GdkEventButton *event, gpointer user_data );
static int create_command_entry( GtkBox *parent, GtkAccelGroup *accel_group );
static int create_buttons( GtkDialog *parent, GtkAccelGroup *accel_group );

static int activate_debugger( void );
static int update_memory_map( void );
static int update_breakpoints( void );
static int update_disassembly( void );
static int update_events( void );
static void add_event( gpointer data, gpointer user_data );
static int deactivate_debugger( void );

static void move_disassembly( GtkAdjustment *adjustment, gpointer user_data );

static void evaluate_command( GtkWidget *widget, gpointer user_data );
static void gtkui_debugger_done_step( GtkWidget *widget, gpointer user_data );
static void gtkui_debugger_done_continue( GtkWidget *widget,
					  gpointer user_data );
static void gtkui_debugger_break( GtkWidget *widget, gpointer user_data );
static gboolean delete_dialog( GtkWidget *widget, GdkEvent *event,
			       gpointer user_data );
static void gtkui_debugger_done_close( GtkWidget *widget, gpointer user_data );

static GtkWidget *dialog,		/* The debugger dialog box */
  *continue_button, *break_button,	/* Two of its buttons */
  *register_display,			/* The register display */
  *registers[18],			/* Individual registers */
  *memorymap,				/* The memory map display */
  *map_label[8][4],			/* Labels in the memory map */
  *breakpoints,				/* The breakpoint display */
  *disassembly_box,			/* A box to hold the disassembly */
  *disassembly,				/* The actual disassembly widget */
  *stack,				/* The stack display */
  *events;				/* The events display */

static GtkObject *disassembly_scrollbar_adjustment;

/* The top line of the current disassembly */
static libspectrum_word disassembly_top;

/* Have we created the above yet? */
static int dialog_created = 0;

/* Is the debugger window active (as opposed to the debugger itself)? */
static int debugger_active;

/* The factory used to create the menu bar */
static GtkItemFactory *menu_factory;

/* The debugger's menu bar */
static GtkItemFactoryEntry menu_data[] = {

  { "/_View", NULL, NULL, 0, "<Branch>", NULL },
  { "/View/_Registers", NULL, toggle_display, DEBUGGER_PANE_REGISTERS, "<ToggleItem>", NULL },
  { "/View/_Memory Map", NULL, toggle_display, DEBUGGER_PANE_MEMORYMAP, "<ToggleItem>", NULL },
  { "/View/_Breakpoints", NULL, toggle_display, DEBUGGER_PANE_BREAKPOINTS, "<ToggleItem>", NULL },
  { "/View/_Disassembly", NULL, toggle_display, DEBUGGER_PANE_DISASSEMBLY, "<ToggleItem>", NULL },
  { "/View/_Stack", NULL, toggle_display, DEBUGGER_PANE_STACK, "<ToggleItem>", NULL },
  { "/View/_Events", NULL, toggle_display, DEBUGGER_PANE_EVENTS, "<ToggleItem>", NULL },

};

static guint menu_data_count =
  sizeof( menu_data ) / sizeof( GtkItemFactoryEntry );

static const char*
format_8_bit( void )
{
  return debugger_output_base == 10 ? "%3d" : "0x%02X";
}

static const char*
format_16_bit( void )
{
  return debugger_output_base == 10 ? "%5d" : "0x%04X";
}

int
ui_debugger_activate( void )
{
  int error;

  fuse_emulation_pause();

  /* Create the dialog box if it doesn't already exist */
  if( !dialog_created ) if( create_dialog() ) return 1;

  gtk_widget_show_all( dialog );
  error = hide_hidden_panes(); if( error ) return error;

  gtk_widget_set_sensitive( continue_button, 1 );
  gtk_widget_set_sensitive( break_button, 0 );
  if( !debugger_active ) activate_debugger();

  return 0;
}

static int
hide_hidden_panes( void )
{
  debugger_pane i;
  GtkCheckMenuItem *checkitem; GtkWidget *pane;

  for( i = DEBUGGER_PANE_BEGIN; i < DEBUGGER_PANE_END; i++ ) {

    checkitem = get_pane_menu_item( i ); if( !checkitem ) return 1;

    if( checkitem->active ) continue;

    pane = get_pane( i ); if( !pane ) return 1;

    gtk_widget_hide_all( pane );
  }

  return 0;
}

static GtkCheckMenuItem*
get_pane_menu_item( debugger_pane pane )
{
  const gchar *path;
  GtkWidget *menu_item;

  path = NULL;

  switch( pane ) {
  case DEBUGGER_PANE_REGISTERS: path = "/View/Registers"; break;
  case DEBUGGER_PANE_MEMORYMAP: path = "/View/Memory Map"; break;
  case DEBUGGER_PANE_BREAKPOINTS: path = "/View/Breakpoints"; break;
  case DEBUGGER_PANE_DISASSEMBLY: path = "/View/Disassembly"; break;
  case DEBUGGER_PANE_STACK: path = "/View/Stack"; break;
  case DEBUGGER_PANE_EVENTS: path = "/View/Events"; break;

  case DEBUGGER_PANE_END: break;
  }

  if( !path ) {
    ui_error( UI_ERROR_ERROR, "unknown debugger pane %u", pane );
    return NULL;
  }

  menu_item = gtk_item_factory_get_widget( menu_factory, path );
  if( !menu_item ) {
    ui_error( UI_ERROR_ERROR, "couldn't get menu item '%s' from factory",
	      path );
    return NULL;
  }

  return GTK_CHECK_MENU_ITEM( menu_item );
}

static GtkWidget*
get_pane( debugger_pane pane )
{
  switch( pane ) {
  case DEBUGGER_PANE_REGISTERS: return register_display;
  case DEBUGGER_PANE_MEMORYMAP: return memorymap;
  case DEBUGGER_PANE_BREAKPOINTS: return breakpoints;
  case DEBUGGER_PANE_DISASSEMBLY: return disassembly_box;
  case DEBUGGER_PANE_STACK: return stack;
  case DEBUGGER_PANE_EVENTS: return events;

  case DEBUGGER_PANE_END: break;
  }

  ui_error( UI_ERROR_ERROR, "unknown debugger pane %u", pane );
  return NULL;
}
  
int
ui_debugger_deactivate( int interruptable )
{
  if( debugger_active ) deactivate_debugger();

  if( dialog_created ) {
    gtk_widget_set_sensitive( continue_button, !interruptable );
    gtk_widget_set_sensitive( break_button,     interruptable );
  }

  return 0;
}

static int
create_dialog( void )
{
  int error;
  GtkWidget *hbox, *vbox, *hbox2;
  GtkAccelGroup *accel_group;
  debugger_pane i;

  gtkui_font font;

  error = gtkui_get_monospaced_font( &font ); if( error ) return error;

  dialog = gtkstock_dialog_new( "Fuse - Debugger",
				GTK_SIGNAL_FUNC( delete_dialog ) );

  /* Keyboard shortcuts */
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group( GTK_WINDOW( dialog ), accel_group );

  /* The menu bar */
  error = create_menu_bar( GTK_BOX( GTK_DIALOG( dialog )->vbox ),
			   accel_group );
  if( error ) return error;

  /* Some boxes to contain the things we want to display */
  hbox = gtk_hbox_new( FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ), hbox,
		      TRUE, TRUE, 5 );

  vbox = gtk_vbox_new( FALSE, 5 );
  gtk_box_pack_start( GTK_BOX( hbox ), vbox, TRUE, TRUE, 5 );

  hbox2 = gtk_hbox_new( FALSE, 5 );
  gtk_box_pack_start_defaults( GTK_BOX( vbox ), hbox2 );

  /* The main display areas */
  error = create_register_display( GTK_BOX( hbox2 ), font );
  if( error ) return error;

  error = create_memory_map( GTK_BOX( hbox2 ) ); if( error ) return error;

  error = create_breakpoints( GTK_BOX( vbox ) ); if( error ) return error;

  error = create_disassembly( GTK_BOX( hbox ), font );
  if( error ) return error;

  error = create_stack_display( GTK_BOX( hbox ), font );
  if( error ) return error;

  error = create_events( GTK_BOX( hbox ) ); if( error ) return error;

  error = create_command_entry( GTK_BOX( GTK_DIALOG( dialog )->vbox ),
				accel_group );
  if( error ) return error;

  /* The action buttons */

  error = create_buttons( GTK_DIALOG( dialog ), accel_group );
  if( error ) return error;

  /* Initially, have all the panes visible */
  for( i = DEBUGGER_PANE_BEGIN; i < DEBUGGER_PANE_END; i++ ) {
    
    GtkCheckMenuItem *check_item;

    check_item = get_pane_menu_item( i ); if( !check_item ) break;
    gtk_check_menu_item_set_active( check_item, TRUE );
  }

  gtkui_free_font( font );

  dialog_created = 1;

  return 0;
}

static int
create_menu_bar( GtkBox *parent, GtkAccelGroup *accel_group )
{
  GtkWidget *menu_bar;

  menu_factory = gtk_item_factory_new( GTK_TYPE_MENU_BAR, "<main>",
				       accel_group );
  gtk_item_factory_create_items( menu_factory, menu_data_count, menu_data,
				 NULL);
  menu_bar = gtk_item_factory_get_widget( menu_factory, "<main>" );

  gtk_box_pack_start( parent, menu_bar, FALSE, FALSE, 0 );
  
  return 0;
}

static void
toggle_display( gpointer callback_data GCC_UNUSED, guint callback_action,
		GtkWidget *widget )
{
  GtkWidget *pane;

  pane = get_pane( callback_action ); if( !pane ) return;

  if( GTK_CHECK_MENU_ITEM( widget )->active ) {
    gtk_widget_show_all( pane );
  } else {
    gtk_widget_hide_all( pane );
  }
}

static int
create_register_display( GtkBox *parent, gtkui_font font )
{
  size_t i;

  register_display = gtk_table_new( 9, 2, FALSE );
  gtk_box_pack_start( parent, register_display, FALSE, FALSE, 0 );

  for( i = 0; i < 18; i++ ) {
    registers[i] = gtk_label_new( "" );
    gtkui_set_font( registers[i], font );
    gtk_table_attach( GTK_TABLE( register_display ), registers[i],
		      i%2, i%2+1, i/2, i/2+1, 0, 0, 2, 2 );
  }

  return 0;
}

static int
create_memory_map( GtkBox *parent )
{
  GtkWidget *table, *label;
  size_t i, j;

  memorymap = gtk_frame_new( "Memory Map" );
  gtk_box_pack_start( parent, memorymap, FALSE, FALSE, 0 );

  table = gtk_table_new( 9, 4, FALSE );
  gtk_container_add( GTK_CONTAINER( memorymap ), table );

  label = gtk_label_new( "Address" );
  gtk_table_attach( GTK_TABLE( table ), label, 0, 1, 0, 1, 0, 0, 2, 2 );

  label = gtk_label_new( "Source" );
  gtk_table_attach( GTK_TABLE( table ), label, 1, 2, 0, 1, 0, 0, 2, 2 );

  label = gtk_label_new( "W?" );
  gtk_table_attach( GTK_TABLE( table ), label, 2, 3, 0, 1, 0, 0, 2, 2 );

  label = gtk_label_new( "C?" );
  gtk_table_attach( GTK_TABLE( table ), label, 3, 4, 0, 1, 0, 0, 2, 2 );

  for( i = 0; i < 8; i++ ) {
    for( j = 0; j < 4; j++ ) {
      map_label[i][j] = gtk_label_new( "" );
      gtk_table_attach( GTK_TABLE( table ), map_label[i][j],
			j, j + 1, 8 - i, 9 - i, 0, 0, 2, 2 );
    }
  }

  return 0;
}

static int
create_breakpoints( GtkBox *parent )
{
  size_t i;

  gchar *breakpoint_titles[] = { "ID", "Type", "Value", "Ignore", "Life",
				 "Condition" };

  breakpoints = gtk_clist_new_with_titles( 6, breakpoint_titles );
  gtk_clist_column_titles_passive( GTK_CLIST( breakpoints ) );
  for( i = 0; i < 6; i++ )
    gtk_clist_set_column_auto_resize( GTK_CLIST( breakpoints ), i, TRUE );
  gtk_box_pack_start_defaults( parent, breakpoints );

  return 0;
}

static int
create_disassembly( GtkBox *parent, gtkui_font font )
{
  size_t i;

  GtkWidget *scrollbar;
  gchar *disassembly_titles[] = { "Address", "Instruction" };

  /* A box to hold the disassembly listing and the scrollbar */
  disassembly_box = gtk_hbox_new( FALSE, 0 );
  gtk_box_pack_start_defaults( parent, disassembly_box );

  /* The disassembly CList itself */
  disassembly = gtk_clist_new_with_titles( 2, disassembly_titles );
  gtkui_set_font( disassembly, font );
  gtk_clist_column_titles_passive( GTK_CLIST( disassembly ) );
  for( i = 0; i < 2; i++ )
    gtk_clist_set_column_auto_resize( GTK_CLIST( disassembly ), i, TRUE );
  gtk_box_pack_start_defaults( GTK_BOX( disassembly_box ), disassembly );

  /* The disassembly scrollbar */
  disassembly_scrollbar_adjustment =
    gtk_adjustment_new( 0, 0x0000, 0xffff, 0.5, 20, 20 );
  gtk_signal_connect( GTK_OBJECT( disassembly_scrollbar_adjustment ),
		      "value-changed", GTK_SIGNAL_FUNC( move_disassembly ),
		      NULL );
  scrollbar =
    gtk_vscrollbar_new( GTK_ADJUSTMENT( disassembly_scrollbar_adjustment ) );
  gtk_box_pack_start( GTK_BOX( disassembly_box ), scrollbar, FALSE, FALSE, 0 );

  gtkui_scroll_connect( GTK_CLIST( disassembly ),
			GTK_ADJUSTMENT( disassembly_scrollbar_adjustment ) );

  return 0;
}

static int
create_stack_display( GtkBox *parent, gtkui_font font )
{
  size_t i;
  gchar *stack_titles[] = { "Address", "Value" };

  stack = gtk_clist_new_with_titles( 2, stack_titles );
  gtkui_set_font( stack, font );
  gtk_clist_column_titles_passive( GTK_CLIST( stack ) );
  for( i = 0; i < 2; i++ )
    gtk_clist_set_column_auto_resize( GTK_CLIST( stack ), i, TRUE );
  gtk_box_pack_start( parent, stack, TRUE, TRUE, 5 );

  gtk_signal_connect( GTK_OBJECT( stack ), "select-row",
		      GTK_SIGNAL_FUNC( stack_click ), NULL );

  return 0;
}

static void
stack_click( GtkCList *clist GCC_UNUSED, gint row, gint column GCC_UNUSED,
	     GdkEventButton *event, gpointer user_data GCC_UNUSED )
{
  libspectrum_word address, destination;
  int error;

  /* Ignore events which aren't double-clicks or select-via-keyboard */
  if( event && event->type != GDK_2BUTTON_PRESS ) return;

  address = SP + ( 19 - row ) * 2;

  destination =
    readbyte_internal( address ) + readbyte_internal( address + 1 ) * 0x100;

  error = debugger_breakpoint_add_address(
    DEBUGGER_BREAKPOINT_TYPE_EXECUTE, -1, destination, 0,
    DEBUGGER_BREAKPOINT_LIFE_ONESHOT, NULL
  );
  if( error ) return;

  debugger_run();
}

static int
create_events( GtkBox *parent )
{
  size_t i;
  gchar *titles[] = { "Time", "Type" };

  events = gtk_clist_new_with_titles( 2, titles );
  gtk_clist_column_titles_passive( GTK_CLIST( events ) );
  for( i = 0; i < 2; i++ )
    gtk_clist_set_column_auto_resize( GTK_CLIST( events ), i, TRUE );
  gtk_box_pack_start( parent, events, TRUE, TRUE, 5 );

  gtk_signal_connect( GTK_OBJECT( events ), "select-row",
		      GTK_SIGNAL_FUNC( events_click ), NULL );

  return 0;
}

static void
events_click( GtkCList *clist, gint row, gint column GCC_UNUSED,
	      GdkEventButton *event, gpointer user_data GCC_UNUSED )
{
  int got_text, error;
  gchar *buffer;
  libspectrum_dword tstates;

  /* Ignore events which aren't double-clicks or select-via-keyboard */
  if( event && event->type != GDK_2BUTTON_PRESS ) return;

  got_text = gtk_clist_get_text( clist, row, 0, &buffer );
  if( !got_text ) {
    ui_error( UI_ERROR_ERROR, "couldn't get text for row %d", row );
    return;
  }

  tstates = atoi( buffer );
  error = debugger_breakpoint_add_time(
    DEBUGGER_BREAKPOINT_TYPE_TIME, tstates, 0,
    DEBUGGER_BREAKPOINT_LIFE_ONESHOT, NULL
  );
  if( error ) return;

  debugger_run();
}

static int
create_command_entry( GtkBox *parent, GtkAccelGroup *accel_group )
{
  GtkWidget *hbox, *entry, *eval_button;

  /* An hbox to hold the command entry widget and the 'evaluate' button */
  hbox = gtk_hbox_new( FALSE, 5 );
  gtk_box_pack_start( parent, hbox, FALSE, FALSE, 0 );

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

  /* Return is equivalent to clicking on 'evaluate' */
  gtk_widget_add_accelerator( eval_button, "clicked", accel_group,
			      GDK_Return, 0, 0 );

  return 0;
}

static int
create_buttons( GtkDialog *parent, GtkAccelGroup *accel_group )
{
  static const gtkstock_button
    step  = { "Single step", GTK_SIGNAL_FUNC( gtkui_debugger_done_step ), NULL, NULL, 0, 0, 0, 0 },
    cont  = { "Continue", GTK_SIGNAL_FUNC( gtkui_debugger_done_continue ), NULL, NULL, 0, 0, 0, 0 },
    brk   = { "Break", GTK_SIGNAL_FUNC( gtkui_debugger_break ), NULL, NULL, 0, 0, 0, 0 };

  /* Create the action buttons for the dialog box */
  gtkstock_create_button( GTK_WIDGET( parent ), accel_group, &step );
  continue_button = gtkstock_create_button( GTK_WIDGET( parent ), accel_group,
					    &cont );
  break_button = gtkstock_create_button( GTK_WIDGET( parent ), accel_group,
					 &brk );
  gtkstock_create_close( GTK_WIDGET( parent ), accel_group,
			 GTK_SIGNAL_FUNC( gtkui_debugger_done_close ), TRUE );

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
  gchar *disassembly_text[2] = { &buffer[0], &buffer[40] };
  libspectrum_word address;
  int capabilities; size_t length;
  int error;

  const char *register_name[] = { "PC", "SP",
				  "AF", "AF'",
				  "BC", "BC'",
				  "DE", "DE'",
				  "HL", "HL'",
				  "IX", "IY",
                                };

  libspectrum_word *value_ptr[] = { &PC, &SP,  &AF, &AF_,
				    &BC, &BC_, &DE, &DE_,
				    &HL, &HL_, &IX, &IY,
				  };

  if( !dialog_created ) return 0;

  for( i = 0; i < 12; i++ ) {
    snprintf( buffer, 5, "%3s ", register_name[i] );
    snprintf( &buffer[4], 76, format_16_bit(), *value_ptr[i] );
    gtk_label_set_text( GTK_LABEL( registers[i] ), buffer );
  }

  strcpy( buffer, "  I   " ); snprintf( &buffer[6], 76, format_8_bit(), I );
  gtk_label_set_text( GTK_LABEL( registers[12] ), buffer );
  strcpy( buffer, "  R   " );
  snprintf( &buffer[6], 80, format_8_bit(), ( R & 0x7f ) | ( R7 & 0x80 ) );
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

  sprintf( format_string, "   ULA %s", format_8_bit() );
  snprintf( buffer, 1024, format_string, ula_last_byte() );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_AY ) {
    sprintf( format_string, "\n    AY %s", format_8_bit() );
    length = strlen( buffer );
    snprintf( &buffer[length], 1024-length, format_string,
	      machine_current->ay.current_register );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) {
    sprintf( format_string, "\n128Mem %s", format_8_bit() );
    length = strlen( buffer );
    snprintf( &buffer[length], 1024-length, format_string,
	      machine_current->ram.last_byte );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY ) {
    sprintf( format_string, "\n+3 Mem %s", format_8_bit() );
    length = strlen( buffer );
    snprintf( &buffer[length], 1024-length, format_string,
	      machine_current->ram.last_byte2 );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_VIDEO  ||
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY ||
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY       ) {
    sprintf( format_string, "\nTmxDec %s", format_8_bit() );
    length = strlen( buffer );
    snprintf( &buffer[length], 1024-length, format_string,
	      scld_last_dec.byte );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY ||
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY       ) {
    sprintf( format_string, "\nTmxHsr %s", format_8_bit() );
    length = strlen( buffer );
    snprintf( &buffer[length], 1024-length, format_string, scld_last_hsr );
  }

  if( settings_current.zxcf_active ) {
    sprintf( format_string, "\n  ZXCF %s", format_8_bit() );
    length = strlen( buffer );
    snprintf( &buffer[length], 1024-length, format_string,
	      zxcf_last_memctl() );
  }

  gtk_label_set_text( GTK_LABEL( registers[17] ), buffer );

  /* Update the memory map display */
  error = update_memory_map(); if( error ) return error;

  error = update_breakpoints(); if( error ) return error;

  /* Update the disassembly */
  error = update_disassembly(); if( error ) return error;

  /* And the stack display */
  gtk_clist_freeze( GTK_CLIST( stack ) );
  gtk_clist_clear( GTK_CLIST( stack ) );

  for( i = 0, address = SP + 38; i < 20; i++, address -= 2 ) {
    
    libspectrum_word contents = readbyte_internal( address ) +
				0x100 * readbyte_internal( address + 1 );

    snprintf( disassembly_text[0], 40, format_16_bit(), address );
    snprintf( disassembly_text[1], 40, format_16_bit(), contents );
    gtk_clist_append( GTK_CLIST( stack ), disassembly_text );
  }

  gtk_clist_thaw( GTK_CLIST( stack ) );

  /* And the events display */
  error = update_events(); if( error ) return error;

  return 0;
}

static int
update_memory_map( void )
{
  size_t i;
  char buffer[ 40 ];

  for( i = 0; i < 8; i++ ) {

    snprintf( buffer, 40, format_16_bit(), (unsigned)i * 0x2000 );
    gtk_label_set_text( GTK_LABEL( map_label[i][0] ), buffer );

    snprintf( buffer, 40, "%s %d", memory_bank_name( &memory_map_read[i] ),
	      memory_map_read[i].page_num );
    gtk_label_set_text( GTK_LABEL( map_label[i][1] ), buffer );

    gtk_label_set_text( GTK_LABEL( map_label[i][2] ),
			memory_map_read[i].writable ? "Y" : "N" );

    gtk_label_set_text( GTK_LABEL( map_label[i][3] ),
			memory_map_read[i].contended ? "Y" : "N" );
  }

  return 0;
}

static int
update_breakpoints( void )
{
  gchar buffer[ 1024 ],
    *breakpoint_text[6] = { &buffer[  0], &buffer[ 40], &buffer[80],
			    &buffer[120], &buffer[160], &buffer[200] };
  GSList *ptr;
  char format_string[ 1024 ], page[ 1024 ];

  /* Create the breakpoint list */
  gtk_clist_freeze( GTK_CLIST( breakpoints ) );
  gtk_clist_clear( GTK_CLIST( breakpoints ) );

  for( ptr = debugger_breakpoints; ptr; ptr = ptr->next ) {

    debugger_breakpoint *bp = ptr->data;

    snprintf( breakpoint_text[0], 40, "%lu", (unsigned long)bp->id );
    snprintf( breakpoint_text[1], 40, "%s",
	      debugger_breakpoint_type_text[ bp->type ] );

    switch( bp->type ) {

    case DEBUGGER_BREAKPOINT_TYPE_EXECUTE:
    case DEBUGGER_BREAKPOINT_TYPE_READ:
    case DEBUGGER_BREAKPOINT_TYPE_WRITE:
      if( bp->value.address.page == -1 ) {
	snprintf( breakpoint_text[2], 40, format_16_bit(),
		  bp->value.address.offset );
      } else {
	debugger_breakpoint_decode_page( page, 1024, bp->value.address.page );
	snprintf( format_string, 1024, "%%s:%s", format_16_bit() );
	snprintf( breakpoint_text[2], 40, format_string, page,
		  bp->value.address.offset );
      }
      break;

    case DEBUGGER_BREAKPOINT_TYPE_PORT_READ:
    case DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE:
      sprintf( format_string, "%s:%s", format_16_bit(), format_16_bit() );
      snprintf( breakpoint_text[2], 40, format_string,
		bp->value.port.mask, bp->value.port.port );
      break;

    case DEBUGGER_BREAKPOINT_TYPE_TIME:
      snprintf( breakpoint_text[2], 40, "%5d", bp->value.tstates );
      break;

    case DEBUGGER_BREAKPOINT_TYPE_EVENT:
      snprintf( breakpoint_text[2], 40, "%s:%s", bp->value.event.type,
		bp->value.event.detail );
      break;

    }

    snprintf( breakpoint_text[3], 40, "%lu", (unsigned long)bp->ignore );
    snprintf( breakpoint_text[4], 40, "%s",
	      debugger_breakpoint_life_text[ bp->life ] );
    if( bp->condition ) {
      debugger_expression_deparse( breakpoint_text[5], 80, bp->condition );
    } else {
      strcpy( breakpoint_text[5], "" );
    }

    gtk_clist_append( GTK_CLIST( breakpoints ), breakpoint_text );
  }

  gtk_clist_thaw( GTK_CLIST( breakpoints ) );

  return 0;
}

static int
update_disassembly( void )
{
  size_t i, length; libspectrum_word address;
  char buffer[80];
  char *disassembly_text[2] = { &buffer[0], &buffer[40] };

  gtk_clist_freeze( GTK_CLIST( disassembly ) );
  gtk_clist_clear( GTK_CLIST( disassembly ) );

  for( i = 0, address = disassembly_top; i < 20; i++ ) {
    int l;
    snprintf( disassembly_text[0], 40, format_16_bit(), address );
    debugger_disassemble( disassembly_text[1], 40, &length, address );

    /* pad to 16 characters (long instruction) to avoid varying width */
    l = strlen( disassembly_text[1] );
    while( l < 16 ) disassembly_text[1][l++] = ' ';
    disassembly_text[1][l] = 0;

    address += length;

    gtk_clist_append( GTK_CLIST( disassembly ), disassembly_text );
  }
  gtk_clist_thaw( GTK_CLIST( disassembly ) );

  return 0;
}

static int
update_events( void )
{
  gtk_clist_freeze( GTK_CLIST( events ) );
  gtk_clist_clear( GTK_CLIST( events ) );

  event_foreach( add_event, NULL );

  gtk_clist_thaw( GTK_CLIST( events ) );

  return 0;
}

static void
add_event( gpointer data, gpointer user_data GCC_UNUSED )
{
  event_t *ptr = data;

  char buffer[80];
  char *event_text[2] = { &buffer[0], &buffer[40] };

  /* Skip events which have been removed */
  if( ptr->type == EVENT_TYPE_NULL ) return;

  snprintf( event_text[0], 40, "%d", ptr->tstates );
  strncpy( event_text[1], event_name( ptr->type ), 40 );

  gtk_clist_append( GTK_CLIST( events ), event_text );
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
ui_debugger_disassemble( libspectrum_word address )
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

static gboolean
delete_dialog( GtkWidget *widget, GdkEvent *event GCC_UNUSED,
	       gpointer user_data )
{
  gtkui_debugger_done_close( widget, user_data );
  return TRUE;
}

static void
gtkui_debugger_done_close( GtkWidget *widget, gpointer user_data GCC_UNUSED )
{
  gtk_widget_hide_all( widget );
  gtkui_debugger_done_continue( NULL, NULL );
}
