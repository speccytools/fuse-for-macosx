---
title: Weak Disk Data
description: This section describes the Fuse poke finder.
order: 190
group: Floppy Disk Interfaces
---

Some copy protections have what is described as 'weak/random’ data. Each time
the sector is read one or more bytes will change, the value may be random
between consecutive reads of the same sector. Two disk image formats (Extended
DSK and UDI) can store this type of data. Fuse can read and use weak sector data
from EDSK and UDI files when present, and can save back weak sector data to UDI
image format files.
