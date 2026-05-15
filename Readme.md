# FuseX - Fuse Emulator Fork

This is a fork of Fuse (the Free Unix Spectrum Emulator), originally forked from [Fuse for macOS](https://sourceforge.net/p/fuse-for-macosx/fuse-for-macos) on SourceForge.

This fork is maintained for [speccytools](https://github.com/speccytools).

See [README](./README) for original instructions.

## Requirements

### On Mac

To build, do

```bash
sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer
```

To switch xcodebuild to use XCode, and then do

```bash
make
```

### On Windows

Install MINGW64, do

```
./build_win32.sh
```
