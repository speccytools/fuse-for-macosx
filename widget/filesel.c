/* filesel.c: File selection dialog box
   Copyright (c) 2001 Matan Ziv-Av, Philip Kendall, Russell Marks

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

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "display.h"
#include "keyboard.h"
#include "ui.h"
#include "uidisplay.h"
#include "widget.h"

struct dirent **widget_filenames; /* Filenames in the current
					    directory */
size_t widget_numfiles;	  /* The number of files in the current
			     directory */

/* The number of the filename in the top-left corner of the current
   display, that of the filename which the `cursor' is on, and that
   which it will be on after this keypress */
static int top_left_file, current_file, new_current_file;

static void widget_scan( char *dir );
static int widget_select_file(const struct dirent *dirent);
static int widget_scan_compare( const void *a, const void *b );

static char* widget_getcwd( void );
static int widget_print_all_filenames( struct dirent **filenames, int n,
				       int top_left, int current );
static int widget_print_filename( struct dirent *filename, int position,
				  int colour );
static void widget_selectfile_keyhandler( int key );

#ifndef HAVE_SCANDIR

int scandir( const char *dir, struct dirent ***namelist,
	     int (*select)(const struct dirent *),
	     int (*compar)(const struct dirent *,const struct dirent*))
{
  DIR *directory; struct dirent *dirent;

  int allocated, number;
  int i;

  (*namelist) = (struct dirent**)malloc( 32 * sizeof(struct dirent*) );
  if( *namelist == NULL ) return -1;

  allocated = 32; number = 0;

  directory = opendir( dir );
  if( directory == NULL ) {
    free( *namelist );
    *namelist = NULL;
    return -1;
  }

  while( 1 ) {
    errno = 0;
    dirent = readdir( directory );

    if( dirent == NULL ) {
      if( errno == 0 ) {	/* End of directory */
	break;
      } else {
	for( i=0; i<number; i++ ) free( (*namelist)[number] );
	free( *namelist );
	*namelist = NULL;
	closedir( directory );
	return -1;
      }
    }

    if( select( dirent ) ) {

      if( ++number > allocated ) {
	struct dirent **oldptr = *namelist;

	(*namelist)=
	  (struct dirent**)realloc( (*namelist),
				    2 * allocated * sizeof(struct dirent*) );
	if( *namelist == NULL ) {
	  for( i=0; i<number-1; i++ ) free( (*namelist)[number] );
	  free( oldptr );
	  closedir( directory );
	  return -1;
	}
	allocated *= 2;
      }

      (*namelist)[number-1] = (struct dirent*)malloc( sizeof(struct dirent) );
      if( (*namelist)[number-1] == NULL ) {
	for( i=0; i<number-1; i++ ) free( (*namelist)[number] );
	free( *namelist );
	return -1;
      }

      memcpy( (*namelist)[number-1], dirent, sizeof(struct dirent) );

    }

  }

  if( closedir( directory ) ) {
    for( i=0; i<number; i++ ) free( (*namelist)[number] );
    free( *namelist );
    *namelist = NULL;
    return -1;
  }

  if( compar != NULL ) { 
    qsort( (*namelist), number, sizeof(struct dirent*),
	   (int(*)(const void*,const void*))compar );
  }

  return number;

}
#endif				/* #ifndef HAVE_SCANDIR */

static void widget_scan( char *dir )
{
  size_t i;

  /* Free the memory belonging to the files in the previous directory */
  for( i=0; i<widget_numfiles; i++ ) free( widget_filenames[i] );

  widget_numfiles = scandir( dir, &widget_filenames,
			     widget_select_file, widget_scan_compare );
}

static int widget_select_file(const struct dirent *dirent){
  return( dirent->d_name && strcmp( dirent->d_name, "." ) );
}

static int widget_scan_compare( const void *a, const void *b )
{
  const char *name1 = (*(const struct dirent**)a)->d_name;
  const char *name2 = (*(const struct dirent**)b)->d_name;

  struct stat file_info;
  int isdir1, isdir2;

  int error;

  error = stat( name1, &file_info ); if( error ) return 0;
  isdir1 = S_ISDIR( file_info.st_mode );
  
  error = stat( name2, &file_info ); if( error ) return 0;
  isdir2 = S_ISDIR( file_info.st_mode );
  
  if( isdir1 && !isdir2 ) {
    return -1;
  } else if( isdir2 && !isdir1 ) {
    return 1;
  } else {
    return strcmp( name1, name2 );
  }

}

/* File selection widget */

