/* filesel.c: File selection dialog box
   Copyright (c) 2001-2004 Matan Ziv-Av, Philip Kendall, Russell Marks,
			   Marek Januszewski

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

#ifdef USE_WIDGET

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef WIN32
#include <windows.h>
#endif				/* #ifdef WIN32 */

#include "fuse.h"
/*  #include "keyboard.h" */
#include "widget_internals.h"

struct widget_dirent **widget_filenames; /* Filenames in the current
					    directory */
size_t widget_numfiles;	  /* The number of files in the current
			     directory */

/* The number of the filename in the top-left corner of the current
   display, that of the filename which the `cursor' is on, and that
   which it will be on after this keypress */
static size_t top_left_file, current_file, new_current_file;

static void widget_scan( char *dir );
static int widget_select_file( const struct dirent *dirent );
static int widget_scan_compare( const widget_dirent **a,
				const widget_dirent **b );

static char* widget_getcwd( void );
static int widget_print_all_filenames( struct widget_dirent **filenames, int n,
				       int top_left, int current );
static int widget_print_filename( struct widget_dirent *filename, int position,
				  int inverted );

/* The filename to return */
char* widget_filesel_name;

/* Should we exit all widgets when we're done with this selector? */
static int exit_all_widgets;

static int widget_scandir( const char *dir, struct widget_dirent ***namelist,
			   int (*select_fn)(const struct dirent*) )
{
  DIR *directory; struct dirent *dirent;

  int allocated, number;
  int i; size_t length;

  (*namelist) =
    (struct widget_dirent**)malloc( 32 * sizeof(struct widget_dirent*) );
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
	for( i=0; i<number; i++ ) {
	  free( (*namelist)[i]->name );
	  free( (*namelist)[i] );
	}
	free( *namelist );
	*namelist = NULL;
	closedir( directory );
	return -1;
      }
    }

    if( select_fn( dirent ) ) {

      if( ++number > allocated ) {
	struct widget_dirent **oldptr = *namelist;

	(*namelist) = (struct widget_dirent**)realloc(
	  (*namelist), 2 * allocated * sizeof(struct widget_dirent*)
	);
	if( *namelist == NULL ) {
	  for( i=0; i<number-1; i++ ) {
	    free( oldptr[i]->name );
	    free( oldptr[i] );
	  }
	  free( oldptr );
	  closedir( directory );
	  return -1;
	}
	allocated *= 2;
      }

      (*namelist)[number-1] =
	(struct widget_dirent*)malloc( sizeof(struct widget_dirent) );
      if( (*namelist)[number-1] == NULL ) {
	for( i=0; i<number-1; i++ ) {
	  free( (*namelist)[i]->name );
	  free( (*namelist)[i] );
	}
	free( *namelist );
	*namelist = NULL;
	closedir( directory );
	return -1;
      }

      length = strlen( dirent->d_name ) + 1;
      if( length < 16 ) length = 16;

      (*namelist)[number-1]->name = (char*)malloc( length * sizeof(char) );
      if( (*namelist)[number-1]->name == NULL ) {
	free( (*namelist)[number-1] );
	for( i=0; i<number-1; i++ ) {
	  free( (*namelist)[i]->name );
	  free( (*namelist)[i] );
	}
	free( *namelist );
      }

      strcpy( (*namelist)[number-1]->name, dirent->d_name );

    }

  }

  if( closedir( directory ) ) {
    for( i=0; i<number; i++ ) {
      free( (*namelist)[i]->name );
      free( (*namelist)[i] );
    }
    free( *namelist );
    *namelist = NULL;
    return -1;
  }

  return number;

}

static void widget_scan( char *dir )
{
  struct stat file_info;

  size_t i; int error;
  
  /* Free the memory belonging to the files in the previous directory */
  for( i=0; i<widget_numfiles; i++ ) {
    free( widget_filenames[i]->name );
    free( widget_filenames[i] );
  }

  widget_numfiles = widget_scandir( dir, &widget_filenames,
				    widget_select_file );
  if( widget_numfiles == (size_t)-1 ) return;

  for( i=0; i<widget_numfiles; i++ ) {
    error = stat( widget_filenames[i]->name, &file_info );
    widget_filenames[i]->mode = error ? 0 : file_info.st_mode;
  }

  qsort( widget_filenames, widget_numfiles, sizeof(struct widget_dirent*),
	 (int(*)(const void*,const void*))widget_scan_compare );

}

