---
title: Inputs Preferences
description: This section describes the Fuse inputs preferences dialog.
order: 50
group: Fuse Preferences
---

Fuse can emulate many of the common types of joystick which were available for
the Spectrum. The input for these emulated joysticks can be taken from real
joysticks attached to the emulating machine, or from the q, a, o, p, and space
keys on the emulating machines keyboard, configured via the *Real device*
option. You can also configure which joystick axes to use on gamepads with more than one joystick/pad.

Note that when using the keyboard to emulate a joystick, the q, a, o, p, and
space keys will not have their normal effect (to avoid problems with games which
do things like use p for pause when using a joystick). See also the *Machine >
Bind Keys to Joystick* option.

Each of the joysticks (including the 'fake' keyboard joystick) can be configured
to emulate any one of the following joystick types:

JOYSTICK | DESCRIPTION
:--- | :---
*None* | No joystick: any input will simply be ignored.
*Cursor* | A cursor joystick, equivalent to pressing 5 (left), 6 (down), 7 (up), 8 (right), and 0 (fire).
*Kempston* | A Kempston joystick, read from input port 31. Note that the *Peripherals preferences > Kempston interface* option must also be set for the input to be recognised.
*Sinclair 1, Sinclair 2* | The 'left' and 'right' Sinclair joysticks, equivalent to pressing 1 (left), 2 (right), 3 (down), 4 (up), and 5 (fire), or 6 (left), 7 (right), 8 (down), 9 (up), and 0 (fire) respectively.
*Timex 1, Timex 2* | The 'left' and 'right' joysticks as attached to the Timex 2068s built-in joystick interface.

<br>
For the real joysticks, it is also possible to configure what effect each button on the joystick will have: this can be Joystick Fire, equivalent to presing the emulated joystick's fire button, Nothing, meaning to have no effect, or any Spectrum key, meaning that pressing that button will be equivalent to pressing that Spectrum key.

## Other options

OPTION | DESCRIPTION
:--- | :---
*Interface II* | If this option is selected, Fuse will emulate a cartridge port as found on the Interface II. Cartridges can then be inserted and removed via the *Media > Cartridge* menu. Note that the Pentagon, Scorpion, Interface II, ZXATASP and ZXCF all use the same hardware mechanism for accessing some of their extended features, so only one of these should be selected at once or unpredictable behaviour will occur.
*Kempston joystick interface* | If this option is selected, Fuse will emulate a Kempston joystick interface (probably the most widely supported type on the Spectrum). Note that this option is basically equivalent to plugging the interface itself into a Spectrum, not to connecting a joystick; this affects how the Spectrum responds to a read of input port 31. To use a Kempston joystick in a game, this option must be enabled, and you must also select a Kempston joystick above.
*Kempston mouse* | If this option is selected, Fuse will emulate a Kempston mouse interface. If you're using Fuse full-screen, your mouse is automatically used as if attached to the Kempston interface. Otherwise, you'll need to click on the Spectrum display in order to tell Fuse to grab the pointer (and make it invisible); to tell Fuse to release it, press Escape.
*Swap mouse buttons* | If this option is enabled, the left and right mouse buttons will be swapped when emulating a Kempston mouse.
