---
title: Didaktik 80 Emulation
description: This section describes the Didaktik 80 interface emulation in Fuse.
order: 90
group: Floppy Disk Interfaces
---

Fuse supports Didaktik 80 (and Didaktik 40) emulation. It emulates the original
version of the Didaktik 80, running MDOS 1 and with a WD2797 floppy controller.

The ROM file is not supplied with Fuse for macOS and so must be installed before
the interface can be used. The expected file is a 14KB dump of the
Didaktik 40/80 MDOS 1 ROM named didaktik80.rom and placed in the Fuse packages
Contents/Resources directory.

See the [Disk File Formats](formats.html) section for more details on supported
disk file formats.

The Didaktik 80 may only be used with 16K, 48K and TC2048 emulation.

To press the Didaktik 80's 'SNAP' button, choose the Machine > Didaktik SNAP
menu option.
