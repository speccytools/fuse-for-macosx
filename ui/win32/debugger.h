/* debugger.h: the Win32 debugger
   Copyright (c) 2004 Marek Januszewski

   $Id$

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

/* FIXME: http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vclib/html/_MFCNOTES_TN020.asp */

#define IDD_DBG				2000
#define IDC_DBG_LV_PC			( IDD_DBG + 1 )
#define IDC_DBG_SB_PC			( IDD_DBG + 2 )
#define IDC_DBG_LV_STACK		( IDD_DBG + 3 )
#define IDC_DBG_LV_EVENTS		( IDD_DBG + 4 )
#define IDC_DBG_LV_BPS			( IDD_DBG + 5 )
#define IDC_DBG_ED_EVAL			( IDD_DBG + 6 )
#define IDC_DBG_BTN_EVAL		( IDD_DBG + 7 )
#define IDC_DBG_GRP_MEMMAP		( IDD_DBG + 8 )
#define IDC_DBG_BTN_STEP		( IDD_DBG + 9 )
#define IDC_DBG_BTN_CONT		( IDD_DBG + 10 )
#define IDC_DBG_BTN_BREAK		( IDD_DBG + 11 )

#define IDC_DBG_MAP11			( IDD_DBG + 12 )
#define IDC_DBG_MAP12			( IDC_DBG_MAP11 + 1 )
#define IDC_DBG_MAP13			( IDC_DBG_MAP12 + 1 )
#define IDC_DBG_MAP14			( IDC_DBG_MAP13 + 1 )
#define IDC_DBG_MAP21			( IDC_DBG_MAP14 + 1 )
#define IDC_DBG_MAP22			( IDC_DBG_MAP21 + 1 )
#define IDC_DBG_MAP23			( IDC_DBG_MAP22 + 1 )
#define IDC_DBG_MAP24			( IDC_DBG_MAP23 + 1 )
#define IDC_DBG_MAP31			( IDC_DBG_MAP24 + 1 )
#define IDC_DBG_MAP32			( IDC_DBG_MAP31 + 1 )
#define IDC_DBG_MAP33			( IDC_DBG_MAP32 + 1 )
#define IDC_DBG_MAP34			( IDC_DBG_MAP33 + 1 )
#define IDC_DBG_MAP41			( IDC_DBG_MAP34 + 1 )
#define IDC_DBG_MAP42			( IDC_DBG_MAP41 + 1 )
#define IDC_DBG_MAP43			( IDC_DBG_MAP42 + 1 )
#define IDC_DBG_MAP44			( IDC_DBG_MAP43 + 1 )
#define IDC_DBG_MAP51			( IDC_DBG_MAP44 + 1 )
#define IDC_DBG_MAP52			( IDC_DBG_MAP51 + 1 )
#define IDC_DBG_MAP53			( IDC_DBG_MAP52 + 1 )
#define IDC_DBG_MAP54			( IDC_DBG_MAP53 + 1 )
#define IDC_DBG_MAP61			( IDC_DBG_MAP54 + 1 )
#define IDC_DBG_MAP62			( IDC_DBG_MAP61 + 1 )
#define IDC_DBG_MAP63			( IDC_DBG_MAP62 + 1 )
#define IDC_DBG_MAP64			( IDC_DBG_MAP63 + 1 )
#define IDC_DBG_MAP71			( IDC_DBG_MAP64 + 1 )
#define IDC_DBG_MAP72			( IDC_DBG_MAP71 + 1 )
#define IDC_DBG_MAP73			( IDC_DBG_MAP72 + 1 )
#define IDC_DBG_MAP74			( IDC_DBG_MAP73 + 1 )
#define IDC_DBG_MAP81			( IDC_DBG_MAP74 + 1 )
#define IDC_DBG_MAP82			( IDC_DBG_MAP81 + 1 )
#define IDC_DBG_MAP83			( IDC_DBG_MAP82 + 1 )
#define IDC_DBG_MAP84			( IDC_DBG_MAP83 + 1 )

