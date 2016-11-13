---
title: Recording Preferences
description: This section describes the Fuse Recording preferences dialog.
order: 40
group: Fuse Preferences
---

Use the Recording pane of Fuse preferences to configure how Fuse deals with RZX emulator input recordings.

OPTION | DESCRIPTION
:--- | :---
*Emulator recording create autosaves* | If this option is selected, Fuse will add a snapshot into the recording stream every 5 seconds while creating an RZX file, thus enabling the rollback facilities to be used without having to explicitly add snapshots into the stream. Older snapshots will be pruned from the stream to keep the file size and number of snapshots down: each snapshot up to 15 seconds will be kept, then one snapshot every 15 seconds until one minute, then one snapshot every minute until 5 minutes, and then one snapshot every 5 minutes. Note that this "pruning" applies only to automatically inserted snapshots: snapshots manually inserted into the stream will never be pruned.
*Emulator recording always embed snapshot* | Specify whether a snapshot should be embedded in an RZX file when recording is started from an existing snapshot.
*Emulator recording competition mode* | Any input recordings which are started when this option is selected will be made in "competition mode". In essence, this means that Fuse will act just like a real Spectrum would: you can't load snapshots, pause the emulation in any way, change the speed or anything that you couldn't do on the real machine. If any of these things are attempted, or if the emulated Fuse is running more than 5% faster or slower than normal Spectrum speed, then the recording will immediately be stopped.<br>Recordings made with competition mode active will be digitally signed, in theory to "certify" that it was made with the above restrictions in place. *However, this procedure is not secure (and cannot be made so), so the presence of any signature on an RZX file should not be taken as providing proof that it was made with competition mode active*. This feature is included in Fuse solely as it was one of the requirements for Fuse to be used in an on-line tournament.
*Emulator recording competition code* | The numeric code entered here will be written into any RZX files made in competition mode. This is another feature for on-line tournaments which can be used to "prove" that the recording was made after a specific code was released. If you're not playing in such a tournament, you can safely ignore this option.
