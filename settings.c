/* settings.c: Handling configuration settings
   Copyright (c) 2001 Philip Kendall

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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "fuse.h"
#include "settings.h"

/* The current settings of options, etc */
settings_info settings_current;

static int settings_command_line( int argc, char **argv,
				  settings_info *settings );

/* Called on emulator startup */
int settings_init( int argc, char **argv )
{
  int error;

  error = settings_defaults( &settings_current );
  if( error ) return error;

  error = settings_command_line( argc, argv, &settings_current );
  if( error ) return error;

  return 0;
}

/* Fill the settings structure with sensible defaults */
int settings_defaults( settings_info *settings )
{
  settings->issue2 = 0;
  settings->joy_kempston = 0;
  settings->tape_traps = 1;
  settings->stereo_ay = 0;

  settings->snapshot = NULL;
  settings->tape_file = NULL;

  return 0;
}

/* Read options from the command line */
static int settings_command_line( int argc, char **argv,
				  settings_info *settings )
{

  struct option long_options[] = {

    {        "issue2", 0, &(settings->issue2), 1 },
    {     "no-issue2", 0, &(settings->issue2), 0 },

    {    "separation", 0, &(settings->stereo_ay), 1 },
    { "no-separation", 0, &(settings->stereo_ay), 0 },

    {      "kempston", 0, &(settings->joy_kempston), 1 },
    {   "no-kempston", 0, &(settings->joy_kempston), 0 },

    {         "traps", 0, &(settings->tape_traps), 1 },
    {      "no-traps", 0, &(settings->tape_traps), 0 },

    {      "snapshot", 1, NULL, 's' },
    {          "tape", 1, NULL, 't' },

    { 0, 0, 0, 0 }		/* End marker: DO NOT REMOVE */
  };

  while( 1 ) {

    int c = getopt_long( argc, argv, "", long_options, NULL );

    if( c == -1 ) break;	/* End of option list */

    switch( c ) {

    case 0: break;	/* Used for long option returns */

    case 's': settings->snapshot = optarg; break;
    case 't': settings->tape_file = optarg; break;

    case ':':
      fprintf( stderr, "%s: missing parameter on command line\n",
	       fuse_progname );
      break;

    case '?':
      break;

    default:
      fprintf( stderr, "%s: getopt_long returned `%c'\n",
	       fuse_progname, (char)c );
      break;

    }
  }

  return 0;
}

/* Copy one settings object to another */
int settings_copy( settings_info *dest, settings_info *src )
{
  dest->issue2       = src->issue2;
  dest->joy_kempston = src->joy_kempston;
  dest->tape_traps   = src->tape_traps;
  dest->stereo_ay    = src->stereo_ay;

  return 0;
}

    
