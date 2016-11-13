---
title: Graphics Filters
description: This section describes the graphics filters for Fuse.
order: 80
group: Fuse Preferences
---

Fuse has the ability to apply essentially arbitrary filters between building its
image of the Spectrum's screen, and displaying it on the emulating machine's
monitor. These filters can be used to do various forms of smoothing, emulation
of TV scanlines and various other possibilities.

A complication arises due to the fact that the Timex machines have their
high-resolution video mode with twice the horizontal resolution. To deal with
this, Fuse treats these machines as having a 'normal' display size which is
twice the size of a normal Spectrum's screen, leading to a different set of
filters being available for these machines.

The available filters are:

Filter | Description
:--- | :---
*Timex half* |A Timex-machine specific filter which scales the screen down to half normal (Timex) size; that is, the same size as a normal Spectrum screen.
*Normal* |The simplest filter: just display one pixel for every pixel on the Spectrum's screen.
*Double size* |Scale the displayed screen up to double size.
*Triple size* |Scale the displayed screen up to triple size.
*2xSaI, Super 2xSaI, SuperEagle* |Three interpolating filters which apply successively more smoothing. All three double the size of the displayed screen.
*AdvMAME2x* |A double-sizing, non-interpolating filter which attempts to smooth diagonal lines.
*AdvMAME3x* |Very similar to AdvMAME2x, except that it triples the size of the displayed screen.
*TV 2x, TV 3x, Timex TV* |Three filters which attempt to emulate the effect of television scanlines. The first is a double-sizing filter for non-Timex machines, the second is a similar triple-sizing filter, while the last is a single-sizing filter for Timex machines (note that this means TV 2X and Timex TV produce the same size output).
*PAL TV, PAL TV 2x, PAL TV 3x* |Three filters which attempt to emulate the effect of the PAL TV system which layers a lower-resolution colour image over the top of a higher-resolution black-and-white image. The filters can also optionally add scanlines like the other TV series scalers.
*Dot matrix* |A double-sizing filter which emulates the effect of a dot-matrix display.
*Timex 1.5x* |An interpolating Timex-specific filter which scales the Timex screen up to 1.5x its usual size (which is therefore 3x the size of a 'normal' Spectrum screen).
*HQ 2x* |A double-sizing, interpolating high-quality magnification filter by Maxim Stepin. You can find more information at [Wikipedia](https://en.wikipedia.org/wiki/Hqx).
*HQ 3x* |Very similar to HQ 2x, except that it triples the size of the displayed screen.
