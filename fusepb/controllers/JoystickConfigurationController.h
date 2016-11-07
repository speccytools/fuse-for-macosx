/* JoystickConfigurationController.h: Routines for dealing with the joystick configuration panel
   Copyright (c) 2004 Fredrick Meunier

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

#import <AppKit/AppKit.h>


@interface JoystickConfigurationController : NSWindowController {
  IBOutlet NSPopUpButton *joyFire1;
  IBOutlet NSPopUpButton *joyFire2;
  IBOutlet NSPopUpButton *joyFire3;
  IBOutlet NSPopUpButton *joyFire4;
  IBOutlet NSPopUpButton *joyFire5;
  IBOutlet NSPopUpButton *joyFire6;
  IBOutlet NSPopUpButton *joyFire7;
  IBOutlet NSPopUpButton *joyFire8;
  IBOutlet NSPopUpButton *joyFire9;
  IBOutlet NSPopUpButton *joyFire10;
  IBOutlet NSPopUpButton *joyFire11;
  IBOutlet NSPopUpButton *joyFire12;
  IBOutlet NSPopUpButton *joyFire13;
  IBOutlet NSPopUpButton *joyFire14;
  IBOutlet NSPopUpButton *joyFire15;
  IBOutlet NSPopUpButton *joyXAxis;
  IBOutlet NSPopUpButton *joyYAxis;

  int joyNum;
}
- (IBAction)apply:(id)sender;
- (IBAction)cancel:(id)sender;
- (void)showWindow:(id)sender;
@end
