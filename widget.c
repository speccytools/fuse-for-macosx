/* widget.c: Simple dialog boxes for all user interfaces.
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
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "fuse.h"
#include "display.h"
#include "machine.h"
#include "ui.h"
#include "uidisplay.h"
#include "keyboard.h"
#include "widget.h"

int widget_keymode;

static void printchar(int x, int y, int col, int ch);
static void printstring(int x, int y, int col, char *s);
static void rect(int x, int y, int w, int h, int col);

static char widget_font[768];

#define ERROR_MESSAGE_MAX_LENGTH 1024

static int widget_read_font( const char *filename, size_t offset )
{
  int fd; struct stat file_info; unsigned char *buffer;

  char error_message[ ERROR_MESSAGE_MAX_LENGTH ];

  fd = machine_find_rom( filename );
  if( fd == -1 ) {
    fprintf( stderr,"%s: couldn't find ROM `%s'", fuse_progname, filename );
    return 1;
  }

  if( fstat( fd, &file_info) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't stat `%s'", fuse_progname, filename );
    perror( error_message );
    close(fd);
    return errno;
  }

  buffer = mmap( 0, file_info.st_size, PROT_READ, MAP_SHARED, fd, 0 );
  if( buffer == (void*)-1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't mmap `%s'", fuse_progname, filename );
    perror( error_message );
    close(fd);
    return errno;
  }

  if( close(fd) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't close `%s'", fuse_progname, filename );
    perror( error_message );
    munmap( buffer, file_info.st_size );
    return errno;
  }

  memcpy( widget_font, buffer+offset-1, 768 );

  if( munmap( buffer, file_info.st_size ) == -1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't munmap `%s'", fuse_progname, filename );
    perror( error_message );
    return errno;
  }

  return 0;

}

static void printchar(int x, int y, int col, int ch) {
    
    int mx, my;

    if((ch<32)||(ch>127)) ch=33;
    ch-=32;
    
    for(my=0; my<8; my++) {
        int b;
        b=widget_font[ch*8+my];
        for(mx=0; mx<8; mx++) {
            if(b&128) uidisplay_putpixel(mx+DISPLAY_BORDER_WIDTH+8*x, my+DISPLAY_BORDER_HEIGHT+8*y, col);
            b<<=1;
        }
    }
}

static void printstring(int x, int y, int col, char *s) {
    int i;
    i=0;
    if(s) {
        while((x<32) && s[i]) {
            printchar(x,y,col,s[i]);
            i++;
            x++;
        }
    }
}

static void rect(int x, int y, int w, int h, int col) {
    int mx, my;
    for(my=0;my<h;my++)for(mx=0;mx<w;mx++)
        uidisplay_putpixel(mx+DISPLAY_BORDER_WIDTH+x, my+DISPLAY_BORDER_HEIGHT+y, col);
}

/* ------------------------------------------------------------------ */

static struct dirent **filenames; /* Filenames in the current directory */
static size_t numfiles;		  /* The number of files in the current
				     directory */

#ifndef HAVE_SCANDIR

int scandir( const char *dir, struct dirent ***namelist,
	     int (*select)(const struct dirent *),
	     int (*compar)(const struct dirent *, const struct dirent*))
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

static int select_file(const struct dirent *dirent);
static int widget_scan_compare( const void *a, const void *b );

static void scan( char *dir ) {
  size_t i;

  /* Free the memory belonging to the files in the previous directory */
  for( i=0; i<numfiles; i++ ) free( filenames[i] );

  numfiles = scandir( dir, &filenames, select_file, widget_scan_compare );
}

