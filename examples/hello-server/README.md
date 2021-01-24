# hello-server

This is just an example, on how to build on macOS to Linux using musl, it's also useful to run on serverless platforms like AWS Lambda.

This package creates a docker image from scratch, the result is about ~3.59mb.

## Requirements

### Linux

Install `musl-gcc`
- ArchLinux: `pacman -S musl`

### Mac

Install `x86_64-linux-musl`

```sh
brew install FiloSottile/musl-cross/musl-cross
```

## Tutorial

To build a docker image

```sh
cd hello-server
./docker/build.sh
```