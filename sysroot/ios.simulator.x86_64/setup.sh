#! /bin/sh

set -e
set -u

TOOLCHAIN="ios.simulator.x86_64"

SDK_VERSION="13.2"
SDK_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk"

IOS_ARCH="x86_64"
IOS_VERSION="11.4.1"
IOS_TRIPLE="$IOS_ARCH-apple-ios$IOS_VERSION-simulator"

DARWIN_ARCH="x86_64"
DARWIN_VERSION="17.7.0"
DARWIN_TRIPLE="$DARWIN_ARCH-apple-darwin$DARWIN_VERSION"

## TODO: inline not-esy-gen-tools here
gen_tools () {
  TOOL_NAME="$1"
  TARGET_ARGS="$2"
  HOST="$(which $TOOL_NAME)"
  TARGET="$HOST $TARGET_ARGS"
  if [ "$(uname)" != "Darwin" ]; then
    TARGET="$(which "$DARWIN_TRIPLE-$TOOL_NAME") $TARGET_ARGS"
  fi

  sysroot-gen-tools "$TOOLCHAIN" "$DARWIN_TRIPLE" "$TOOL_NAME" "$TARGET" "$HOST"
}

ARCH_ARGS="-arch $IOS_ARCH"
## TODO: why we cannot use target=$IOS_TRIPLE
CLANG_ARGS="-isysroot $SDK_SYSROOT --target=$DARWIN_TRIPLE -miphoneos-version-min=$IOS_VERSION -arch $IOS_ARCH"
## TODO: platform_version name shouldn't be hard coded to ios
LD_ARGS="$ARCH_ARGS -ios_simulator_version_min $IOS_VERSION -syslibroot $SDK_SYSROOT"

gen_tools ar ""
gen_tools as "$ARCH_ARGS"
gen_tools clang "$CLANG_ARGS"
gen_tools clang++ "$CLANG_ARGS"
gen_tools ld "$LD_ARGS"
gen_tools nm "$ARCH_ARGS"
if [ "$(uname)" == "Darwin" ]; then
  gen_tools objdump "$ARCH_ARGS"
fi
gen_tools ranlib ""
gen_tools strip "$ARCH_ARGS"
## TODO: should add ldid?

## CMAKE
cat > $cur__install/toolchain.cmake <<EOF
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR $DARWIN_ARCH)

set(CMAKE_OSX_SYSROOT $SDK_SYSROOT)

set(CMAKE_AR $DARWIN_TRIPLE-ar)
set(CMAKE_RANLIB $DARWIN_TRIPLE-ranlib)
set(CMAKE_C_COMPILER $DARWIN_TRIPLE-clang)
set(CMAKE_CXX_COMPILER $DARWIN_TRIPLE-clang++)
EOF

if [ "$(uname)" != "Darwin" ]; then
  ln -s $(which "$DARWIN_TRIPLE-install_name_tool") $cur__bin/install_name_tool
fi