static int select_file(const struct dirent *dirent){
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

/* Are we in a widget at the moment? (Used by the key handling routines
   to know whether to call the Spectrum or widget key handler) */
int widget_active;

/* The keyhandler to use for the current widget */
widget_keyhandler_fn widget_keyhandler;

/* Two ways of finishing a widget */
typedef enum widget_finish_state {
  WIDGET_FINISHED_OK = 1,
  WIDGET_FINISHED_CANCEL,
} widget_finish_state;

/* Have we finished with this widget, and if so in which way? */
static widget_finish_state widget_finished;

/* Global initialisation/end routines */

int widget_init( void )
{
  int error;

  error = widget_read_font( "roms/48.rom", 15617 );
  if( error ) return error;

  numfiles = 0;

  return 0;
}

int widget_end( void )
{
  int i;

  for( i=0; i<numfiles; i++) free( filenames[i] );

  return 0;
}

/* Widget timer routines */

/* FIXME: * race conditions.
          * Do I really need a signal function here, or can I just ignore it?
	    Re-read Stevens.
*/

static struct sigaction widget_timer_old_handler;
static struct itimerval widget_timer_old_timer;

static void widget_timer_signal( int signo );

static int widget_timer_init( void )
{
  struct sigaction handler;
  struct itimerval timer;

  handler.sa_handler = widget_timer_signal;
  sigemptyset( &handler.sa_mask );
  handler.sa_flags = 0;
  sigaction( SIGALRM, &handler, &widget_timer_old_handler );

  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 100000UL;
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 100000UL;
  setitimer( ITIMER_REAL, &timer, &widget_timer_old_timer );

  return 0;
}

/* The signal handler: just wake up */
static void widget_timer_signal( int signo )
{
  return;
}

static int widget_timer_end( void )
{
  /* Restore the old timer */
  setitimer( ITIMER_REAL, &widget_timer_old_timer, NULL);

  /* And the old signal handler */
  sigaction( SIGALRM, &widget_timer_old_handler, NULL);

  return 0;
}

/* File selection widget */

/* The number of the filename in the top-left corner of the current
   display, that of the filename which the `cursor' is on, and that
   which it will be on after this keypress */
static int top_left_file, current_file, new_current_file;

static char* widget_getcwd( void );
static int widget_print_all_filenames( struct dirent **filenames, int n,
				       int top_left, int current );
static int widget_print_filename( struct dirent *filename, int position,
				  int colour );
static void widget_selectfile_keyhandler( int key );

const char* widget_selectfile( void )
{
  char *directory;

  int error;

  error = widget_timer_init();
  if( error ) return NULL;

  directory = widget_getcwd();
  if( directory == NULL ) return NULL;

  scan( directory );
  current_file = 0;
  top_left_file = 0;
    
  /* A bright blue border */
  /* FIXME: Do this more efficiently! */
  rect(6,6,244,180,9);

  /* And set up the key handler */
  widget_keyhandler = widget_selectfile_keyhandler;

  /* Show all the filenames */
  widget_print_all_filenames( filenames, numfiles,
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
	widget_print_all_filenames( filenames, numfiles,
				    top_left_file, current_file );

      } else if( current_file >= top_left_file+36 ) {

	top_left_file = current_file & ~1; top_left_file -= 34;
	widget_print_all_filenames( filenames, numfiles,
				    top_left_file, current_file );

      } else {

	/* Otherwise, just reprint the new current file, display the
	   screen, and then print the current file back in red so
	   we don't have to do so later */

	widget_print_filename( filenames[ current_file ],
			       current_file - top_left_file, 1 );
        
	uidisplay_lines(DISPLAY_BORDER_HEIGHT, DISPLAY_BORDER_HEIGHT+192);
	  
	widget_print_filename( filenames[ current_file ],
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
    return filenames[ current_file ]->d_name;
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
  rect( 8, 8, 240, 176, 8 );

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
  printstring( 2 + ( position & 1 ) * 15, 3 + position/2, colour, buffer );

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
    if( current_file > 0          ) new_current_file--;
    break;

  case KEYBOARD_6:		/* Down */
    if( current_file < numfiles-2 ) new_current_file += 2;
    break;

  case KEYBOARD_7:		/* Up */
    if( current_file > 1          ) new_current_file -= 2;
    break;

  case KEYBOARD_8:		/* Right */
    if( current_file < numfiles-1 ) new_current_file++;
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
       ( strlen(fn) + 1 + strlen( filenames[ current_file ]->d_name ) + 1 ) *
       sizeof(char)
    );
    if( fn == NULL ) {
      free( ptr );
      widget_finished = WIDGET_FINISHED_CANCEL;
      return;
    }
    strcat( fn, "/" ); strcat( fn, filenames[ current_file ]->d_name );
			
    if(chdir(fn)==-1) {
      if(errno==ENOTDIR) {
	free( fn );
	widget_finished = WIDGET_FINISHED_OK;
      }
    } else {
      scan( fn ); free( fn );
      new_current_file = 0;
      /* Force a redisplay of all filenames */
      current_file = 1; top_left_file = 1;
    }
    break;

  }
}
