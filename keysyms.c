/* keysyms.c: keysym to Spectrum key mappings for both Xlib and GDK
   Copyright (c) 2000-2001 Philip Kendall, Matan Ziv-Av, Russell Marks

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
   Foundation, Inc., 49 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <stdlib.h>

#include "keysyms.h"

/* Work out which UI we're using and get the appropriate header file. As
   GTK+ and Xlib use the same keysyms (only the prefix is different), define
   a small macro to mean I don't have to repeat the table...

   However, the svgalib keysyms are different enough that I need a separate
   table.

   FIXME: Need some way to keep the two tables consistent
*/
#if defined( UI_GTK )
#include <gdk/gdkkeysyms.h>
#define KEY(keysym) GDK_ ## keysym
#elif defined( UI_X )
#include <X11/keysym.h>
#define KEY(keysym) XK_ ## keysym
#elif defined( UI_SVGA )
#include <vgakeyboard.h>
#elif defined( UI_FB )
#else
#error No user interface selected
#endif

/* The mappings from keysyms to Spectrum keys. The keysym `keysym'
   maps to one or two Spectrum keys, specified by {port,bit}{1,2}. 
   Unused second keys are specified by having `bit2' being zero

   These mappings are ordered basically like a standard (English) PC keyboard,
   top to bottom, left to right, but with a few additions for other keys
*/

#if defined( UI_GTK ) || defined( UI_X )

static keysyms_key_info keysyms_data[] = {

  { KEY(1)          , KEYBOARD_1,      KEYBOARD_NONE },
  { KEY(2)          , KEYBOARD_2,      KEYBOARD_NONE },
  { KEY(3)          , KEYBOARD_3,      KEYBOARD_NONE },
  { KEY(4)          , KEYBOARD_4,      KEYBOARD_NONE },
  { KEY(5)          , KEYBOARD_5,      KEYBOARD_NONE },
  { KEY(6)          , KEYBOARD_6,      KEYBOARD_NONE },
  { KEY(7)          , KEYBOARD_7,      KEYBOARD_NONE },
  { KEY(8)          , KEYBOARD_8,      KEYBOARD_NONE },
  { KEY(9)          , KEYBOARD_9,      KEYBOARD_NONE },
  { KEY(0)          , KEYBOARD_0,      KEYBOARD_NONE },
  { KEY(minus)      , KEYBOARD_Symbol, KEYBOARD_j    },
  { KEY(equal)      , KEYBOARD_Symbol, KEYBOARD_l    },
  { KEY(BackSpace)  , KEYBOARD_Caps,   KEYBOARD_0    },

  { KEY(q)          , KEYBOARD_q,      KEYBOARD_NONE },
  { KEY(w)          , KEYBOARD_w,      KEYBOARD_NONE },
  { KEY(e)          , KEYBOARD_e,      KEYBOARD_NONE },
  { KEY(r)          , KEYBOARD_r,      KEYBOARD_NONE },
  { KEY(t)          , KEYBOARD_t,      KEYBOARD_NONE },
  { KEY(y)          , KEYBOARD_y,      KEYBOARD_NONE },
  { KEY(u)          , KEYBOARD_u,      KEYBOARD_NONE },
  { KEY(i)          , KEYBOARD_i,      KEYBOARD_NONE },
  { KEY(o)          , KEYBOARD_o,      KEYBOARD_NONE },
  { KEY(p)          , KEYBOARD_p,      KEYBOARD_NONE },

  { KEY(Caps_Lock)  , KEYBOARD_Caps,   KEYBOARD_2    },
  { KEY(a)          , KEYBOARD_a,      KEYBOARD_NONE },
  { KEY(s)          , KEYBOARD_s,      KEYBOARD_NONE },
  { KEY(d)          , KEYBOARD_d,      KEYBOARD_NONE },
  { KEY(f)          , KEYBOARD_f,      KEYBOARD_NONE },
  { KEY(g)          , KEYBOARD_g,      KEYBOARD_NONE },
  { KEY(h)          , KEYBOARD_h,      KEYBOARD_NONE },
  { KEY(j)          , KEYBOARD_j,      KEYBOARD_NONE },
  { KEY(k)          , KEYBOARD_k,      KEYBOARD_NONE },
  { KEY(l)          , KEYBOARD_l,      KEYBOARD_NONE },
  { KEY(semicolon)  , KEYBOARD_Symbol, KEYBOARD_o    },
  { KEY(apostrophe) , KEYBOARD_Symbol, KEYBOARD_7    },
  { KEY(numbersign) , KEYBOARD_Symbol, KEYBOARD_3    },
  { KEY(Return)     , KEYBOARD_Enter,  KEYBOARD_NONE },

  { KEY(Shift_L)    , KEYBOARD_Caps,   KEYBOARD_NONE },
  { KEY(z)          , KEYBOARD_z,      KEYBOARD_NONE },
  { KEY(x)          , KEYBOARD_x,      KEYBOARD_NONE },
  { KEY(c)          , KEYBOARD_c,      KEYBOARD_NONE },
  { KEY(v)          , KEYBOARD_v,      KEYBOARD_NONE },
  { KEY(b)          , KEYBOARD_b,      KEYBOARD_NONE },
  { KEY(n)          , KEYBOARD_n,      KEYBOARD_NONE },
  { KEY(m)          , KEYBOARD_m,      KEYBOARD_NONE },
  { KEY(comma)      , KEYBOARD_Symbol, KEYBOARD_n    },
  { KEY(period)     , KEYBOARD_Symbol, KEYBOARD_m    },
  { KEY(slash)      , KEYBOARD_Symbol, KEYBOARD_v    },
  { KEY(Shift_R)    , KEYBOARD_Caps,   KEYBOARD_NONE },

  { KEY(Control_L)  , KEYBOARD_Symbol, KEYBOARD_NONE },
  { KEY(Alt_L)      , KEYBOARD_Symbol, KEYBOARD_NONE },
  { KEY(Meta_L)     , KEYBOARD_Symbol, KEYBOARD_NONE },
  { KEY(Super_L)    , KEYBOARD_Symbol, KEYBOARD_NONE },
  { KEY(Hyper_L)    , KEYBOARD_Symbol, KEYBOARD_NONE },
  { KEY(space)      , KEYBOARD_space,  KEYBOARD_NONE },
  { KEY(Hyper_R)    , KEYBOARD_Symbol, KEYBOARD_NONE },
  { KEY(Super_R)    , KEYBOARD_Symbol, KEYBOARD_NONE },
  { KEY(Meta_R)     , KEYBOARD_Symbol, KEYBOARD_NONE },
  { KEY(Alt_R)      , KEYBOARD_Symbol, KEYBOARD_NONE },
  { KEY(Control_R)  , KEYBOARD_Symbol, KEYBOARD_NONE },
  { KEY(Mode_switch), KEYBOARD_Symbol, KEYBOARD_NONE },

  { 0, 0, 0 }			/* End marker: DO NOT MOVE! */

};