#define IDC_DBG_REG_PC			( IDC_DBG_MAP84 + 1 )
#define IDC_DBG_REG_SP			( IDC_DBG_REG_PC + 1 )
#define IDC_DBG_REG_AF			( IDC_DBG_REG_SP + 1 )
#define IDC_DBG_REG_AF_			( IDC_DBG_REG_AF + 1 )
#define IDC_DBG_REG_BC			( IDC_DBG_REG_AF_ + 1 )
#define IDC_DBG_REG_BC_			( IDC_DBG_REG_BC + 1 )
#define IDC_DBG_REG_DE			( IDC_DBG_REG_BC_ + 1 )
#define IDC_DBG_REG_DE_			( IDC_DBG_REG_DE + 1 )
#define IDC_DBG_REG_HL			( IDC_DBG_REG_DE_ + 1 )
#define IDC_DBG_REG_HL_			( IDC_DBG_REG_HL + 1 )
#define IDC_DBG_REG_IX			( IDC_DBG_REG_HL_ + 1 )
#define IDC_DBG_REG_IY			( IDC_DBG_REG_IX + 1 )
#define IDC_DBG_REG_I			( IDC_DBG_REG_IY + 1 )
#define IDC_DBG_REG_R			( IDC_DBG_REG_I + 1 )
#define IDC_DBG_REG_T_STATES		( IDC_DBG_REG_R + 1 )
#define IDC_DBG_REG_IFF1		( IDC_DBG_REG_T_STATES + 1 )
#define IDC_DBG_REG_IFF2		( IDC_DBG_REG_IFF1 + 1 )
#define IDC_DBG_REG_SZ5H3PNC		( IDC_DBG_REG_IFF2 + 1 )
#define IDC_DBG_REG_ULA			( IDC_DBG_REG_SZ5H3PNC + 1 )
#define IDC_DBG_REG_IM			( IDC_DBG_REG_ULA + 1 )
#define IDC_DBG_OTHER1			( IDC_DBG_REG_IM + 1 )
#define IDC_DBG_OTHER2			( IDC_DBG_OTHER1 + 1 )

#define IDC_DBG_TEXT_SP			( IDC_DBG_OTHER2 + 1 )
#define IDC_DBG_TEXT_PC			( IDC_DBG_OTHER2 + 2 )
#define IDC_DBG_TEXT_AF1		( IDC_DBG_OTHER2 + 3 )
#define IDC_DBG_TEXT_AF			( IDC_DBG_OTHER2 + 4 )
#define IDC_DBG_TEXT_BC			( IDC_DBG_OTHER2 + 5 )
#define IDC_DBG_TEXT_DE			( IDC_DBG_OTHER2 + 6 )
#define IDC_DBG_TEXT_HL			( IDC_DBG_OTHER2 + 7 )
#define IDC_DBG_TEXT_IX			( IDC_DBG_OTHER2 + 8 )
#define IDC_DBG_TEXT_I			( IDC_DBG_OTHER2 + 9 )
#define IDC_DBG_TEXT_R			( IDC_DBG_OTHER2 + 10 )
#define IDC_DBG_TEXT_IY			( IDC_DBG_OTHER2 + 11 )
#define IDC_DBG_TEXT_HL1		( IDC_DBG_OTHER2 + 12 )
#define IDC_DBG_TEXT_DE1		( IDC_DBG_OTHER2 + 13 )
#define IDC_DBG_TEXT_BC1		( IDC_DBG_OTHER2 + 14 )
#define IDC_DBG_TEXT_IM			( IDC_DBG_OTHER2 + 15 )
#define IDC_DBG_TEXT_IFF1		( IDC_DBG_OTHER2 + 16 )
#define IDC_DBG_TEXT_IFF2		( IDC_DBG_OTHER2 + 17 )
#define IDC_DBG_TEXT_ULA		( IDC_DBG_OTHER2 + 18 )
#define IDC_DBG_TEXT_T_STATES		( IDC_DBG_OTHER2 + 19 )

#define IDC_DBG_TEXT_ADDRESS		( IDC_DBG_OTHER2 + 20 )
#define IDC_DBG_TEXT_WRITABLE		( IDC_DBG_OTHER2 + 21 )
#define IDC_DBG_TEXT_CONTENDED		( IDC_DBG_OTHER2 + 22 )
#define IDC_DBG_TEXT_SOURCE		( IDC_DBG_OTHER2 + 23 )

#define IDM_DBG_MENU			( IDC_DBG_TEXT_T_STATES + 1 )
#define IDM_DBG_VIEW			( IDC_DBG_TEXT_T_STATES + 2 )
#define IDM_DBG_REG			( IDC_DBG_TEXT_T_STATES + 3 )
#define IDM_DBG_MEMMAP			( IDC_DBG_TEXT_T_STATES + 4 )
#define IDM_DBG_BPS			( IDC_DBG_TEXT_T_STATES + 5 )
#define IDM_DBG_DIS			( IDC_DBG_TEXT_T_STATES + 6 )
#define IDM_DBG_STACK			( IDC_DBG_TEXT_T_STATES + 7 )
#define IDM_DBG_EVENTS			( IDC_DBG_TEXT_T_STATES + 8 )
