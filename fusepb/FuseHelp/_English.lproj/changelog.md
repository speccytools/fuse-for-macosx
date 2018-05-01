---
title: What's New In Fuse?
description: This section describes the changes for version 1.5.3 of the Fuse emulator.
---

## What's new in Fuse for macOS 1.5.3

### Emulation core improvements:
* Disable inactive peripherals after loading a snapshot (Sergio Baldoví).

### Miscellaneous improvements:
* Re-enable sound after phantom typist finishes loading TAP, standard ROM TZX
  or +3 DSK images (thanks, Alberto Garcia) (Fredrick Meunier).
* RZX files containing a snapshot which cannot be compressed are now written
  correctly (Philip Kendall, thanks Chris Flynn).

## What's new in Fuse for macOS 1.5.2

### Emulation core improvements:
* Emulate ROM bug loading zero length blocks when using tape traps
  (ub880d).

### Machine specific improvements:
* Fix the format of double-sided +3 disks (Sergio Baldoví).

### Miscellaneous improvements:
* Spectrum reset is accelerated when auto-load is enabled and a
  file is loaded from the menu (Fredrick Meunier).
* Add preferences for changing the auto-load mode (Fredrick Meunier).

## What's new in Fuse for macOS 1.5.1

### Debugger improvements:
* Prevent crash when we try to disassemble an instruction with many DD or FD
  prefixes (Philip Kendall; thanks, Miguel Angel Rodríguez Jódar).
* Fix crash when setting debugger variables (Gergely Szasz).

### Profiler improvements:
* Prevent crash when we try to profile an instruction with many DD or FD
  prefixes (Philip Kendall; thanks, Sergio Baldoví).

## What's new in Fuse for macOS 1.5.0

### Debugger improvements:
* Ensure conditional timed breakpoints work correctly (Philip Kendall).

### Miscellaneous improvements:
* Autoload snapshots replaced by a "phantom typist" which types LOAD "" or
  similar.
* Alkatraz loaders (e.g. Cobra and Fairlight) are now accelerated (Philip
  Kendall).

## What's new in Fuse for macOS 1.4.1

### Emulation core improvements:
* Improvements to the loader acceleration code to reduce errors - Blood
  Brothers, City Slicker, Driller, Dynamite Dan, Games Compendium (by Gremlin),
  Joe Blade II, Kokotoni Wilf, Powerplay, Saboteur, Trapdoor and Zanthrax now
  all load successfully (thanks, windale and ub880d) (Philip Kendall).
* Multiface 3 returns values stored from ports 0x1ffd and 0x7ffd (thanks,
  Fredrick Meunier) (Sergio Baldoví).
* Set contention for DivIDE/DivMMC EPROM memory and clear data to 1's (Sergio
  Baldoví).
* Fix generation of malformed RZXs (thanks, Nicholas Naime) (ub880d).

### Debugger improvements:
* Add new "tape:microphone" and "spectrum:frames" system variables to allow
  access to the current tape level and frame count since reset (Philip Kendall).

### Deprecated features removed:
* All Z80 variables in the debugger must now be referenced as "z80:NAME" rather
  than just "NAME" e.g. "set z80:af 0x1234" rather than just "set af 0x1234"
  (Philip Kendall).

### Various other minor bugfixes.

## What's new in Fuse for macOS 1.4.0

### New features:
* Add DivMMC emulation (Philip Kendall and Sergio Baldoví).
* Add ZXMMC emulation (Philip Kendall and Sergio Baldoví).

### Miscellaneous improvements:
* Add support for MLT format screenshots (Fredrick Meunier).

### Various other minor bugfixes.

## What's new in Fuse for macOS 1.3.8

### Emulation core improvements:
* Add workaround for Multiface One and 128 clash (thanks, Fredrick Meunier)
  (Sergio Baldoví).
* Limit RZX sentinel warning to once per playback (Sergio Baldoví).
* Disable Melodik interface on 128K machines (Sergio Baldoví).
* Correct the list of machines for Multiface One (Fredrick Meunier).
* Fix Z80 snapshot writing when +D is enabled (thanks, Fredrick Meunier) (Sergio
  Baldoví).
* Fix offset of keyboard mappings in Z80 v3 snaphots (Sergio Baldoví).
* Don't use MDR random length by default (Fredrick Meunier).
* Also include Eject options for Opus and Didaktik disk interfaces (thanks,
  Alain Vezes) (Fredrick Meunier).

