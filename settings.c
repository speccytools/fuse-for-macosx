/* settings.c: Handling configuration settings
   Copyright (c) 2001-2 Philip Kendall

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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#endif				/* #ifdef HAVE_GETOPT_LONG */

#ifdef HAVE_LIB_XML2
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#endif				/* #ifdef HAVE_LIB_XML2 */

#include "fuse.h"
#include "machine.h"
#include "settings.h"
#include "spectrum.h"
#include "ui/ui.h"

/* The current settings of options, etc */
settings_info settings_current;

#ifdef HAVE_LIB_XML2
static int read_config_file( settings_info *settings );
static int parse_xml( xmlDocPtr doc, settings_info *settings );
#endif				/* #ifdef HAVE_LIB_XML2 */

static int settings_command_line( int argc, char **argv,
				  settings_info *settings );

/* Called on emulator startup */
int settings_init( int argc, char **argv )
{
  int error;

  error = settings_defaults( &settings_current );
  if( error ) return error;

#ifdef HAVE_LIB_XML2
  error = read_config_file( &settings_current );
  if( error ) return error;
#endif				/* #ifdef HAVE_LIB_XML2 */

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
  settings->slt_traps = 1;
  
#ifdef HAVE_LIBZ
  settings->rzx_compression = 1;
#else			/* #ifdef HAVE_LIBZ */
  settings->rzx_compression = 0;
#endif			/* #ifdef HAVE_LIBZ */

  settings->sound_device = NULL;
  settings->sound = 1;
  settings->sound_load = 1;
  settings->stereo_ay = 0;
  settings->stereo_beeper = 0;

  settings->snapshot = NULL;
  settings->tape_file = NULL;

  settings->record_file = settings->playback_file = NULL;

  settings->start_machine = "48";

  settings->svga_mode = 320;

  return 0;
}

#ifdef HAVE_LIB_XML2

/* Read options from the user's config file (if libxml2 is available) */

static int
read_config_file( settings_info *settings )
{
  const char *home; char path[256];
  struct stat stat_info;

  xmlDocPtr doc;

  home = getenv( "HOME" );
  if( !home ) {
    ui_error( UI_ERROR_ERROR, "couldn't get your home directory" );
    return 1;
  }

  snprintf( path, 256, "%s/.fuserc", home );

  /* See if the file exists; if it doesn't, return without error */
  if( stat( path, &stat_info ) ) {
    if( errno == ENOENT ) {
      return 0;
    } else {
      ui_error( UI_ERROR_ERROR, "couldn't stat '%s': %s", path,
		strerror( errno ) );
      return 1;
    }
  }

  doc = xmlParseFile( path );
  if( !doc ) {
    ui_error( UI_ERROR_ERROR, "error reading config file" );
    return 1;
  }

  if( parse_xml( doc, settings ) ) { xmlFreeDoc( doc ); return 1; }

  xmlFreeDoc( doc );

  return 0;
}

static int
parse_xml( xmlDocPtr doc, settings_info *settings )
{
  xmlNodePtr node;

  node = xmlDocGetRootElement( doc );
  if( xmlStrcmp( node->name, (const xmlChar*)"settings" ) ) {
    ui_error( UI_ERROR_ERROR, "config file's root node is not 'settings'" );
    return 1;
  }

  node = node->xmlChildrenNode;
  while( node ) {

    /* FIXME: memory leak on string settings */

    if( !strcmp( node->name, (const xmlChar*)"issue2" ) ) {
      settings->issue2 =
	atoi( xmlNodeListGetString( doc, node->xmlChildrenNode, 1 ) );
    } else if( !strcmp( node->name, (const xmlChar*)"kempston" ) ) {
      settings->tape_traps =
	atoi( xmlNodeListGetString( doc, node->xmlChildrenNode, 1 ) );
    } else if( !strcmp( node->name, (const xmlChar*)"machine" ) ) {
      settings->start_machine =
	strdup( xmlNodeListGetString( doc, node->xmlChildrenNode, 1 ) );
    } else if( !strcmp( node->name, (const xmlChar*)"playbackfile" ) ) {
      settings->playback_file =
	strdup( xmlNodeListGetString( doc, node->xmlChildrenNode, 1 ) );
    } else if( !strcmp( node->name, (const xmlChar*)"recordfile" ) ) {
      settings->record_file =
	strdup( xmlNodeListGetString( doc, node->xmlChildrenNode, 1 ) );
    } else if( !strcmp( node->name, (const xmlChar*)"separation" ) ) {
      settings->stereo_ay =
	atoi( xmlNodeListGetString( doc, node->xmlChildrenNode, 1 ) );
    } else if( !strcmp( node->name, (const xmlChar*)"slt_traps" ) ) {
      settings->slt_traps =
	atoi( xmlNodeListGetString( doc, node->xmlChildrenNode, 1 ) );
    } else if( !strcmp( node->name, (const xmlChar*)"snapshot" ) ) {
      settings->snapshot =
	strdup( xmlNodeListGetString( doc, node->xmlChildrenNode, 1 ) );
    } else if( !strcmp( node->name, (const xmlChar*)"sounddevice" ) ) {
      settings->sound_device =
	strdup( xmlNodeListGetString( doc, node->xmlChildrenNode, 1 ) );
    } else if( !strcmp( node->name, (const xmlChar*)"svgamode" ) ) {
      settings->svga_mode =
	atoi( xmlNodeListGetString( doc, node->xmlChildrenNode, 1 ) );
    } else if( !strcmp( node->name, (const xmlChar*)"tapefile" ) ) {
      settings->tape_file =
	strdup( xmlNodeListGetString( doc, node->xmlChildrenNode, 1 ) );
    } else if( !strcmp( node->name, (const xmlChar*)"tapetraps" ) ) {
      settings->tape_traps =
	atoi( xmlNodeListGetString( doc, node->xmlChildrenNode, 1 ) );
    } else if( !strcmp( node->name, (const xmlChar*)"text" ) ) {
      /* Do nothing */
    } else {
      ui_error( UI_ERROR_ERROR, "Unknown setting '%s' in config file",
		node->name );
      return 1;
    }

    node = node->next;
  }

  return 0;
}

