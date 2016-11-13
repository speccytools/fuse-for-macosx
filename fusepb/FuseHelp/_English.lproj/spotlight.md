---
title: Spotlight Importer
description: This section describes Fuses support of the Spotlight feature of Mac OS X 10.4 and newer.
order: 240
---

On Mac OS X 10.4 and newer, the Fuse Spotlight importer allows Spotlight to find
ZX Spectrum emulation related files based on metadata in the files. The metadata
supported is:

## TZX Tape images

Title, publishers, authors, year of release, languages, category,
original price, loader, origin, comment, audio channel count, supported machines
and peripherals.

## Snapshots (SZX, Z80, SNA, etc.)

Machine type, and status of the enabled joysticks (Kempston, Cursor, Sinclair,
Timex and Fuller), ZXATASP and ZXCF IDE peripherals, and the Interface II and
Timex dock cartridges.

## RZX Spectrum recordings

As snapshots above.

## SCR Screen dumps

Graphics mode, width, height, orientation, colour space

## Custom Metadata Attribute Details

There are several Fuse-specific metadata attributes added by this importer, detailed below:

ATTRIBUTE | DESCRIPTION
:--- | :---
*Category* | type of software (arcade adventure, puzzle, word processor, ...)
*Price* | the original price the software sold for (includes currency)
*Loader* | protection scheme used in the title (Speedlock 1, Alkatraz, ...)
*Origin* | (Original, Budget re-release, ...)
*Machine* | Spectrum 16K, 48K (Issue 1), 48K, 128K, +2, +2A, +3, SE, Timex TC2048, Timex TC2068, Timex TS2068, Scorpion ZS 256, Pentagon 128K
*Joystick* | Kempston, Cursor, Sinclair 1, Sinclair 2, Timex 1, Timex 2, Fuller
*GraphicsMode* | Standard, HiColour, HiRes
