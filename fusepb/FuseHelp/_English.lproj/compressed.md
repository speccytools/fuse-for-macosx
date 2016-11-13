---
title: Compressed Files
description: This section describes Fuses support of compressed files.
order: 220
---

Snapshots, tape images, dock cartridges and input recording files (RZX) can be
read from files compressed with bzip2, gzip or zip just as if they were
uncompressed. In the zip case, only the first supported file found inside the
archive is loaded. There is currently no support for reading compressed +3, +D,
Opus or Beta disk images.
