---
title: Menus And Keys
description: This section describes the Fuse menus and options.
order: 20
---

Since many of the keys available are devoted to emulation of the Spectrum's
keyboard, the primary way of controlling Fuse itself (rather than the emulated
machine) is via the menus. There are also shortcuts for some menu options.

Here's what the menu options do, along with the shortcuts for those items which have them:

MENU|SHORTCUT|DESCRIPTION
:--- | :--- | :---
*File > Open File…*|⌘-O|Open a Spectrum file. Snapshots will be loaded into memory; tape images will be inserted into the emulated tape deck, and if the *Auto-load tapes/disks* option is set will begin loading. Opening a disk image or a Timex dock image will cause the appropriate machine type (+3, Scorpion or TC2068) to be selected with the image inserted, and disks will automatically load if the *Auto-load tapes/disks* option is set.
*File > Save Snapshot As…*|Shift-⌘-S|Save a snapshot (machine state, memory contents, etc.). By default the file will be saved in SZX format, you can also save in the Z80 or SNA formats by selecting the appropriate format from the Format popup button.
*File > Emulator Recording Record…*||Start recording input to an RZX file, initialised from the current emulation state.
*File > Emulator Recording > Record from Snapshot…*||Start recording input to an RZX file, initialised from a snapshot. You will first be asked for the snapshot to use and then for the file to save the recording to.
*File > Emulator Recording > Continue Recording…*||Continue recording input into an existing RZX file from the last recorded state. Finalised recordings cannot be resumed. You will be prompted for the recording to continue.
*File > Emulator Recording > Insert Bookmark*|⌘-B|Inserts a bookmark of the current state into the RZX file. This can be used at a later point to roll back to the inserted state by using one of the commands below.
*File > Emulator Recording > Go To Last Bookmark*|⌘-Z|Rolls back the recording to the point at which the previous bookmark was inserted. Recording will continue from that point.
*File > Emulator Recording > Go To Bookmark…*||Roll back the recording to any bookmark which has been inserted into the recording.
*File > Emulator Recording > Play…*||Playback recorded input from an RZX file. This lets you replay keypresses recorded previously. RZX files generally contain a snapshot with the Spectrum's state at the start of the recording; if the selected RZX file doesn't, you'll be prompted for a snapshot to load as well.
*File > Emulator Recording > Stop*||Stop any currently-recording/playing RZX file.
*File > Emulator Recording > Finalise Recording…*||Compact an RZX file. Any interspersed snapshots will be removed and the recording cannot be continued. All action replays submitted to the RZX Archive should be finalised.
*File > AY Sound Recording > Record…*||Start recording the bytes output via the AY-3-8912 sound chip to a PSG file.
*File > AY Sound Recording > Stop*||Stop any current AY logging.
*File > Open Screenshot…*||Load an SCR screenshot (essentially just a binary dump of the Spectrum's video memory) onto the current screen. Fuse supports screenshots saved in the Timex hi-colour and hi-res modes as well as 'normal' Spectrum screens, and will make a simple conversion if a hi-colour or hi-res screenshot is loaded onto a non-Timex machine.
*File > Save Screenshot As…*||Save a copy of whatever's currently displayed on the Spectrum's screen as an SCR file.
*File > Export Screenshot…*||Save the current screen as any supported format (currently PNG, TIFF, BMP, JPG and GIF).
*File > Import Binary Data…*||Load binary data from a file into the Spectrum's memory. After selecting the file to load data from, you can choose where to load the data and how much data to load.
*File > Export Binary Data…*||Save an arbitrary chunk of the Spectrum's memory to a file. Select the file you wish to save to, followed by the location and length of data you wish to save.
*Machine > Reset*||Reset the emulated Spectrum.
*Machine > Hard Reset*||Reset the emulated Spectrum. A hard reset is equivalent to turning the Spectrum's power off, and then turning it back on.
*Machine > Debugger*||Start the monitor/debugger. See the [Monitor/Debugger](monitor.html) section for more information.
*Machine > POKE Finder*||Start the 'poke finder'. See the [POKE Finder](pokefinder.html) section for more information.
*Machine > POKEs/Cheats*||Allow one to use multiface POKEs for things such as infinite lives. See the [POKES/Cheats](pokememory.html) section for more information.
*Machine > Memory Browser*||Start the memory browser. It should be fairly obvious what this does; perhaps the only thing worth noting is that emulation is paused until you close the window.
*Machine > NMI*||Sends a non-maskable interrupt to the emulated Spectrum. Due to a typo in the standard 48K ROM, this will cause a reset, but modified ROMs are available which make use of this feature. When the +D (or DISCiPLE) is emulated, this is used to access the +D (or DISCiPLE)'s screenshot and snapshot features (see the [+D Emulation](plusd.html) and [DISCIPLE Emulation](disciple.html) sections for more information). For the DISCiPLE, Caps Shift must be held down whilst pressing the NMI button. Note that GDOS on the DISCiPLE contains a bug which causes corruption of saved snapshots, and a failure to return from the NMI menu correctly.  This bug is not present in G+DOS on the +D.
*Machine > Multiface Red Button*||Presses the Multiface One/128/3 red button to active the interface.
*Machine > Didaktik SNAP*||Presses the Didaktik 80 (or Didaktik 40)'s 'SNAP' button.
*Machine > Bind Keys to Joystick*|⌘-J|If this option is selected and the Keyboard joystick is configured via the [Preferences > Joysticks](input.html) dialog, the q, a, o, p, and space keys will not have their normal effect and will be used as the up, down, left, right and fire buttons of the joystick respectively. This setting can be toggled with ⌘-J.
*Machine > Use Recreated ZX Spectrum keyboard*|⌘-R|Enable the use of a Recreated ZX Spectrum in game mode. This is a Bluetooth keyboard in the shape of a 48K ZX Spectrum that can be paired your Mac to give an authentic feeling of using a real ZX Spectrum. While this mode is enabled, your Mac keyboard will not give correct input to the emulator. This setting can be toggled with ⌘-R.
*Machine > Use Shift with Cursor Keys*||Treat the keyboard arrow keys as shifted like the ZX Spectrum+ keyboard's arrow keys or as unshifted like a cursor joystick that maps to the 5, 6, 7 and 8 keys.
*Media > Tape > Open…*||Choose a PZX, TAP or TZX virtual-tape file to load from. If Auto-load tapes/disks is set in the [Preferences > General](general.html) dialog (as it is by default), the tape will begin loading. Otherwise, you have to start the load in the emulated machine (with `LOAD ""` or the 128's Tape Loader option, though you may need to reset first). To guarantee that TZX files will load properly, you should select the file, make sure tape-loading traps are disabled in the [Preferences > General](general.html) dialog, then select the *Tape > Play* option. That said, most TZXs will work with tape-loading traps enabled (often quickly loading partway, then loading the rest real-time), so you might want to try it that way first.
*Media > Tape > Play*||Start playing the PZX, TAP or TZX file, if required. (Choosing the option again pauses playback, and a further press resumes). To explain - if tape-loading traps have been disabled (in the [Preferences > General](general.html) dialog), starting the loading process in the emulated machine isn't enough. You also have to 'press play', so to speak :-), and this is how you do that. You may also need to 'press play' like this in certain other circumstances, e.g. TZXs containing multi-load games may have a stop-the-tape request (which Fuse obeys).
*Media > Tape > Browse*||Browse through the current tape. A brief display of each of the data blocks on the current tape will appear, from which you can select which block Fuse will play next. Emulation will continue while the browser is displayed; clicking on a block will select it and 'fast-foward' the virtual tape deck.
*Media > Tape > Rewind*||Rewind the current virtual tape, so it can be read again from the beginning.
*Media > Tape > Close*||Close the current virtual tape file. This is particularly useful when you want a 'clean slate' to add newly-saved files to, before doing *Tape, Save As…*
*Media > Tape > Save As…*||Write the current virtual-tape contents to a TZX file. The virtual-tape contents are the contents of the previously-loaded tape (if any has been loaded since you last did a Tape, Close), followed by anything you've saved from the emulated machine since. These newly-saved files are not written to any tape file until you choose this option!
*Media > Tape > Record*||Starts directly recording the output from the emulated Spectrum to the current virtual-tape. This is useful when you want to record using a non-standard ROM or from a custom save routine. Most tape operations are disabled during recording. Stop recording with the *Media > Tape > Stop Recording* menu option.
*Media > Tape > Stop Recording*||Stops the direct recording and places the new recording into the virtual-tape.
*Media > Microdrive*||Virtual Microdrive images are accessible only when the Interface I is active from the [Preferences > Peripherals](peripherals.html) menu. Note that any changes to the Microdrive image will not be written to the file on disk until the appropriate 'eject and write' option is used. Each Microdrive cartridge has it's own version of the following menus:
*Media > Microdrive 1 > Insert New*||Insert a new (unformatted) Microdrive cartridge into emulated Microdrive 1.
*Media > Microdrive 1 > Insert…*||Select a Microdrive cartridge image file to read/write into emulated Microdrive 1.
*Media > Microdrive 1 > Eject*||Eject the Microdrive image in Microdrive 1. If the image has been modified, you will be asked as to whether you want any changes saved.
*Media > Microdrive 1 > Save*||Save the Microdrive image in Microdrive 1, and then eject the image.
*Media > Microdrive 1 > Save as…*||Write the Microdrive image in Microdrive 1 to a new file, and then eject the image.  You will be prompted for a filename.
*Media > Microdrive 1 > Write protect > Enable*||Enable the write protect tab for the image in Microdrive 1.
*Media > Drive*||Virtual disk images are only accessible when emulating a disk interface, +3, Pentagon or Scorpion. If any of the disk options are selected while emulating a +3, they refer to the +3's disk drives, which both default to being the 3" type (in effect, the internal drive plus an external FD-1). With the usual +3 format, these have a capacity of 173K. If the Pentagon or Scorpion is being emulated, these options refer to the Beta disk drives. If any other machine is being emulated this menu cannot be selected unless a disk interface like the Opus Discovery, +D, DISCiPLE or Beta 128 is being emulated. (See the [.DSK Format](dsk.html) and [Beta Disk Formats](trdos.html) sections for notes on the file formats supported). Note that (since version 0.6.2), Fuse works with true virtual disk images: any changes made to a disk image will not affect the file which was 'inserted' into the drive. If you do want to keep any changes, use the appropriate 'eject and write' option before exiting Fuse.
*Media > Drive A: > New*||Insert a new (unformatted) disk cartridge into emulated drive A:/1.
*Media > Drive A: > Insert…*||Select a disk-image file to read/write in the  emulated drive A:/1.
*Media > Drive A: > Eject*||Deselect the disk image currently in the machine's drive A:/1 - or from the emulated machine's perspective, eject it. Note that any changes made to the image will not be saved.
*Media > Drive A: > Save*||Deselect the disk image currently in the machine's drive A:/1 and save the current state of the disk.
*Media > Drive A: > Save as…*||Deselect the disk image currently in the machine's drive A:/1 and save the current state of the disk to a new file. You will be prompted for a filename.
*Media > Drive A: > Flip*||Flip the disk image currently in the machine's drive A:/1, only applicable for single sided drives and two-sided disk images.
*Media > Drive A: > Write Protect*||Enable the write protect tab for the image in drive A:/1.
*Media > Drive B: > New*||As above, but for the machine's drive B:/1.
*Media > Drive B: > Insert…*||As above, but for the machine's drive B:/1.
*Media > Drive B: > Eject*||As above, but for drive B:/2.
*Media > Drive B: > Save*||As above, but for drive B:/2.
*Media > Drive B: > Save as…*||As above, but for drive B:/2.
*Media > Drive B: > Flip*||Flip the disk image currently in the machine's drive B:/2, only applicable for single sided drives and two-sided disk images.
*Media > Drive B: > Write Protect*||Enable the write protect tab for the image in drive B:/2.
*Media > Cartridge > Open…*||Insert a cartridge into the Timex 2068 dock or Interface II cartridge slot. This will cause the emulated machine to be reset.
*Media > Cartridge > Close*||Remove the cartridge from the Timex 2068 dock or Interface II cartridge slot. This will cause the emulated machine to be reset.
*Media > IDE Master > Insert…*||Connect an IDE hard disk to the current IDE interface's master channel.
*Media > IDE Master > Commit*||Cause any writes which have been done to virtual hard disk attached to the current IDE interface's master channel to be committed to the real disk, such that they survive the virtual disk being ejected.
*Media > IDE Master > Eject*||Eject the virtual hard disk from the current IDE interface's master channel. Note that any writes to the virtual hard disk will be lost unless the *Media > IDE Master > Commit* option is used before the disk is ejected.
*Media > IDE Slave > Insert…*||The same as the *Media > IDE Master* entries above, but for the current IDE interface's slave channel.
*Media > IDE Slave > Commit*||The same as the *Media > IDE Master* entries above, but for the current IDE interface's slave channel.
*Media > IDE Slave > Eject*||The same as the *Media > IDE Master* entries above, but for the current IDE interface's slave channel.
