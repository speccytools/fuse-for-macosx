/* menu_data.c: Data for the widget menus
   Copyright (c) 2002-2004 Philip Kendall

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

#include <stdlib.h>

#include "rzx.h"
#include "screenshot.h"
#include "snapshot.h"
#include "specplus3.h"
#include "tape.h"
#include "ui/scaler/scaler.h"
#include "widget_internals.h"

/* FIXME: there must be a better way of initialising all the menu data */

static widget_menu_entry widget_menu_file[];
static widget_menu_entry widget_menu_file_recording[];
static widget_menu_entry file_aylogging[];

static widget_menu_entry widget_menu_machine[];

static widget_menu_entry widget_menu_options[];
static widget_menu_entry widget_menu_roms[];

static widget_menu_entry widget_menu_media[];
static widget_menu_entry widget_menu_tape[];
static widget_menu_entry widget_menu_disk[];
static widget_menu_entry widget_menu_disk_a[];
static widget_menu_entry widget_menu_disk_b[];
static widget_menu_entry widget_menu_cart[];

static widget_menu_entry widget_menu_help[];

/* Main menu */

static widget_menu_widget_t main_file =    { WIDGET_TYPE_MENU,
					     &widget_menu_file    };
static widget_menu_widget_t main_options = { WIDGET_TYPE_MENU,
					     &widget_menu_options };
static widget_menu_widget_t main_machine = { WIDGET_TYPE_MENU, 
					     &widget_menu_machine };
static widget_menu_widget_t main_media =   { WIDGET_TYPE_MENU, 
					     &widget_menu_media };
static widget_menu_widget_t main_help =    { WIDGET_TYPE_MENU,
					     &widget_menu_help    };

