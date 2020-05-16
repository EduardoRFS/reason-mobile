#! /bin/sh

export PKG_CONFIG_PATH=
export PKG_CONFIG_LIBDIR=

ENABLE_VIDEO_X11="no"
if [ "$ESY_TOOLCHAIN_SYSTEM" == "linux" ]; then
  ENABLE_VIDEO_X11="yes"
fi

ENABLE_RENDER_METAL="no"
if [ "$ESY_TOOLCHAIN_SYSTEM" == "ios" ]; then
  ENABLE_RENDER_METAL="yes"
fi

HOST="$ESY_TOOLCHAIN_HOST"
if [ "$ESY_TOOLCHAIN_SYSTEM" == "ios" ]; then
  ## TODO: perhaps upstream that? Because jeez that's a weird triple
  PROCESSOR="arm"
  if [ "$ESY_TOOLCHAIN_PROCESSOR" == "x86_64" ]; then
    PROCESSOR="x86_64"
  fi
  HOST="$PROCESSOR-ios-darwin$ESY_TOOLCHAIN_DARWIN_KERNEL"
fi

./configure \
  --prefix=$cur__install \
  --build=$ESY_TOOLCHAIN_BUILD \
  --host=$HOST \
  --enable-video-x11=$ENABLE_VIDEO_X11 \
  --enable-render-metal=$ENABLE_RENDER_METAL \
  --with-sysroot=$ESY_TOOLCHAIN_SYSROOT \
  CC=$ESY_TOOLCHAIN_CC \
  CXX=$ESY_TOOLCHAIN_CXX \
  CFLAGS="-fobjc-weak"

if [ "$ESY_TOOLCHAIN_SYSTEM" == "ios" ]; then
  cp include/SDL_config_iphoneos.h include/SDL_config.h
fi