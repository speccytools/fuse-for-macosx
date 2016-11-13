---
title: Movie Recording
description: This section describes Fuses support of movies.
order: 230
---

Fuse can save movies with sound in a specific file format (FMF). This recording
is very fast and has a moderate size, but you need to use the fmfconv program in
fuse‐utils to convert the format into regular video and/or audio file. The Movie
Compression Level preference allows you to set the compression level to None,
Lossless or High. The default is Lossless.  Recording a movie may slow down
emulation, if you experience performance problems, you can try to set
compression to None.

Fuse records every displayed frame, so by default the recorded file has about
50 video frames per second. A standard  video has about  24‐30 frames per second
framerate,  so if you set the Frame rate 1:n preference to 2 than recording
frame rate is reduced to about 25/s. The exact frame rate depends on the Z80
clock frequency which varies depending on the specific emulated machine.

Note: You can see all of the Spectrum graphics effects (for example those in
scene demos) only if the Fuse frame rate option is set to 1, but in most cases
like game software you can safely use a setting of 2. Also, movie recording
stops if the emulated machine is changed while recording is in progress.

The recorded number of channels matches the sound preferences which is set to
mono by default. You can record stereo sound if you use the AY stereo separation
preference.

You can use the fmfconv program from the fuse-utils package to convert a
FMF-format recorded movie file into a standard video file.
