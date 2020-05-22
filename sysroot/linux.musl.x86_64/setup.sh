#! /bin/sh

set -e
set -u

TOOLCHAIN="linux.musl.x86_64"
BASE_TRIPLE="x86_64-linux-musl"
TARGET_TRIPLE="x86_64-unknown-linux-musl"

gen_tools () {
  TOOL_NAME="$1"
  HOST="$(which $TOOL_NAME)"
  TARGET="$(which $BASE_TRIPLE-$TOOL_NAME || which $TOOL_NAME)"

  if [ "$(uname)" == "Linux" ] && [ "$TOOL_NAME" == "gcc" ]; then
    TARGET="REALGCC=$HOST musl-gcc"
  fi

  sysroot-gen-tools "$TOOLCHAIN" "$TARGET_TRIPLE" "$TOOL_NAME" "$TARGET" "$HOST"
}

gen_tools ar
gen_tools as
gen_tools gcc
gen_tools g++
gen_tools ld
gen_tools nm
gen_tools objdump
gen_tools ranlib
gen_tools strip
