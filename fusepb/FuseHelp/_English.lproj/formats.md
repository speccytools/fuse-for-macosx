---
title: Disk Formats
description: This section describes the disk formats supported by Fuse.
order: 180
group: Floppy Disk Interfaces
---

Fuse supports several disk image formats in its +D, Didaktik, DISCiPLE, Opus and
Beta 128 emulation.

The following are supported for reading:

Format | Description
:--- | :---
*.UDI* | Ultra Disk Image; for specification details please see [the format document](http://faqwiki.zxnet.co.uk/wiki/UDI_format). This is the only image format which can store all the relevant information of the recorded data on a magnetic disk, so it can be used for any non standard disk format.  Fuse can store and read both MFM and FM formatted disk data in/from this container.
*.FDI* | UKV Spectrum Debugger disk image format.
*.MGT, .IMG* | DISCiPLE/+D file formats.
*.SAD* | For compatibility with SAM Coupé disk images using these formats. Note that SAM Coupé '.DSK' images share the same format as '.MGT'.
*.D80, .D40* | Didaktik 80 and Didaktik 40 file formats.
*.TRD* | TR-DOS disk image. TRD and SCL sectors are loaded interleaved, therefore you might experience problems when these images are used with TR-DOS ROMs that use the turbo format (sequential sectors); for detailed information please see [this file on the RAMSOFT website](http://web.archive.org/web/20070808150548/http://www.ramsoft.bbk.org/tech/tr-info.zip)
*.SCL* | A simple archive format for TR-DOS disk files. For specification please see [this webpage](http://www.zx-modules.de/fileformats/sclformat.html)
*.TD0* | Teledisk image format; Fuse supports only files not created with "Advanced Compression". Detailed description found in [this document](http://www.classiccmp.org/dunfield/img54306/td0notes.txt) and [this document](https://web.archive.org/web/20130116072335/http://www.fpns.net/willy/wteledsk.htm).
*.DSK* | CPC disk image format; Fuse supports the plain old and the new extended CPC format too. Further information please see the The .DSK Format section and the CPCEMU manual section 7.7.1 [here](http://www.cpc-emu.org/linux/cpcemu_e.txt) or [here](http://www.cpctech.org.uk/docs/extdsk.html).
*.OPD, .OPU* | Opus Discovery file formats.

<br>
Fuse supports the *.UDI*, *.FDI*, *.MGT*, *.IMG*, *.SAD*, *.D80*, *.D40*,
*.TRD*, *.SCL*, *.OPD*, *.OPU* and *.DSK* (only the old CPC format) formats for
writing.

You can save disk images with any output format, just select the appropriate
type from the disk save dialog.

Not all image formats can store all disk image information. You cannot save a
disk image with an inappropriate format that loses some information (e.g.
variable track length or sector length).
