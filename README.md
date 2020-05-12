# ReasonML Mobile

> 🚧 This is some heavily WIP idea / code / structure 🚧

> ⚠️ if you need help contact me on twitter or discord ⚠️

> ⚠️ if you want paid help ... twitter or discord ⚠️

This repository is designed to provide some tooling to cross compile using esy to Android and iOS

## Example

To compile the hello-reason project for android(arm64):

```sh
# setup
cd hello-reason
esy
node ../generate/dist/cli.js android
esy @android

# build
esy @android not-esy-setup dune build -x android

# shell
esy @android not-esy-setup $SHELL

# any command here will be in the right env
```

## Status

Currently [Revery](https://github.com/revery-ui/revery) is used as a baseline and is tested compiling from Linux and Mac to other platforms

### Android

We can already cross compile to it and the bits that aren't possible to cross compile, namely `reason-skia` cstubs were patched for arm64.

Only arm64 was tested, but x86_64 should also be working

### iOS

Currently you can compile simple examples, like the `hello-reason`, you still cannot compile Revery as there is some patches missing for iOS.

Only arm64 was tested, but x86_64 should also be working

## What it is

### Toolchain

A docker image ready to run the target binaries if needed, and with some tooling, more details at [TOOLCHAIN.md](./TOOLCHAIN.md)

### Generate

A script and some helpers to assist esy when doing cross compilation, it's an ad-hoc solution to concentrate a some hacks to use packages that weren't originally designed to it.

### Patches

A collection of patches designed to solve packages that aren't solved by the hacks implmented on the generate or are package specific.

### Sysroot

An esy package, making the compiler + sysroot available to esy. Currently only Android 24 arm64 is available
