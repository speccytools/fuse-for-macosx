---
title: What's New In Fuse?
description: This section describes the changes for version 1.3.2 of the Fuse emulator.
---

## What's new in Fuse for macOS 1.3.2

### Emulation core improvements:
* Allow keyboard arrow keys to be used as a cursor joystick (thanks,
  solaris104) (Fredrick Meunier).
* Limit sound generation to less than 500% speed (thanks, windale and
  Sergio Baldoví) (Fredrick Meunier).

### Miscellaneous improvements:
* QuickLook Generator: speed improvements by enabling concurrent requests
  (Fredrick Meunier).
* New Fuse Help with improved organisation and cross-references (Fredrick Meunier).

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

## What's new in Fuse For Mac OS X 1.2.2

### New features:
* Support loading first tape, snapshot, dock cartridge or input recording (RZX)
  file found inside .zip files (Patrik Rak and Sergio Baldoví).
* Support auto-booting TR-DOS disk images without a boot file (thanks, windale,
  Vatra and Fredrick Meunier) (Sergio Baldoví).

### Emulation core improvements:
* Change microphone state when 0 tstate pulses do not have the no edge flag set
  (Fredrick Meunier).

### Machine specific improvements:
* Fix +3 disk autoload (thanks, windale and BogDan Vatra) (Sergio Baldoví and
  Fredrick Meunier).
* Fix floppy drive selection when resetting a +3 (thanks, windale and BogDan
  Vatra) (Sergio Baldoví).

### Various other minor bugfixes.

## What's new in Fuse For Mac OS X 1.2.1

### Emulation core improvements:
* Fix bugs when the detect loaders feature is being used (Fredrick Meunier).

### Debugger improvements:
* Remove the need for "%" when accessing system variables (Philip Kendall).
* Add Z80 registers as debugger variables (Philip Kendall).
* Expose last byte written to the ULA, tstates since interrupt, primary and
  secondary memory control ports as debugger system variables (Philip Kendall).
* Make breakpoints on events honour lifetime (Sergio Baldoví).
* Extend breakpoints on paging events to more peripherals: Beta 128, +D,
  Didaktik 80, DISCiPLE, Opus Discovery and SpeccyBoot (Sergio Baldoví).
* Split +D memory sources into RAM and ROM sections (Sergio Baldoví).
* Coalesce +D and DISCiPLE RAM pages so they show as 8K pages (Sergio Baldoví).

### Miscellaneous improvements:
* Add an emulator module startup manager to automatically handle dependency
  issues (Philip Kendall).

### Various other minor bugfixes.

## What's new in Fuse For Mac OS X 1.2.0

### New features:
* Add Currah µSource emulation (Stuart Brady).
* Add Didaktik 80/40 emulation (Gergely Szasz).

### Emulation core improvements:
* Allow continuing emulator recordings if there is a final snapshot in the RZX
  (Sergio Baldoví).
* Fix the prune function on emulator recording rollback (Sergio Baldoví).
* Use SZX format for the initial snapshot in emulator recording files (Sergio
  Baldoví).
* Fix loading of EDSK files with Sector Offset block (Sergio Baldoví).
* Migrate disk "index event" handling to the FDD layer and have the FDC layer
  use it for their STATUS registers (Gergely Szasz).
* Implement WD2797 emulation (Gergely Szasz).
* Centralise the "Disk icon" update code to the FDD layer (Gergely Szasz).
* Fix disk image corruption after saving UDI files (Sergio Baldoví).
* Check ready status after loading a disk into floppy disk drive (thanks, John
  Elliott) (Sergio Baldoví).
* Fix overlapped SEEK commands (Sergio Baldoví).
* Fix length of data returned by READ_DIAG (thanks, Fredrick Meunier) (Sergio
  Baldoví).
* Fix writing .td0 format disk files (Sergio Baldoví).
* Fix speech in Cobra's Arc - Medium Case.tzx when loaded with tape traps
  enabled (thanks, zx81 and Sergio Baldoví) (Fredrick Meunier).
* Skip tape traps if VERIFY is requested (UB880D).
* Fix loading sound with some custom loaders (Fredrick Meunier).
* Check if data blocks are headers for handling PZX files (Fredrick Meunier).
* Correct display of pulses in PZX pulse block for tape browser (Fredrick
  Meunier).
