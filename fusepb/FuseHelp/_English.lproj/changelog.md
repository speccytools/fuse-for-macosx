---
title: What's New In Fuse?
description: This section describes the changes for version 1.3.5 of the Fuse emulator.
---

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
