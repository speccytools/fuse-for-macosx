---
title: Printer Emulation
description: This section describes the printer emulation in Fuse.
order: 70
---

The various models of Spectrum supported a range of ways to connect printers,
three of which are supported by Fuse. Different printers are made available for
the different models:

MACHINE | PRINTER
:--- | :---
*16, 48, TC2048, TC2068* | ZX Printer
*128 , +2, Pentagon* | Serial printer (text-only)
*+2A, +3* | Parallel printer (text-only)

<br>
If Opus Discovery, +D or DISCiPLE emulation is in use and printer emulation is
enabled, text-only emulation of the disk interface's parallel printer interface
is provided.

Any printout is appended to one (or both) of two files, depending on the printer
\- these default to *printout.txt* for text output, and *printout.pbm* for
graphics (PBM images are supported by most image viewers and converters). These
names can be changed with the *Preferences > Peripherals* dialog. While the ZX
Printer can only output graphically, simulated text output is generated at the
same time using a crude sort of OCR based on the current character set (a bit
like using SCREEN$). There is currently no support for graphics when using the
serial/parallel output, though any escape codes used will be 'printed'
faithfully. (!)

By the way, it's not a good idea to modify the *printout.pbm* file outside of
Fuse if you want to continue appending to it. The header needs to have a certain
layout for Fuse to be able to continue appending to it correctly, and the file
will be overwritten if it can't be appended to.