static int widget_select_file(const struct dirent *dirent){
  return( dirent->d_name && strcmp( dirent->d_name, "." ) );
}

static int widget_scan_compare( const struct widget_dirent **a,
				const struct widget_dirent **b )
{
  int isdir1 = S_ISDIR( (*a)->mode ),
      isdir2 = S_ISDIR( (*b)->mode );

  if( isdir1 && !isdir2 ) {
    return -1;
  } else if( isdir2 && !isdir1 ) {
    return 1;
  } else {
    return strcmp( (*a)->name, (*b)->name );
  }

}

/* File selection widget */

int
widget_filesel_draw( void *data )
{
  char *directory;

  int error;

  /* Should we exit all widgets when we're done with the fileselector;
     defaults to 'yes' if no data given for backward compatability reasons */
  if( !data || *(int*)data ) {
    exit_all_widgets = 1;
  } else {
    exit_all_widgets = 0;
  }

  directory = widget_getcwd();
  if( directory == NULL ) return 1;

  widget_scan( directory );
  new_current_file = current_file = 0;
  top_left_file = 0;

  /* Create the dialog box */
  error = widget_dialog_with_border( 1, 2, 30, 20 );
  if( error ) { free( directory ); return error; }
    
  /* Show all the filenames */
  widget_print_all_filenames( widget_filenames, widget_numfiles,
			      top_left_file, current_file );

  return 0;
}

