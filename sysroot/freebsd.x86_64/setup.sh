#! /bin/sh

set -e
set -u

ARCH="x86_64"
TOOLCHAIN="freebsd.x86_64"
BASE_TRIPLE="$ARCH-unknown-freebsd12.1"
TARGET_TRIPLE="$ARCH-unknown-freebsd12.1"

BASE="https://download.freebsd.org/ftp/releases/amd64/amd64/12.1-RELEASE/base.txz"

mkdir -p $cur__target_dir/base
curl -o base.txz $BASE
tar xvf base.txz
SYSROOT=$PWD

gen_tools () {
  TOOL_NAME="$1"
  TARGET_ARGS="$2"
  HOST="$(which $TOOL_NAME)"
  TARGET="$HOST $TARGET_ARGS"

  sysroot-gen-tools "$TOOLCHAIN" "$TARGET_TRIPLE" "$TOOL_NAME" "$TARGET" "$HOST"
}

## TODO: this should work only with x86_64 as target
CLANG_ARGS="--sysroot=$SYSROOT --target=$TARGET_TRIPLE"
LD_ARGS="--sysroot=$SYSROOT"

gen_tools ar ""
gen_tools as ""
gen_tools clang "$CLANG_ARGS"
gen_tools clang++ "$CLANG_ARGS"
gen_tools ld "$LD_ARGS"
gen_tools nm ""
gen_tools objdump ""
gen_tools ranlib ""
gen_tools strip ""

cat > $cur__install/toolchain.cmake <<EOF
set(CMAKE_SYSTEM_NAME FreeBSD)
set(CMAKE_SYSTEM_PROCESSOR $ARCH)

set(CMAKE_AR $TARGET_TRIPLE-ar)
set(CMAKE_RANLIB $TARGET_TRIPLE-ranlib)
set(CMAKE_C_COMPILER $TARGET_TRIPLE-gcc)
set(CMAKE_CXX_COMPILER $TARGET_TRIPLE-g++)
EOF
