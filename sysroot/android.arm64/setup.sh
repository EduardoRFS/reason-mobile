#! /bin/sh

set -e
set -u

export PATH="$PATH:$1"

TOOLCHAIN="android.arm64"
BASE_TRIPLE="aarch64-linux-android"
TARGET_TRIPLE="aarch64-linux-android24"

gen_tools () {
  TOOL_NAME="$1"
  HOST="$(which $TOOL_NAME)"
  TARGET="$(which $TARGET_TRIPLE-$TOOL_NAME || which $BASE_TRIPLE-$TOOL_NAME)"

  sysroot-gen-tools "$TOOLCHAIN" "$TARGET_TRIPLE" "$TOOL_NAME" "$TARGET" "$HOST"
}

gen_tools ar
gen_tools as
gen_tools clang
gen_tools clang++
gen_tools ld
gen_tools nm
gen_tools objdump
gen_tools ranlib
gen_tools strip

cat > $cur__install/toolchain.cmake <<EOF
set(ANDROID_ABI arm64-v8a)
set(ANDROID_NATIVE_API_LEVEL 24) # API level

include($NDK_ROOT/build/cmake/android.toolchain.cmake)
EOF
