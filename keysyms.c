/* keysyms.c: keysym to Spectrum key mappings for both Xlib and GDK
   Copyright (c) 2000 Philip Kendall

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

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <stdlib.h>

#include "keysyms.h"

/* The keysyms are specified either as GDK_<xx> (if we're using GTK+) or
   XK_<xx> if we're using Xlib. One quick macro so I don't have to repeat
   the entire table... */
#ifdef HAVE_LIBGTK
#include <gdk/gdkkeysyms.h>
#define KEY(keysym) GDK_ ## keysym
#else                   /* #ifdef HAVE_LIBGTK */
#include <X11/keysym.h>
#define KEY(keysym) XK_ ## keysym
#endif

/* The mappings from keysyms to Spectrum keys. The keysym `keysym'
   maps to one or two Spectrum keys, specified by {port,bit}{1,2}. 
   Unused second keys are specified by having `bit2' being zero

   These mappings are ordered basically like a standard (English) PC keyboard,
   top to bottom, left to right, but with a few additions for other keys
*/

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

  { 0, 0, 0 }			 /* End marker: DO NOT MOVE! */

};

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
