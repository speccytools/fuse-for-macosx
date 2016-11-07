/* keysyms.m: UI keysym to Fuse input layer keysym mappings
   Copyright (c) 2000-2005 Philip Kendall, Matan Ziv-Av, Russell Marks,
                           Fredrick Meunier, Catalin Mihaila

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

#import <AppKit/NSEvent.h>

#include <config.h>

#include "input.h"
#include "keyboard.h"

/* Map ADC keyboard scancode to Fuse input layer keysym for Spectrum
   virtual keyboard */
keysyms_map_t keysyms_map[] = {

  { 48,         INPUT_KEY_Tab        },
  { 36,         INPUT_KEY_Return     },
  { 51,         INPUT_KEY_BackSpace  },
  /* Start keypad */
  { 83,         INPUT_KEY_1          },
  { 84,         INPUT_KEY_2          },
  { 85,         INPUT_KEY_3          },
  { 86,         INPUT_KEY_4          },
  { 87,         INPUT_KEY_5          },
  { 88,         INPUT_KEY_6          },
  { 89,         INPUT_KEY_7          },
  { 91,         INPUT_KEY_8          },
  { 92,         INPUT_KEY_9          },
  { 82,         INPUT_KEY_0          },
  { 65,         INPUT_KEY_period     },
  { 75,         INPUT_KEY_slash      },
  { 67,         INPUT_KEY_asterisk   },
  { 78,         INPUT_KEY_minus      },
  { 69,         INPUT_KEY_plus       },
  { 76,         INPUT_KEY_Return     },
  { 81,         INPUT_KEY_equal      },
  /* End keypad */
  { 18,         INPUT_KEY_1          },
  { 19,         INPUT_KEY_2          },
  { 20,         INPUT_KEY_3          },
  { 21,         INPUT_KEY_4          },
  { 23,         INPUT_KEY_5          },
  { 22,         INPUT_KEY_6          },
  { 26,         INPUT_KEY_7          },
  { 28,         INPUT_KEY_8          },
  { 25,         INPUT_KEY_9          },
  { 29,         INPUT_KEY_0          },
  { 12,         INPUT_KEY_q          },
  { 13,         INPUT_KEY_w          },
  { 14,         INPUT_KEY_e          },
  { 15,         INPUT_KEY_r          },
  { 17,         INPUT_KEY_t          },
  { 16,         INPUT_KEY_y          },
  { 32,         INPUT_KEY_u          },
  { 34,         INPUT_KEY_i          },
  { 31,         INPUT_KEY_o          },
  { 35,         INPUT_KEY_p          },
  { 0,          INPUT_KEY_a          },
  { 1,          INPUT_KEY_s          },
  { 2,          INPUT_KEY_d          },
  { 3,          INPUT_KEY_f          },
  { 5,          INPUT_KEY_g          },
  { 4,          INPUT_KEY_h          },
  { 38,         INPUT_KEY_j          },
  { 40,         INPUT_KEY_k          },
  { 37,         INPUT_KEY_l          },
  { 6,          INPUT_KEY_z          },
  { 7,          INPUT_KEY_x          },
  { 8,          INPUT_KEY_c          },
  { 9,          INPUT_KEY_v          },
  { 11,         INPUT_KEY_b          },
  { 45,         INPUT_KEY_n          },
  { 46,         INPUT_KEY_m          },
  { 49,         INPUT_KEY_space      },

  { 0xff, 0 }			/* End marker: DO NOT MOVE! */

};

/* Map ADC keyboard scancode to Fuse input layer keysym for Recreated
   ZX Spectrum keyboard */
keysyms_map_t recreated_keysyms_map[] = {
  
  { 44,         INPUT_KEY_slash        },
  { 41,         INPUT_KEY_semicolon    },
  { 43,         INPUT_KEY_comma        },
  { 47,         INPUT_KEY_period       },
  { 33,         INPUT_KEY_bracketleft  },
  { 30,         INPUT_KEY_bracketright },
  
  { 0, 0 }			/* End marker: DO NOT MOVE! */
  
};

/* Map UCS-2(?) Unicode to Fuse input layer keysym for
   non-extended mode Spectrum symbols present on keyboards */
keysyms_map_t unicode_keysyms_map[] = {

  { 27,                      INPUT_KEY_Escape      },
  { NSUpArrowFunctionKey,    INPUT_KEY_Up          },
  { NSDownArrowFunctionKey,  INPUT_KEY_Down        },
  { NSLeftArrowFunctionKey,  INPUT_KEY_Left        },
  { NSRightArrowFunctionKey, INPUT_KEY_Right       },
  { NSF1FunctionKey,         INPUT_KEY_F1          },
  { NSF2FunctionKey,         INPUT_KEY_F2          },
  { NSF3FunctionKey,         INPUT_KEY_F3          },
  { NSF4FunctionKey,         INPUT_KEY_F4          },
  { NSF5FunctionKey,         INPUT_KEY_F5          },
  { NSF6FunctionKey,         INPUT_KEY_F6          },
  { NSF7FunctionKey,         INPUT_KEY_F7          },
  { NSF8FunctionKey,         INPUT_KEY_F8          },
  { NSF9FunctionKey,         INPUT_KEY_F9          },
  { NSF10FunctionKey,        INPUT_KEY_F10         },
  { NSF11FunctionKey,        INPUT_KEY_F11         },
  { NSF12FunctionKey,        INPUT_KEY_F12         },
  { NSDeleteFunctionKey,     INPUT_KEY_BackSpace   },
  { NSHomeFunctionKey,       INPUT_KEY_Home        },
  { NSEndFunctionKey,        INPUT_KEY_End         },
  { NSPageUpFunctionKey,     INPUT_KEY_Page_Up     },
  { NSPageDownFunctionKey,   INPUT_KEY_Page_Down   },
  { NSMenuFunctionKey,       INPUT_KEY_Mode_switch },
  { '-',                     INPUT_KEY_minus       },
  { '_',                     INPUT_KEY_underscore  },
  { '=',                     INPUT_KEY_equal       },
  { '+',                     INPUT_KEY_plus        },
  { ';',                     INPUT_KEY_semicolon   },
  { ':',                     INPUT_KEY_colon       },
  { '\'',                    INPUT_KEY_apostrophe  },
  { '"',                     INPUT_KEY_quotedbl    },
  { '#',                     INPUT_KEY_numbersign  },
  { ',',                     INPUT_KEY_comma       },
  { '<',                     INPUT_KEY_less        },
  { '.',                     INPUT_KEY_period      },
  { '>',                     INPUT_KEY_greater     },
  { '/',                     INPUT_KEY_slash       },
  { '?',                     INPUT_KEY_question    },
  { '!',                     INPUT_KEY_exclam      },
  { '@',                     INPUT_KEY_at          },
  { '$',                     INPUT_KEY_dollar      },
  { '%',                     INPUT_KEY_percent     },
  { '&',                     INPUT_KEY_ampersand   },
  { '(',                     INPUT_KEY_parenleft   },
  { ')',                     INPUT_KEY_parenright  },
  { '^',                     INPUT_KEY_asciicircum },
  { '*',                     INPUT_KEY_asterisk    },

  { 0, 0 }			/* End marker: DO NOT MOVE! */

};
