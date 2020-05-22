# hello-serverless

This is just an example, on how to build on macOS to Linux using musl, useful to run on serverless platforms like AWS Lambda.

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

```sh
# setup
cd hello-serverless
esy
node ../generate/dist/cli.js linux.musl.x86_64
esy @linux.musl.x86_64

# shell
esy @linux.musl.x86_64 not-esy-setup $SHELL

# build: should run inside the shell
CCOPT="-static" dune build -x linux.musl.x86_64 @install
```