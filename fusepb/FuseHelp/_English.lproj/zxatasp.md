---
title: ZXATASP and ZXCF
description: This section describes the ZXATASP and ZXCF interface emulation in Fuse.
order: 180
group: Hard Disk Interfaces
---

The ZXATASP and ZXCF interfaces are two peripherals designed by Sami Vehmaa
which significantly extend the capabilities of the Spectrum. More details on
both are available from [Sami's homepage](http://user.tninet.se/~vjz762w/),
but a brief overview is given here.

The real ZXATASP comes with either 128K or 512K of RAM and the ability to
connect an IDE hard disks and a CompactFlash card, while the ZXCF comes with
128K, 512K or 1024K of RAM and the ability to connect a CompactFlash card. From
an emulation point of view, the two interfaces are actually very similar as a
CompactFlash card is logically just an IDE hard disk. Currently, Fuse's
emulation is fixed at having 512K of RAM in the ZXATASP and 1024K in the ZXCF.

To activate the ZXATASP, simply select the *ZXATASP interface* option from the
[Peripherals preferences](peripherals.html) dialog. The state of the upload
and write protect jumpers is then controlled by the *ZXATASP upload* and
*ZXATASP write protect* options. Similarly, the ZXCF is controlled by the *ZXCF
interface* and *ZXCF upload* options (the ZXCF write protect is software
controlled).

If you're using either the ZXATASP or ZXCF, you almost certainly want to
investigate ResiDOS, the operating system designed for use with the ZXATASP and
ZXCF. ResiDOS provides facilities for using the extra RAM, accessing the mass
storage devices and a task manager allowing virtually instant switching between
programs on the Spectrum. See
[its homepage](http://www.worldofspectrum.org/residos/) for more details.