### Various other minor bugfixes.

## What's new in Fuse for macOS 1.3.7

### New features:
* Add Multiface One/128/3 interface emulation (Gergely Szasz and Sergio
  Baldoví).

### Machine specific improvements:
* Restore +2A/+3 ALL_RAM mode from snapshots (Sergio Baldoví).

### Miscellaneous improvements:
* Fix crash when saving CSW tapes (thanks, Nicholas Naime and Fredrick Meunier)
  (Sergio Baldoví).

### Various other minor bugfixes.

## What's new in Fuse for macOS 1.3.6

### New features:
* Add Covox interface emulation (Fredrick Meunier).

### Emulation core improvements:
* Disable accelerate loader while recording RZX files (thanks, windale) (Sergio
  Baldoví).
* Fix releasing captured Kempston mouse (thanks, Robert Uttley) (Fredrick
  Meunier).

### Miscellaneous improvements:
* Restored ability to select save formats (thanks, TomD) (Fredrick Meunier).
* Work around invalid "used bits in last byte" field in TZX tapes (thanks,
  Philip Kendall and Sergio Baldoví) (Fredrick Meunier).
* Save SpecDrum level as unsigned in SZX snapshots (Sergio Baldoví).

### Various other minor bugfixes.

## What's new in Fuse for macOS 1.3.5

### Emulation core improvements:
* Disable tape traps when playing/recording RZX files (thanks, windale) (Sergio
  Baldoví).
* Silently skip PLTT blocks in SZX snapshots (thanks, windale) (Fredrick Meunier
  and Sergio Baldoví).
* Validate "used bits in last byte" field in TZX tapes (thanks, Nicholas Naime
  and Fredrick Meunier) (Sergio Baldoví).
* Fix the load of PZX tapes with malformed strings (thanks, Nicholas Naime)
  (Sergio Baldoví).

### Various other minor bugfixes.

## What's new in Fuse for macOS 1.3.4

### Debugger improvements:
* Fix syntax for "breakpoint read" debugger command in the manual (thanks, Andy
  Chadbourne) (Sergio Baldoví).

## What's new in Fuse for macOS 1.3.3

### Emulation core improvements:
* Add support for the hidden MEMPTR register (thanks, Boo-boo, Vladimir Kladov
  and the members of the "Z80 Assembly Programming On The ZX Spectrum"
  Facebook group) (Philip Kendall).
* Mark new disks as needing to be saved (Gergely Szasz).

### Miscellaneous improvements:
* Remove duplicated insert/eject code from DivIDE, Simple IDE and ZXATASP
  interfaces (Philip Kendall).
* Improve IDE interface master/slave initialisation code (Philip Kendall).
* Fix multiple save of disks (Gergely Szasz).
* Allow overwriting disk images (Gergely Szasz).

### Various other minor bugfixes.

## What's new in Fuse for macOS 1.3.2

### Emulation core improvements:
* Allow keyboard arrow keys to be used as a cursor joystick (thanks,
  solaris104) (Fredrick Meunier).
* Limit sound generation to less than 500% speed (thanks, windale and
  Sergio Baldoví) (Fredrick Meunier).

### Miscellaneous improvements:
* QuickLook Generator: speed improvements by enabling concurrent requests
  (Fredrick Meunier).
* New Fuse Help with improved organisation and cross-references (Fredrick
  Meunier).

### Various other minor bugfixes.

## What's new in Fuse for macOS 1.3.1

### Emulation core improvements:
* Warn on inserting a disk image larger than the emulated drive (thanks,
  Stefano Bodrato) (Fredrick Meunier).

### Miscellaneous improvements:
* Fix Recreated ZX Spectrum Bluetooth keyboard support on the Mac (thanks, Allan
  Høiberg and Jason Hudson) (Fredrick Meunier).

### Various other minor bugfixes.

## What's new in Fuse For Mac OS X 1.3.0

### New features:
* Recreated ZX Spectrum Bluetooth keyboard support (thanks, thrice, Philip
  Kendall and Sergio Baldoví) (Ekkehard Morgenstern).

### Emulation core improvements:
* Reset machine when auto-loading TR-DOS disks (thanks, BogDan Vatra and
  Fredrick Meunier) (Sergio Baldoví).

### Machine specific improvements:
* Update +3e ROMs to v1.43 (Sergio Baldoví; thanks, Garry Lancaster).

### Various other minor bugfixes.
