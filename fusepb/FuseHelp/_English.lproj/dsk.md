---
title: The .DSK Format
description: This section describes the .DSK format for Spectrum +3 disks.
order: 185
group: Floppy Disk Interfaces
---

In general, disk images for the +3 Spectrum are thought of as being in DSK
format. However, this is actually an slight oversimplification; there in in fact
two similar, but not identical, DSK formats. (The difference can be seen by
doing `head -1 dskfile`: one format will start 'MV - CPCEMU' and the other will
start 'EXTENDED').

The Fuse +3 FDC emulation supports both the 'CPCEMU' format and the extended
format, as well as other goodies such as compressed disk images.