#elif defined( UI_SVGA )		/* #if defined( UI_GTK ) || defined( UI_X ) */

static keysyms_key_info keysyms_data[] = {

  { SCANCODE_1            , KEYBOARD_1,      KEYBOARD_NONE },
  { SCANCODE_2            , KEYBOARD_2,      KEYBOARD_NONE },
  { SCANCODE_3            , KEYBOARD_3,      KEYBOARD_NONE },
  { SCANCODE_4            , KEYBOARD_4,      KEYBOARD_NONE },
  { SCANCODE_5            , KEYBOARD_5,      KEYBOARD_NONE },
  { SCANCODE_6            , KEYBOARD_6,      KEYBOARD_NONE },
  { SCANCODE_7            , KEYBOARD_7,      KEYBOARD_NONE },
  { SCANCODE_8            , KEYBOARD_8,      KEYBOARD_NONE },
  { SCANCODE_9            , KEYBOARD_9,      KEYBOARD_NONE },
  { SCANCODE_0            , KEYBOARD_0,      KEYBOARD_NONE },
  { SCANCODE_MINUS        , KEYBOARD_Symbol, KEYBOARD_j    },
  { SCANCODE_EQUAL        , KEYBOARD_Symbol, KEYBOARD_l    },
  { SCANCODE_BACKSPACE    , KEYBOARD_Caps,   KEYBOARD_0    },

  { SCANCODE_Q            , KEYBOARD_q,      KEYBOARD_NONE },
  { SCANCODE_W            , KEYBOARD_w,      KEYBOARD_NONE },
  { SCANCODE_E            , KEYBOARD_e,      KEYBOARD_NONE },
  { SCANCODE_R            , KEYBOARD_r,      KEYBOARD_NONE },
  { SCANCODE_T            , KEYBOARD_t,      KEYBOARD_NONE },
  { SCANCODE_Y            , KEYBOARD_y,      KEYBOARD_NONE },
  { SCANCODE_U            , KEYBOARD_u,      KEYBOARD_NONE },
  { SCANCODE_I            , KEYBOARD_i,      KEYBOARD_NONE },
  { SCANCODE_O            , KEYBOARD_o,      KEYBOARD_NONE },
  { SCANCODE_P            , KEYBOARD_p,      KEYBOARD_NONE },

  { SCANCODE_CAPSLOCK     , KEYBOARD_Caps,   KEYBOARD_2    },
  { SCANCODE_A            , KEYBOARD_a,      KEYBOARD_NONE },
  { SCANCODE_S            , KEYBOARD_s,      KEYBOARD_NONE },
  { SCANCODE_D            , KEYBOARD_d,      KEYBOARD_NONE },
  { SCANCODE_F            , KEYBOARD_f,      KEYBOARD_NONE },
  { SCANCODE_G            , KEYBOARD_g,      KEYBOARD_NONE },
  { SCANCODE_H            , KEYBOARD_h,      KEYBOARD_NONE },
  { SCANCODE_J            , KEYBOARD_j,      KEYBOARD_NONE },
  { SCANCODE_K            , KEYBOARD_k,      KEYBOARD_NONE },
  { SCANCODE_L            , KEYBOARD_l,      KEYBOARD_NONE },
  { SCANCODE_SEMICOLON    , KEYBOARD_Symbol, KEYBOARD_o    },
  { SCANCODE_APOSTROPHE   , KEYBOARD_Symbol, KEYBOARD_7    },
  /* this is what `#' returns on a UK keyboard
   * (`\' returns SCANCODE_LESS).
   */
  { SCANCODE_BACKSLASH    , KEYBOARD_Symbol, KEYBOARD_3    },
  { SCANCODE_ENTER	  , KEYBOARD_Enter,  KEYBOARD_NONE },

  { SCANCODE_LEFTSHIFT    , KEYBOARD_Caps,   KEYBOARD_NONE },
  { SCANCODE_Z            , KEYBOARD_z,      KEYBOARD_NONE },
  { SCANCODE_X            , KEYBOARD_x,      KEYBOARD_NONE },
  { SCANCODE_C            , KEYBOARD_c,      KEYBOARD_NONE },
  { SCANCODE_V            , KEYBOARD_v,      KEYBOARD_NONE },
  { SCANCODE_B            , KEYBOARD_b,      KEYBOARD_NONE },
  { SCANCODE_N            , KEYBOARD_n,      KEYBOARD_NONE },
  { SCANCODE_M            , KEYBOARD_m,      KEYBOARD_NONE },
  { SCANCODE_COMMA        , KEYBOARD_Symbol, KEYBOARD_n    },
  { SCANCODE_PERIOD       , KEYBOARD_Symbol, KEYBOARD_m    },
  { SCANCODE_SLASH        , KEYBOARD_Symbol, KEYBOARD_v    },
  { SCANCODE_RIGHTSHIFT   , KEYBOARD_Caps,   KEYBOARD_NONE },

  { SCANCODE_LEFTCONTROL  , KEYBOARD_Symbol, KEYBOARD_NONE },
  { SCANCODE_LEFTALT      , KEYBOARD_Symbol, KEYBOARD_NONE },
#ifdef SCANCODE_LEFTWIN
  { SCANCODE_LEFTWIN      , KEYBOARD_Symbol, KEYBOARD_NONE },
#endif				/* #ifdef SCANCODE_LEFTWIN */
  { SCANCODE_SPACE        , KEYBOARD_space,  KEYBOARD_NONE },
#ifdef SCANCODE_RIGHTWIN
  { SCANCODE_RIGHTWIN     , KEYBOARD_Symbol, KEYBOARD_NONE },
#endif				/* #ifdef SCANCODE_RIGHTWIN */
  { 127                   , KEYBOARD_Symbol, KEYBOARD_NONE }, /* Menu */
  { SCANCODE_RIGHTALT     , KEYBOARD_Symbol, KEYBOARD_NONE },
  { SCANCODE_RIGHTCONTROL , KEYBOARD_Symbol, KEYBOARD_NONE },

  { 0, 0, 0 }			/* End marker: DO NOT MOVE! */

};
#elif defined( UI_FB )

static keysyms_key_info keysyms_data[] = {

  { 1<<9            , KEYBOARD_0,      KEYBOARD_NONE },
  { 1<<10            , KEYBOARD_Enter,      KEYBOARD_NONE },
  { 1<<11            , KEYBOARD_p,      KEYBOARD_NONE },
  { 1<<12            , KEYBOARD_q,      KEYBOARD_NONE },
  { 0, 0, 0 }			/* End marker: DO NOT MOVE! */
};
#endif				/* #if defined( UI_GTK ) || defined( UI_X ) */

keysyms_key_info* keysyms_get_data(unsigned keysym)
{
  keysyms_key_info *ptr;

  for( ptr=keysyms_data; ptr->keysym; ptr++ ) {
    if(keysym==ptr->keysym) {
      return ptr;
    }
  }

  return NULL;

}
