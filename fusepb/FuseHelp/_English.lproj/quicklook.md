---
title: Quick Look
description: This section describes FuseX's support for Quick Look thumbnails and previews on macOS 12 and newer.
order: 250
---

On macOS 12 and newer, FuseX's Quick Look extensions allow viewing
thumbnails and previews of ZX Spectrum emulation related files. The supported
types are:

## SCR and MLT Screen dumps

Preview and thumbnails for SCR and MLT screen dumps in all graphics modes.

## Snapshots (SZX, Z80, SNA, etc.)

The screen image embedded in the snapshot is extracted and used as the
thumbnail for the file. Advanced graphics effects produced in some demo programs
cannot be reproduced in this way in the same way as SCR files.

## RZX Spectrum recordings

The first embedded snapshot in the file is extracted and used as the thumbnail
for the file in the same way as snapshots above.

## Tape images (TAP and TZX)

The first Spectrum screen file (SCREEN$) stored with the standard ROM loader in
the tape is extracted and used as the thumbnail of the file in the same way as
snapshot files above.