* Set AF, AF' and SP to 0xffff on reset (Stuart Brady).
* Leave most registers unchanged on a soft reset (Stuart Brady).
* Emulate interrupt and NMI timings more precisely (Stuart Brady).
* Emulate NMOS and CMOS Z80 variants (Stuart Brady).
* Switch to 2KB page size (Stuart Brady).
* Fix inaccurate output when AY envelopes are used (Matthew Westcott and
  Fredrick Meunier).
* Fix some peripherals activation when loading snapshots (Sergio Baldoví).

### Machine specific improvements:
* The address range 0x4000 - 0x7FFF is contended on the TS2068 in the home, Dock
  and Exrom banks (thanks, Richard Atkinson). It is assumed that this is the
  same for other Timex models (Fredrick Meunier).
* Clear all Opus RAM on hard reset (Stuart Brady).
* Ensure the ZX Printer does not require a hard reset to enable (thanks,
  RMartins) (Fredrick Meunier).
* Fix timing of events when emulating Scorpion (Stuart Brady).
* Ensure we have successfully selected a Pentagon or a Timex machine before
  inserting their media (Fredrick Meunier).
* Fix Beta 128 type II commands (thanks, windale and BogDan Vatra) (Sergio
  Baldoví).
* Don't enable the Beta 128 interface when loading a snapshot on a machine with
  Beta built-in (thanks, windale and BogDan Vatra) (Fredrick Meunier).
* Lock port +3 1FFDh if paging is disabled (Brian Ruthven and Fredrick Meunier).
* Spectranet: fix segfault in error handling when setting SO_REUSEADDR (Stuart
  Brady).
* Prefer Scorpion to Pentagon when loading SCL/TRD disks for better timing
  compatibility (thanks, windale) (Sergio Baldoví).
* Update SE ROMs to v4.07 (thanks, Andrew Owen) (Sergio Baldoví).

### Debugger improvements:
* Fix disassembly of LD (HL), LD (IX) and LD (IY) (BogDan Vatra).
* Add I and R register setting and getting to the debugger (Sergio Baldoví).
* Show the status of the halted flag in the debugger (Stuart Brady).
* Fix time breakpoints later than a frame in the future (Sergio Baldoví).
* Timex EXROM and Dock text was truncated in the debugger UI (thanks, Andrew
  Owen and Sergio Baldoví) (Fredrick Meunier).
* Fix memory issues when removing a matched breakpoint (Tom Seddon).
* Signal the UI when the breakpoints list is changed (BogDan Vatra).
* Document IF token for conditional expressions in debugger section (thanks,
  TK90XFan) (Sergio Baldoví).
* Fix documentation of address syntax used in debugger section (Sergio Baldoví).
* Allow debugger to dereference memory locations (Philip Kendall).
* Allow strings with escaped spaces in the debugger (Sergio Baldoví).
* Make wildcard event breakpoints work (thanks, Sergio Baldoví) (Philip
  Kendall).
* Protect divide expression from a divide by zero exception (Fredrick Meunier).

### Miscellaneous improvements:
* Fix save tape traps with SE ROM (Andrew Owen and Fredrick Meunier).
* Don't ignore the return code from setuid() (Sergio Baldoví).
* Generic FDD UI handling cleanup (Alex Badea).
* Remove unused macros on disk peripherals (Sergio Baldoví).
* Link to autoload snapshot for NTSC Spectrum (Sergio Baldoví).
* Abort start if we can't drop root privileges (Fredrick Meunier).
* Don't show an error when rewinding an empty tape (Sergio Baldoví).
* Only try to load the fallback ROM if it is different to the standard one
  (Fredrick Meunier).
* Track port attachment for each data bus line (Stuart Brady).
* Fix segfault due to inconsistent SETUP_CHECK() and CHECK() ordering (UB880D;
  thanks, Guesser).
* Use tape traps if we are using a custom ROM if the instructions at the entry
  points have been preserved (thanks, Alberto Garcia) (Fredrick Meunier).

### Various other minor bugfixes.