widget_menu_entry widget_menu_main[] = {
  { "Main menu", 0, 0, NULL },		/* Menu title */

  { "(F)ile",    INPUT_KEY_f, widget_menu_widget, &main_file    },
  { "(O)ptions", INPUT_KEY_o, widget_menu_widget, &main_options },
  { "(M)achine", INPUT_KEY_m, widget_menu_widget, &main_machine },
  { "M(e)dia",   INPUT_KEY_e, widget_menu_widget, &main_media   },

  { "(H)elp",    INPUT_KEY_h, widget_menu_widget, &main_help    },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* File menu */

static widget_menu_widget_t
  file_recording = { WIDGET_TYPE_MENU, &widget_menu_file_recording };

static widget_menu_entry widget_menu_file[] = {
  { "File", 0, 0, NULL },		/* Menu title */

  { "(O)pen...",	        INPUT_KEY_o, widget_apply_to_file,
					     widget_menu_open		     },
  { "(S)ave to 'snapshot.z80'", INPUT_KEY_s, widget_menu_save_snapshot, NULL },
  { "(R)ecording",		INPUT_KEY_r, widget_menu_widget,
					     &file_recording                 },
  { "A(Y) Logging",		INPUT_KEY_y, widget_menu_widget,
					     &file_aylogging                 },
  { "O(p)en SCR screenshot...", INPUT_KEY_p, widget_apply_to_file,
                                             screenshot_scr_read             },
  { "S(a)ve Screen to 'fuse.scr'", INPUT_KEY_a, widget_menu_save_scr,   NULL },

#ifdef USE_LIBPNG
  { "Save S(c)reen to 'fuse.png'",INPUT_KEY_c, widget_menu_save_screen, NULL },
#endif				/* #ifdef USE_LIBPNG */

  { "E(x)it",			INPUT_KEY_x, widget_menu_exit,          NULL },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* File/Recording menu */

static widget_menu_entry widget_menu_file_recording[] = {
  { "Recording", 0, 0, NULL },		/* Menu title */

  { "(R)ecord...", INPUT_KEY_r, widget_menu_rzx_recording,  NULL },
  { "Record (f)rom snap...", INPUT_KEY_f, widget_menu_rzx_recording_snap,
								        NULL },
  { "(P)lay...",   INPUT_KEY_p, widget_menu_rzx_playback,   NULL },
  { "(S)top",	   INPUT_KEY_s, widget_menu_rzx_stop,       NULL },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* File/AY Logging menu */

static widget_menu_entry file_aylogging[] = {
  { "AY Logging", 0, 0, NULL },		/* Menu title */

  { "(R)ecord...", INPUT_KEY_r, widget_menu_psg_record, NULL },
  { "(S)top",	   INPUT_KEY_s, widget_menu_psg_stop,   NULL },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* Options menu */

static widget_menu_widget_t options_general = { WIDGET_TYPE_GENERAL, NULL };
static widget_menu_widget_t options_sound   = { WIDGET_TYPE_SOUND,   NULL };
static widget_menu_widget_t options_rzx     = { WIDGET_TYPE_RZX,     NULL };
static widget_menu_widget_t options_roms    = { WIDGET_TYPE_MENU,
						&widget_menu_roms };

static widget_menu_entry widget_menu_options[] = {
  { "Options", 0, 0, NULL },		/* Menu title */

  { "(G)eneral...", INPUT_KEY_g, widget_menu_widget, &options_general },
  { "(S)ound...",   INPUT_KEY_s, widget_menu_widget, &options_sound   },
  { "(R)ZX...",	    INPUT_KEY_r, widget_menu_widget, &options_rzx     },
  { "S(e)lect ROMS...",
                    INPUT_KEY_e, widget_menu_widget, &options_roms    },

#if defined( UI_SDL ) || defined( UI_X )
  { "(F)ilter...",  INPUT_KEY_f, widget_menu_filter, NULL             },
#endif				/* #if defined( UI_SDL ) || defined( UI_X ) */

#ifdef HAVE_LIB_XML2
  { "S(a)ve",	    INPUT_KEY_a, widget_menu_save_options, NULL       },
#endif				/* #ifdef HAVE_LIB_XML2 */

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* Options/Select ROMs menu */

static libspectrum_machine
  spectrum_16 = LIBSPECTRUM_MACHINE_16,
  spectrum_48 = LIBSPECTRUM_MACHINE_48,
  spectrum_128 = LIBSPECTRUM_MACHINE_128,
  spectrum_plus2 = LIBSPECTRUM_MACHINE_PLUS2,
  spectrum_plus2a = LIBSPECTRUM_MACHINE_PLUS2A,
  spectrum_plus3 = LIBSPECTRUM_MACHINE_PLUS3,
  timex_tc2048 = LIBSPECTRUM_MACHINE_TC2048,
  timex_tc2068 = LIBSPECTRUM_MACHINE_TC2068,
  pentagon = LIBSPECTRUM_MACHINE_PENT,
  scorpion = LIBSPECTRUM_MACHINE_SCORP;

static widget_menu_entry widget_menu_roms[] = {
  { "Select ROMs", 0, 0, NULL },	/* Menu title */

  { "Spectrum 1(6)K...", INPUT_KEY_6, widget_menu_select_roms, &spectrum_16 },
  { "Spectrum (4)8K...", INPUT_KEY_4, widget_menu_select_roms, &spectrum_48 },
  { "Spectrum 12(8)K...", INPUT_KEY_8, widget_menu_select_roms,
							       &spectrum_128 },
  { "Spectrum +(2)...", INPUT_KEY_2, widget_menu_select_roms,
							     &spectrum_plus2 },
  { "Spectrum +2(A)...", INPUT_KEY_a, widget_menu_select_roms,
                                                            &spectrum_plus2a },
  { "Spectrum +(3)...", INPUT_KEY_3, widget_menu_select_roms,
							     &spectrum_plus3 },
  { "Timex (T)C2048...", INPUT_KEY_t, widget_menu_select_roms, &timex_tc2048 },
  { "Timex T(C)2068...", INPUT_KEY_c, widget_menu_select_roms, &timex_tc2068 },
  { "(P)entagon 128K...", INPUT_KEY_p, widget_menu_select_roms, &pentagon },
  { "(S)corpion ZS 256...", INPUT_KEY_s, widget_menu_select_roms, &scorpion },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* Machine menu */

static widget_menu_widget_t machine_sel = { WIDGET_TYPE_SELECT, NULL };

static widget_menu_entry widget_menu_machine[] = {
  { "Machine", 0, 0, NULL },		/* Menu title */

  { "(R)eset",     INPUT_KEY_r, widget_menu_reset,  NULL         },
  { "(S)elect...", INPUT_KEY_s, widget_menu_widget, &machine_sel },
  { "(D)ebugger...", INPUT_KEY_d, widget_menu_break, NULL        },
  { "(N)MI",       INPUT_KEY_n, widget_menu_nmi,    NULL         },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

static widget_menu_widget_t media_tape = { WIDGET_TYPE_MENU,
					   &widget_menu_tape };
static widget_menu_widget_t media_disk = { WIDGET_TYPE_MENU,
					   &widget_menu_disk };
static widget_menu_widget_t media_cart = { WIDGET_TYPE_MENU,
					   &widget_menu_cart };

static widget_menu_entry widget_menu_media[] = {
  { "Media", 0, 0, NULL },		/* Menu title */

  { "(T)ape",      INPUT_KEY_t, widget_menu_widget, &media_tape },
  { "(D)isk",      INPUT_KEY_d, widget_menu_widget, &media_disk },
  { "(C)artridge", INPUT_KEY_c, widget_menu_widget, &media_cart },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* Media/Tape menu */

static widget_menu_widget_t tape_browse = { WIDGET_TYPE_BROWSE, NULL };

static widget_menu_entry widget_menu_tape[] = {
  { "Tape", 0, 0, NULL },		/* Menu title */

  { "(O)pen tape...",          INPUT_KEY_o, widget_apply_to_file,    
                                               tape_open_default_autoload },
  { "(P)lay tape",             INPUT_KEY_p, widget_menu_play_tape,   NULL },
  { "(B)rowse tape...",	       INPUT_KEY_b, widget_menu_widget, &tape_browse },
  { "(R)ewind tape",           INPUT_KEY_r, widget_menu_rewind_tape, NULL },
  { "(C)lear tape",            INPUT_KEY_c, widget_menu_clear_tape,  NULL },
  { "(W)rite tape to 'tape.tzx'", INPUT_KEY_w, widget_menu_write_tape, NULL },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* Media/Disk menu */

static widget_menu_widget_t disk_a = { WIDGET_TYPE_MENU, &widget_menu_disk_a };
static widget_menu_widget_t disk_b = { WIDGET_TYPE_MENU, &widget_menu_disk_b };

static widget_menu_entry widget_menu_disk[] = {
  { "Disk", 0, 0, NULL },		/* Menu title */

  { "Drive (A):", INPUT_KEY_a, widget_menu_widget, &disk_a },
  { "Drive (B):", INPUT_KEY_b, widget_menu_widget, &disk_b },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* Media/Disk/Drive A: menu */

static int disk_a_number = 0;

static widget_menu_entry widget_menu_disk_a[] = {
  { "Disk/Drive A:", 0, 0, NULL },	/* Menu title */

  { "(I)nsert...", INPUT_KEY_i, widget_apply_to_file,   widget_insert_disk_a },
  { "(E)ject",	   INPUT_KEY_e, widget_menu_eject_disk, &disk_a_number       },
  { "Eject and (w)rite...", INPUT_KEY_w, widget_menu_eject_write_disk,
						        &disk_a_number       },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* Media/Disk/Drive B: menu */

static int disk_b_number = 1;

static widget_menu_entry widget_menu_disk_b[] = {
  { "Disk/Drive B:", 0, 0, NULL },	/* Menu title */

  { "(I)nsert...", INPUT_KEY_i, widget_apply_to_file,   widget_insert_disk_b },
  { "(E)ject",	   INPUT_KEY_e, widget_menu_eject_disk, &disk_b_number       },
  { "Eject and (w)rite...", INPUT_KEY_w, widget_menu_eject_write_disk,
						        &disk_b_number       },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* Media/Cartridge menu */

static widget_menu_entry widget_menu_cart[] = {
  { "Cartridge/Timex Dock", 0, 0, NULL },	/* Menu title */

  { "(I)nsert...", INPUT_KEY_i, widget_apply_to_file,   widget_insert_dock },
  { "(E)ject",	   INPUT_KEY_e, widget_menu_eject_dock, NULL		   },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

/* Help menu */

static widget_picture_data help_keyboard = { "keyboard.scr", NULL, 0 };

static widget_menu_entry widget_menu_help[] = {
  { "Help", 0, 0, NULL },		/* Menu title */

  { "(K)eyboard...", INPUT_KEY_k, widget_menu_keyboard, &help_keyboard },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

#endif				/* #ifdef USE_WIDGET */