const char* widget_selectfile( void )
{
  char *directory;

  int error;

  error = widget_timer_init();
  if( error ) return NULL;

  directory = widget_getcwd();
  if( directory == NULL ) return NULL;

  widget_scan( directory );
  current_file = 0;
  top_left_file = 0;
    
  /* A bright blue border */
  /* FIXME: Do this more efficiently! */
  widget_rectangle( 6, 6, 244, 180, 9 );

  /* And set up the key handler */
  widget_keyhandler = widget_selectfile_keyhandler;

  /* Show all the filenames */
  widget_print_all_filenames( widget_filenames, widget_numfiles,
			      top_left_file, current_file );

  /* And note we're in a widget */
  widget_active = 1;
  widget_finished = 0;

  while( 1 ) { 

    /* Go to sleep for a bit */
    pause();

    /* Now process any events which have occured; the important one
       here is a `keypress' event, which will call one of the
       widget_*_keyhandler functions, which may change new_current_file or
       set widget_finished */
    ui_event();

    /* If we're done, exit the loop */
    if( widget_finished ) break;

    /* If we did move the `cursor' */
    if( new_current_file != current_file ) {

      current_file = new_current_file;

      /* If we've got off the top or bottom of the currently
	 displayed file list, then reset the top-left corner and
	 display the whole thing */
      if( current_file <  top_left_file ) {

	top_left_file = current_file & ~1;
	widget_print_all_filenames( widget_filenames, widget_numfiles,
				    top_left_file, current_file );

      } else if( current_file >= top_left_file+36 ) {

	top_left_file = current_file & ~1; top_left_file -= 34;
	widget_print_all_filenames( widget_filenames, widget_numfiles,
				    top_left_file, current_file );

      } else {

	/* Otherwise, just reprint the new current file, display the
	   screen, and then print the current file back in red so
	   we don't have to do so later */

	widget_print_filename( widget_filenames[ current_file ],
			       current_file - top_left_file, 1 );
        
	uidisplay_lines(DISPLAY_BORDER_HEIGHT, DISPLAY_BORDER_HEIGHT+192);
	  
	widget_print_filename( widget_filenames[ current_file ],
			       current_file - top_left_file, 2 );
	  
      }
    }
  }

  /* We've finished with the widget */
  widget_active = 0;

  /* Deactivate the widget's timer */
  widget_timer_end();

  /* Force the normal screen to be redisplayed */
  display_refresh_all();

  /* Now return, either with a filename or without as appropriate */
  if( widget_finished == WIDGET_FINISHED_OK ) {
    return widget_filenames[ current_file ]->d_name;
  } else {
    return NULL;
  }

}

static char* widget_getcwd( void )
{
  char *directory; size_t directory_length;
  char *ptr;

  directory_length = 64;
  directory = (char*)malloc( directory_length * sizeof( char ) );
  if( directory == NULL ) {
    return NULL;
  }

  do {
    ptr = getcwd( directory, directory_length );
    if( ptr ) break;
    if( errno == ERANGE ) {
      ptr = directory;
      directory_length *= 2;
      directory =
	(char*)realloc( directory, directory_length * sizeof( char ) );
      if( directory == NULL ) {
	free( ptr );
	return NULL;
      }
    } else {
      free( directory );
      return NULL;
    }
  } while(1);

  return directory;
}

static int widget_print_all_filenames( struct dirent **filenames, int n,
				       int top_left, int current )
{
  int i;

  /* Give us a nice black box to start with */
  widget_rectangle( 8, 8, 240, 176, 8 );

  /* Print the filenames, mostly in red, but with the currently selected
     file in blue */
  for( i=top_left; i<n && i<top_left+36; i++ ) {
    if( i == current ) {
      widget_print_filename( filenames[i], i-top_left, 1 );
    } else {
      widget_print_filename( filenames[i], i-top_left, 2 );
    }
  }

  /* Display that lot */
  uidisplay_lines( DISPLAY_BORDER_HEIGHT,
		   DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT );

  /* Now print the currently selected file in red, so we don't have
     to worry about doing it later */
  widget_print_filename( filenames[ current_file ], current_file-top_left, 2 );

  return 0;
}

/* Print a filename onto the dialog box */
static int widget_print_filename( struct dirent *filename, int position,
				  int colour )
{
  char buffer[14];

  strncpy( buffer, filename->d_name, 13 ); buffer[13] = '\0';
  widget_printstring( 2 + ( position & 1 ) * 15, 3 + position/2,
		      colour, buffer );

  return 0;
}

static void widget_selectfile_keyhandler( int key )
{
  char *fn, *ptr;

  new_current_file = current_file;

  switch(key) {
  case KEYBOARD_1:		/* 1 used as `Escape' generates `EDIT',
				   which is Caps + 1 */
    widget_finished = WIDGET_FINISHED_CANCEL;
    break;
  
  case KEYBOARD_5:		/* Left */
    if( current_file > 0                 ) new_current_file--;
    break;

  case KEYBOARD_6:		/* Down */
    if( current_file < widget_numfiles-2 ) new_current_file += 2;
    break;

  case KEYBOARD_7:		/* Up */
    if( current_file > 1                 ) new_current_file -= 2;
    break;

  case KEYBOARD_8:		/* Right */
    if( current_file < widget_numfiles-1 ) new_current_file++;
    break;

  case KEYBOARD_Enter:
    /* Get the new directory name */
    fn = widget_getcwd();
    if( fn == NULL ) {
      widget_finished = WIDGET_FINISHED_CANCEL;
      return;
    }
    ptr = fn;
    fn = realloc( fn,
       ( strlen(fn) + 1 + strlen( widget_filenames[ current_file ]->d_name ) +
	 1 ) * sizeof(char)
    );
    if( fn == NULL ) {
      free( ptr );
      widget_finished = WIDGET_FINISHED_CANCEL;
      return;
    }
    strcat( fn, "/" ); strcat( fn, widget_filenames[ current_file ]->d_name );
			
    if(chdir(fn)==-1) {
      if(errno==ENOTDIR) {
	free( fn );
	widget_finished = WIDGET_FINISHED_OK;
      }
    } else {
      widget_scan( fn ); free( fn );
      new_current_file = 0;
      /* Force a redisplay of all filenames */
      current_file = 1; top_left_file = 1;
    }
    break;

  }
}