int widget_filesel_finish( widget_finish_state finished ) {

  /* Return with null if we didn't finish cleanly */
  if( finished != WIDGET_FINISHED_OK ) {
    if( widget_filesel_name ) free( widget_filesel_name );
    widget_filesel_name = NULL;
  }

  return 0;
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

static int widget_print_all_filenames( struct widget_dirent **filenames, int n,
				       int top_left, int current )
{
  int i;
  int error;

  /* Give us a clean box to start with */
  error = widget_dialog( 1, 2, 30, 20 );
  if( error ) return error;

  /* Print the filenames, mostly normally, but with the currently
     selected file inverted */
  for( i=top_left; i<n && i<top_left+36; i++ ) {
    if( i == current ) {
      widget_print_filename( filenames[i], i-top_left, 1 );
    } else {
      widget_print_filename( filenames[i], i-top_left, 0 );
    }
  }

  /* Display that lot */
  widget_display_lines( 2, 20 );

  return 0;
}

/* Print a filename onto the dialog box */
static int widget_print_filename( struct widget_dirent *filename, int position,
				  int inverted )
{
  char buffer[14];

  int x = (position & 1) ? 17 : 2,
      y = 3 + position/2;

  int foreground = inverted ? WIDGET_COLOUR_BACKGROUND
                            : WIDGET_COLOUR_FOREGROUND,

      background = inverted ? WIDGET_COLOUR_FOREGROUND
                            : WIDGET_COLOUR_BACKGROUND;

  widget_rectangle( 8 * x, 8 * y, 8 * 13, 8, background );

  strncpy( buffer, filename->name, 13 ); buffer[13] = '\0';

  if( S_ISDIR( filename->mode ) ) {
    if( strlen( buffer ) >= 13 ) {
      buffer[12] = '/';
    } else {
      strcat( buffer, "/" );
    }
  }

  widget_printstring( x, y, foreground, buffer );

  return 0;
}

void
widget_filesel_keyhandler( input_key key )
{
  char *fn, *ptr;

  new_current_file = current_file;

  switch(key) {

#if 0
  case INPUT_KEY_Resize:	/* Fake keypress used on window resize */
    widget_dialog_with_border( 1, 2, 30, 20 );
    widget_print_all_filenames( widget_filenames, widget_numfiles,
				top_left_file, current_file        );
    break;
#endif
    
  case INPUT_KEY_Escape:
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    break;
  
  case INPUT_KEY_Left:
  case INPUT_KEY_5:
  case INPUT_KEY_h:
    if( current_file > 0                 ) new_current_file--;
    break;

  case INPUT_KEY_Down:
  case INPUT_KEY_6:
  case INPUT_KEY_j:
    if( current_file < widget_numfiles-2 ) new_current_file += 2;
    break;

  case INPUT_KEY_Up:
  case INPUT_KEY_7:		/* Up */
  case INPUT_KEY_k:
    if( current_file > 1                 ) new_current_file -= 2;
    break;

  case INPUT_KEY_Right:
  case INPUT_KEY_8:
  case INPUT_KEY_l:
    if( current_file < widget_numfiles-1 ) new_current_file++;
    break;

  case INPUT_KEY_Page_Up:
    new_current_file = ( current_file > 36 ) ?
                       current_file - 36     :
                       0;
    break;

  case INPUT_KEY_Page_Down:
    new_current_file = current_file + 36;
    if( new_current_file >= widget_numfiles )
      new_current_file = widget_numfiles - 1;
    break;

  case INPUT_KEY_Home:
    new_current_file = 0;
    break;

  case INPUT_KEY_End:
    new_current_file = widget_numfiles - 1;
    break;

  case INPUT_KEY_Return:
    /* Get the new directory name */
    fn = widget_getcwd();
    if( fn == NULL ) {
      widget_end_widget( WIDGET_FINISHED_CANCEL );
      return;
    }
    ptr = fn;
    fn = realloc( fn,
       ( strlen(fn) + 1 + strlen( widget_filenames[ current_file ]->name ) +
	 1 ) * sizeof(char)
    );
    if( fn == NULL ) {
      free( ptr );
      widget_end_widget( WIDGET_FINISHED_CANCEL );
      return;
    }
    strcat( fn, "/" ); strcat( fn, widget_filenames[ current_file ]->name );
			
/*
  in Win32 errno resulting from chdir on file is EINVAL which may mean many things
  this will not be fixed in mingw - must use native function instead
  http://thread.gmane.org/gmane.comp.gnu.mingw.user/9197
*/ 

    if(chdir(fn)==-1) {
#ifndef WIN32
      if(errno==ENOTDIR) {
#else
      if( GetFileAttributes( fn ) != FILE_ATTRIBUTE_DIRECTORY	) {
#endif
	widget_filesel_name = fn;
	if( exit_all_widgets ) {
	  widget_end_all( WIDGET_FINISHED_OK );
	} else {
	  widget_end_widget( WIDGET_FINISHED_OK );
	}
      }
    } else {
      widget_scan( fn ); free( fn );
      new_current_file = 0;
      /* Force a redisplay of all filenames */
      current_file = 1; top_left_file = 1;
    }
    break;

  default:	/* Keep gcc happy */
    break;

  }

  /* If we moved the cursor */
  if( new_current_file != current_file ) {

    /* If we've got off the top or bottom of the currently displayed
       file list, then reset the top-left corner and display the whole
       thing */
    if( new_current_file < top_left_file ) {

      top_left_file = new_current_file & ~1;
      widget_print_all_filenames( widget_filenames, widget_numfiles,
				  top_left_file, new_current_file );

    } else if( new_current_file >= top_left_file+36 ) {

      top_left_file = new_current_file & ~1; top_left_file -= 34;
      widget_print_all_filenames( widget_filenames, widget_numfiles,
				  top_left_file, new_current_file );

    } else {

      /* Otherwise, print the current file uninverted and the
	 new current file inverted */

      widget_print_filename( widget_filenames[ current_file ],
			     current_file - top_left_file, 0 );
	  
      widget_print_filename( widget_filenames[ new_current_file ],
			     new_current_file - top_left_file, 1 );
        
      widget_display_lines( 2, 20 );
    }

    /* Reset the current file marker */
    current_file = new_current_file;

  }

}

#endif				/* #ifdef USE_WIDGET */
