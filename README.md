# ReasonML Mobile

This repository is designed to provide tooling's to cross compile OCaml and ReasonML to Android and iOS. But it also should work for any platform including Linux and Windows.

## Requirements

- esy@0.6.7
- bash

## Example

To compile the hello-reason project for android arm64:

```sh
## build for host
cd hello-reason
esy

## generate the wrapper
esy add -D generate@EduardoRFS/reason-mobile:generate.json
esy generate android.arm64

## build for android.arm64
esy @android.arm64
```

As most esy projects the binary will be available at `#{self.target_dir}`

```sh
esy @android.arm64 file "#{self.target_dir}/default.android.arm64/bin/hello.exe"
```

## Status

Currently a couple of big projects can be built with small or zero changes, `esy` itself can also be built using this project, allowing it to run inside Android, iOS or any Linux box with a musl and static build.

### Android

Both `android.arm64` and `android.x86_64` are supposed to work out of box, as it has no dependency in the system and will download the toolchain from `android.ndk`.

### iOS

On macOS it requires the Xcode SDK, install Xcode for that.
On Linux it requires `cctools-ports` installed globally and the SDK in the right path at `/Applications`

With that done, `ios.arm64` and `ios.simulator.x86_64` should work

### Linux Musl

For macOS you should follow this `https://github.com/FiloSottile/homebrew-musl-cross`
If you have `aarch64-linux-musl-gcc`, `linux.musl.arm64` should work
If you have `x86_64-linux-musl-gcc`, `linux.musl.x86_64` should work

For Linux both should work out of the box, as it will download the toolchain from `https://musl.cc/`

## What it is

### Generate

A package for esy that will automagically generate the needed assets to cross compile using esy.

### Patches

A collection of patches designed to solve packages that aren't solved by the hacks implemented on the generate or are package specific, like the cstubs who cannot be generated in a cross compilation environment, we use the host ones to solve the problem.

### Sysroot

An esy package, making the compiler + sysroot available to esy. Currently we target the following environments

| OS            | ARCH   | ENV    |
| ------------- | ------ | ------ |
| Android       | arm64  | 24     |
| Android       | x86_64 | 24     |
| iOS           | arm64  | 17.7.0 |
| iOS.simulator | x86_64 | 17.7.0 |
| Linux         | arm64  | musl   |
| Linux         | x86_64 | musl   |
