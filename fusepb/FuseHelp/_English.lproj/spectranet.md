---
title: Spectranet Emulation
description: This section describes the Spectranet interface emulation in Fuse.
order: 140
---

The Spectranet is an ethernet network interface for the ZX Spectrum. The interface can be activated via the *Spectranet* option on the *Peripherals preferences* dialog, and the state of its automatic page-in (disable) jumper controlled via the *Spectranet disable* option. If you're going to be using the Spectranet, you'll probably want one of the firmwares available from the Spectranet [homepage](http://spectrum.alioth.net/doc/index.php/Main_Page) which is also where you can find more information on using the interface.

Installing the Spectranet firmware on Fuse is slightly more complicated than on a real machine, mostly because Fuse's emulation doesn't support DHCP. These instructions are correct as of 2012-01-26 - if you're using a later firmware than this, things may have changed slightly.

The first thing you will need to do is to obtain a copy of the Spectranet installer as a .tap file (or similar). The installer is available at the Spectranet [site](http://spectrum.alioth.net/doc/index.php/Spectranet_ROM_images).

Once you have a copy of the installer, start Fuse and tick the *Spectranet* and *Spectranet disable* options on the *Peripherals preferences* dialog. Once that's done, open the installer file (use the *Media > Tape > Open…* command rather than *File > Open…* to prevent autoloading) and enter the following commands from BASIC:

    CLEAR 26999
    LOAD "" CODE
    RANDOMIZE USR 27000

The screen should turn blue and you'll see around 20 lines of message appearing as the firmware is installed, starting with "Erasing sector 0" and finishing with "Restoring page B", and you'll get the familiar `0 OK, 0: 1` at the bottom of the screen.

Now untick the *Spectranet disable* option on the *Peripherals preferences* dialog and reset the Spectrum. You should see a very brief blue status screen, before the regular copyright screen appears with some Spectranet information at the top - there should be four status lines, starting with "Alioth Spectranet" and ending with the Spectranet's IP address (which will be 255.255.255.255 at this stage).

Now, trigger an NMI (the *Machine > NMI* menu option) and you should get a white on blue Spectranet NMI menu with five options.

Select [A] Configure network settings - this should lead you to another menu, which will scroll of the top of the screen; don't worry about this for now.

You'll now need to set various options:

* [A] Enable/disable DHCP - select N
* [B] Change IP address - enter the IP address of the machine you are running Fuse on.
* [C] Change netmask - enter the appropriate netmask for the IP address you selected above. If that doesn't mean anything to you, try 255.255.255.0
* [D] Change default gateway - enter the appropriate gateway address. If you don't know any better, enter the IP address of your router.
* [E] Change primary DNS - enter the address of your DNS server. If you don't know any better, use Google's public DNS server, 8.8.8.8.

There is no need to change options [F] or [G], but do select:

* [H] Change hostname - enter a hostname for the Spectranet-enabled machine. It doesn't really matter what you enter here - it's mostly useful just to replace the junk default name so you can see what you've entered for the other settings.

Your screen should now look something like this:

    Current configuration
    =====================
    Use DHCP         : No
    IP address       : 192.168.000.002
    Netmask          : 255.255.255.000
    Default gateway  : 192.168.000.001
    Primary DNS      : 192.168.000.001
    Secondary DNS    : 255.255.255.255
    Hardware address : FF:FF:FF:FF:FF:FF
    Hostname         : fuse
    
    <menu options>

If every looks correct, select [I] Save changes and exit (you'll see a brief "Saving configuration..." message) followed by [E] Exit, at which point you'll be returned to BASIC. Reset the Spectrum again and you'll see the same four line status display, but this time with your IP address on the last line.

Now type the following commands:

    %cfgnew
    %cfgcommit

Which will show the standard `0 OK, 0:1` at the bottom of the screen.

Congratulations! You have now installed the Spectranet firmware. To save having to go through all that every time you start Fuse, save a .szx snapshot at this point, and load that in every time you want to use the Spectranet.
