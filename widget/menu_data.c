/* menu_data.c: Data for the widget menus
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

#include <stdlib.h>

#include "rzx.h"
#include "snapshot.h"
#include "specplus3.h"
#include "tape.h"
#include "widget.h"

/* FIXME: there must be a better way of initialising all the menu data */

static widget_menu_entry widget_menu_file[];
static widget_menu_entry widget_menu_file_recording[];
static widget_menu_entry widget_menu_machine[];
static widget_menu_entry widget_menu_options[];
static widget_menu_entry widget_menu_tape[];

#ifdef HAVE_765_H
static widget_menu_entry widget_menu_disk[];
static widget_menu_entry widget_menu_disk_a[];
static widget_menu_entry widget_menu_disk_b[];
#endif					/* #ifdef HAVE_765_H */

static widget_menu_entry widget_menu_help[];

/* Main menu */

static widget_menu_widget_t main_file =    { WIDGET_TYPE_MENU,
					     &widget_menu_file    };
static widget_menu_widget_t main_options = { WIDGET_TYPE_MENU,
					     &widget_menu_options };
static widget_menu_widget_t main_machine = { WIDGET_TYPE_MENU, 
					     &widget_menu_machine };
static widget_menu_widget_t main_tape =    { WIDGET_TYPE_MENU,
					     &widget_menu_tape    };

#ifdef HAVE_765_H
static widget_menu_widget_t main_disk =    { WIDGET_TYPE_MENU,
					     &widget_menu_disk    };
#endif

static widget_menu_widget_t main_help =    { WIDGET_TYPE_MENU,
					     &widget_menu_help    };

widget_menu_entry widget_menu_main[] = {
  { "Main menu", 0, 0, NULL },		/* Menu title */

  { "(F)ile",    KEYBOARD_f, widget_menu_widget, &main_file    },
  { "(O)ptions", KEYBOARD_o, widget_menu_widget, &main_options },
  { "(M)achine", KEYBOARD_m, widget_menu_widget, &main_machine },
  { "(T)ape",	 KEYBOARD_t, widget_menu_widget, &main_tape    },

#ifdef HAVE_765_H
  { "(D)isk",	 KEYBOARD_d, widget_menu_widget, &main_disk    },
#endif					/* #ifdef HAVE_765_H */

  { "(H)elp",    KEYBOARD_h, widget_menu_widget, &main_help    },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* File menu */

static widget_menu_widget_t file_recording = { WIDGET_TYPE_MENU,
					       &widget_menu_file_recording };

static widget_menu_entry widget_menu_file[] = {
  { "File", 0, 0, NULL },		/* Menu title */

  { "(O)pen snapshot...",       KEYBOARD_o, widget_apply_to_file,
                                            snapshot_read                   },
  { "(S)ave to 'snapshot.z80'", KEYBOARD_s, widget_menu_save_snapshot, NULL },
  { "(R)ecording",		KEYBOARD_r, widget_menu_widget,
					    &file_recording                 },
  { "E(x)it",			KEYBOARD_x, widget_menu_exit,          NULL },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* File/Recording menu */

static widget_menu_entry widget_menu_file_recording[] = {
  { "Recording", 0, 0, NULL },		/* Menu title */

  { "(R)ecord...", KEYBOARD_r, widget_menu_rzx_recording,  NULL },
  { "(P)lay...",   KEYBOARD_p, widget_menu_rzx_playback,   NULL },
  { "(S)top",	   KEYBOARD_s, widget_menu_rzx_stop,       NULL },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* Options menu */

static widget_menu_widget_t options_general = { WIDGET_TYPE_GENERAL, NULL };
static widget_menu_widget_t options_sound   = { WIDGET_TYPE_SOUND,   NULL };

static widget_menu_entry widget_menu_options[] = {
  { "Options", 0, 0, NULL },		/* Menu title */

  { "(G)eneral...", KEYBOARD_g, widget_menu_widget, &options_general },
  { "(S)ound...",   KEYBOARD_s, widget_menu_widget, &options_sound   },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* Machine menu */

static widget_menu_widget_t machine_sel = { WIDGET_TYPE_SELECT, NULL };

static widget_menu_entry widget_menu_machine[] = {
  { "Machine", 0, 0, NULL },		/* Menu title */

  { "(R)eset",     KEYBOARD_r, widget_menu_reset,  NULL         },
  { "(S)elect...", KEYBOARD_s, widget_menu_widget, &machine_sel },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* Tape menu */

static widget_menu_entry widget_menu_tape[] = {
  { "Tape", 0, 0, NULL },		/* Menu title */

  { "(O)pen tape...",           KEYBOARD_o, widget_apply_to_file,    
                                            tape_open                     },
  { "(P)lay tape",              KEYBOARD_p, widget_menu_play_tape,   NULL },
  { "(R)ewind tape",            KEYBOARD_r, widget_menu_rewind_tape, NULL },
  { "(C)lear tape",             KEYBOARD_c, widget_menu_clear_tape,  NULL },
  { "Write tape to 'tape.tzx'", KEYBOARD_w, widget_menu_write_tape,  NULL },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

#ifdef HAVE_765_H

/* Disk menu */

static widget_menu_widget_t disk_a = { WIDGET_TYPE_MENU, &widget_menu_disk_a };
static widget_menu_widget_t disk_b = { WIDGET_TYPE_MENU, &widget_menu_disk_b };

static widget_menu_entry widget_menu_disk[] = {
  { "Disk", 0, 0, NULL },		/* Menu title */

  { "Drive (A):", KEYBOARD_a, widget_menu_widget, &disk_a },
  { "Drive (B):", KEYBOARD_b, widget_menu_widget, &disk_b },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* Disk/Drive A: menu */

static specplus3_drive_number disk_a_number = SPECPLUS3_DRIVE_A;

static widget_menu_entry widget_menu_disk_a[] = {
  { "Disk/Drive A:", 0, 0, NULL },	/* Menu title */

  { "(I)nsert...", KEYBOARD_i, widget_apply_to_file,   widget_insert_disk_a },
  { "(E)ject",	   KEYBOARD_e, widget_menu_eject_disk, &disk_a_number       },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* Disk/Drive B: menu */

static specplus3_drive_number disk_b_number = SPECPLUS3_DRIVE_B;

static widget_menu_entry widget_menu_disk_b[] = {
  { "Disk/Drive A:", 0, 0, NULL },	/* Menu title */

  { "(I)nsert...", KEYBOARD_i, widget_apply_to_file,   widget_insert_disk_b },
  { "(E)ject",	   KEYBOARD_e, widget_menu_eject_disk, &disk_b_number       },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

#endif					/* #ifdef HAVE_765_H */

/* Help menu */

static widget_picture_data help_keyboard = { "keyboard.scr", NULL, 0 };

static widget_menu_entry widget_menu_help[] = {
  { "Help", 0, 0, NULL },		/* Menu title */

  { "(K)eyboard...", KEYBOARD_k, widget_menu_keyboard, &help_keyboard },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};