#endif				/* #ifdef HAVE_LIB_XML2 */

/* Read options from the command line */
static int settings_command_line( int argc, char **argv,
				  settings_info *settings )
{

#ifndef HAVE_GETOPT_LONG

    struct option {
      const char *name;
      int has_arg;
      int *flag;
      int val;
    };

#endif				/* #ifndef HAVE_GETOPT_LONG */

  struct option long_options[] = {

    {	    "machine", 1, NULL, 'm' },

    {      "snapshot", 1, NULL, 's' },
    {          "tape", 1, NULL, 't' },

    {      "playback", 1, NULL, 'p' },
    {        "record", 1, NULL, 'r' },

    {  "sound-device", 1, NULL, 'd' },

    {        "issue2", 0, &(settings->issue2), 1 },
    {     "no-issue2", 0, &(settings->issue2), 0 },

    {    "separation", 0, &(settings->stereo_ay), 1 },
    { "no-separation", 0, &(settings->stereo_ay), 0 },

    {           "slt", 0, &(settings->slt_traps), 1 },
    {        "no-slt", 0, &(settings->slt_traps), 0 },

    {      "kempston", 0, &(settings->joy_kempston), 1 },
    {   "no-kempston", 0, &(settings->joy_kempston), 0 },

    {         "traps", 0, &(settings->tape_traps), 1 },
    {      "no-traps", 0, &(settings->tape_traps), 0 },

    {          "help", 0, NULL, 'h' },
    {       "version", 0, NULL, 'V' },

    {     "svga-mode", 1, NULL, 'v' },

    { 0, 0, 0, 0 }		/* End marker: DO NOT REMOVE */
  };

  while( 1 ) {

    struct option *ptr;
    int c;

#ifdef HAVE_GETOPT_LONG
    c = getopt_long( argc, argv, "d:hm:o:p:r:s:t:v:V", long_options, NULL );
#else				/* #ifdef HAVE_GETOPT_LONG */
    c = getopt( argc, argv, "d:hm:o:p:r:s:t:v:V" );
#endif				/* #ifdef HAVE_GETOPT_LONG */

    if( c == -1 ) break;	/* End of option list */

    switch( c ) {

    case 0: break;	/* Used for long option returns */

    case 'd':
      settings->sound_device = optarg;
      break;

    case 'm':
      settings->start_machine = optarg;
      break;

    case 'o':
      for( ptr = long_options; ptr->name; ptr++ ) {
	
	if( ptr->flag == NULL ) continue;

	if( ! strcmp( optarg, ptr->name ) ) {
	  *(ptr->flag) = ptr->val;
	  break;
	}
      }
      break;

    case 'h': settings->show_help = 1; break;

    case 'p': settings->playback_file = optarg; break;
    case 'r': settings->record_file = optarg; break;

    case 's': settings->snapshot = optarg; break;
    case 't': settings->tape_file = optarg; break;

    case 'v': settings->svga_mode = atoi( optarg ); break;

    case 'V': settings->show_version = 1; break;

    case ':':
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
  dest->issue2        = src->issue2;
  dest->joy_kempston  = src->joy_kempston;
  dest->tape_traps    = src->tape_traps;
  dest->slt_traps     = src->slt_traps;

  dest->rzx_compression = src->rzx_compression;

  dest->sound_device  = src->sound_device;
  dest->sound	      = src->sound;
  dest->sound_load    = src->sound_load;
  dest->stereo_ay     = src->stereo_ay;
  dest->stereo_beeper = src->stereo_beeper;

  return 0;
}
