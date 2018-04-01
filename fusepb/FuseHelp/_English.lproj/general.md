---
title: General Preferences
description: This section describes the Fuse general preferences dialog.
order: 10
group: Fuse Preferences
---

The General pane of Fuse preferences lets you configure miscellaneous Fuse
options.

OPTION | DESCRIPTION
:--- | :---
*Emulation speed* | Set how fast Fuse will attempt to emulate the Spectrum, as a percentage of the speed at which the real machine runs. If your machine isn't fast enough to keep up with the requested speed, Fuse will just run as fast as it can. Note that if the emulation speed is 1%, no sound output will be produced.
*Screen refresh rate* | Specify the frame rate, the ratio of spectrum frame updates to real frame updates. This is useful if your machine is having trouble keeping up with the spectrum screen updates.
*Use tape traps* | Ordinarily, Fuse intercepts calls to the ROM tape-loading routine in order to load from tape files more quickly when possible. But this can (rarely) interfere with TZX loading; disabling this option avoids the problem at the cost of slower (i.e. always real-time) tape-loading. When tape-loading traps are disabled, you need to start tape playback manually, by choosing the *Tape > Play* menu item.
*Fast tape loading* | If this option is enabled, then Fuse will run at the fastest possible speed when the virtual tape is playing, thus dramatically reducing the time it takes to load programs. You may wish to disable this option if you wish to stop the tape at a specific point.
*Accelerate loaders* | If this option is enabled, then Fuse will attempt to accelerate tape loaders by "short circuiting" the loading loop. This will in general speed up loading, but may cause some loaders to fail.
*Detect tape loaders* | If this option is enabled, Fuse will attempt to detect when a loading routine is in progress, and then automatically start the virtual tape to load the program in. This is done by using a heuristic to identify a loading routine, so is by no means infalliable, but works in most cases.
*Auto-load media* | On many occasions when you open a tape or disk file, it's because it's got a program in you want to load and run. If this option is enabled, this will automatically happen for you when you open one of these files using the *File > Open* menu option - you must then use the *Media* menu to use tapes or disks for saving data to, or for loading data into an already running program. This function starts loading media using a "phantom typist" that types the required commands. The available options are *Disabled*, *Auto*, *Keyword*, *Keystroke*, *Menu*, *Plus 2A* and *Plus 3*. The first five of these correspond to disable auto-load, automatic detection based on machine model, keyword based entry, keystroke based  entry, and selection from a 128K style menu. Plus 2A and Plus 3 also correspond to selection from a 128K style menu, but have special handling for games which need to be loaded with `LOAD "" CODE`. The most likely need to change this option will be to use *Keystroke* if you have changed the default 48K ROM for one with keystroke entry.
*Show tape/disk status* | Enables the status icons showing whether the disk and tape are being accessed.
*Confirm actions* | Specify whether "dangerous" actions (those which could cause data loss, for example resetting the Spectrum) require confirmation before occuring.
*Issue 2 keyboard* | Early versions of the Spectrum used a different value for unused bits on the keyboard input ports, and a few games depended on the old value of these bits. Enabling this option switches to the old value, to let you run those games.
*Late CPU timings* | If selected, Fuse will cause all screen-related timings (for example, when the screen is rendered and when memory contention occurs) to be one tstate later than "normal", an effect which is present on some real hardware.
*Z80 is CMOS* | If selected, Fuse will emulate a CMOS Z80, as opposed to an NMOS Z80. The undocumented `OUT (C),0` instruction will be replaced with `OUT (C),255` and emulation of a minor timing bug in the NMOS Z80's `LD A,I` and `LD A,R` instructions will be disabled.
*Allow writes to ROM* | If this option is selected, Fuse will happily allow programs to overwrite what would normally be ROM. This probably isn't very useful in most circumstances, especially as the 48K ROM overwrites parts of itself.
*Use .slt traps* | The multi-load aspect of SLT files requires a trap instruction to be supported. This instruction is not generally used except for this trap, but since it's not inconceivable that a program could be wanting to use the real instruction instead, you can choose whether to support the trap or not.
*Set joysticks on snapshot load* | Controls whether Fuse should allow snapshot files to override your current joystick configuration.
*Reset Preferences* | This causes all of Fuse's current preferences to be discarded and replaced with the default values as shipped.
