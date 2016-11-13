---
title: +D Emulation
description: This section describes the +D interface emulation in Fuse.
order: 110
group: Floppy Disk Interfaces
---

Fuse supports emulating the +D disk and printer interface. See the
[Disk File Formats](formats.html) section for more details on supported disk
file formats. The +D's printer port is emulated. (See the
[Printer Emulation](printer.html) section for more details). The +D may only be
used with 48K, 128K and +2 (not +2A) emulation. To access disks, load G+DOS, by
inserting a disk containing the DOS file (+SYS) and entering "RUN". Once DOS is
loaded, you can load to/from +D disks by prefixing file names with 'd n' where
' n ' is the number of the drive in use.  For example, `LOAD d1;"myfile"` would
load the file named 'myfile' from the emulated drive 1. Microdrive syntax may
also be used.

To save a snapshot, choose the *Machine > NMI* menu option, and then press '4'
to save a 48K snapshot, or '5' to save a 128K snapshot.

When saving a 128K snapshot, you must then press Y or N to indicate whether the
screen changed while saving the snapshot, to finish saving. You can also choose
'3' to save a screenshot to disk.

Holding Caps Shift together with any of these options will cause the +D to save
to the 'other' drive to the one used last.

Options '1' and '2' allow screenshots to be printed (in monochrome, in normal
and large formats respectively) if printer emulation is enabled. For saving and
loading of snapshots, and saving of screenshots to disk, G+DOS must be loaded
first, but printing of screenshots can be performed without loading G+DOS.

Finally, 'X' will return from the NMI menu.
