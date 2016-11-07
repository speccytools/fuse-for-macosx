/* PokeMemoryController.m: Routines for dealing with the POKE window
   Copyright (c) 2012 Fredrick Meunier

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

*/

#import "PokeMemoryController.h"

#import "DisplayOpenGLView.h"

#include "pokefinder/pokemem.h"

static void trainer_add( gpointer data, gpointer user_data );

@implementation PokeMemoryController

id notificationObserver;

- (id)init
{
  NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
  NSOperationQueue *mainQueue = [NSOperationQueue mainQueue];
  
  [super init];

  self = [super initWithWindowNibName:@"PokeMemory"];

  [self setWindowFrameAutosaveName:@"PokeMemoryWindow"];
  
  notificationObserver =
    [nc addObserverForName:@"NSWindowWillCloseNotification"
                    object:[self window]
                     queue:mainQueue
                usingBlock:^(NSNotification *note) {
                  [NSApp stopModal];
                
                  for( NSMutableDictionary* trainerModel in [trainersController content] ) {
                    trainer_t* trainer =
                      (trainer_t*)[[trainerModel valueForKey:@"trainerVal"] pointerValue];
                    if( [[trainerModel valueForKey:@"active"] longValue] == NSOnState ) {
                      pokemem_trainer_activate( trainer );
                    } else {
                      pokemem_trainer_deactivate( trainer );
                    }
                  }
                  
                  [trainersController setContent:nil];
                  
                  [trainers reloadData];
                
                  [[DisplayOpenGLView instance] unpause];
                }];
  
  [self setWindowFrameAutosaveName:@"PokeMemoryWindow"];
  
  [trainersController setAvoidsEmptySelection:NO];
  [trainersController setSelectsInsertedObjects:NO];
  
  return self;
}

- (void)dealloc
{
  NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
  [nc removeObserver:notificationObserver];

  [super dealloc];
}

- (void)showWindow:(id)sender
{
  [[DisplayOpenGLView instance] pause];

  if( trainer_list ) {
    g_slist_foreach( trainer_list, trainer_add, self );
  }

  [super showWindow:sender];

  [trainers reloadData];
  
  [NSApp runModalForWindow:[self window]];
}

- (void)addObjectToPokeList:(NSDictionary*)poke
{
  [trainersController addObject:poke];
}

@synthesize trainersController;

@end

static void
trainer_add( gpointer data, gpointer user_data )
{
  PokeMemoryController *controller = user_data;
  trainer_t *trainer = data;

  if( !trainer ) return;

  NSNumber *active = [NSNumber numberWithLong:(trainer->active ? NSOnState : NSOffState)];
  NSString *name = [NSString stringWithUTF8String:trainer->name];

  NSMutableDictionary *trainerModel =
    [NSMutableDictionary dictionaryWithDictionary:@{@"active" : active, @"trainer" : name,
                                                    @"trainerVal" : [NSValue valueWithPointer:trainer]}];
  [controller addObjectToPokeList:trainerModel];
}
