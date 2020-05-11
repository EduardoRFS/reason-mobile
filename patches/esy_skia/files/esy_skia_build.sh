#!/bin/sh

set -e
set -x

export PYTHON="$(which python2 || which python)"

$PYTHON tools/git-sync-deps
ln -s third_party/externals/gyp tools/gyp

SYSTEM_ARGS="target_cpu=\"arm64\" ndk=\"$NDK_ROOT\""

bin/gn gen $cur__target_dir/out/Static \
  --script-executable=$PYTHON \
  --args=" \
    cc=\"$ESY_TOOLCHAIN_CC\" \
    cxx=\"$ESY_TOOLCHAIN_CXX\" \
    $SYSTEM_ARGS \
    is_debug=false \
    skia_use_system_libjpeg_turbo=true \
    extra_cflags=[\"-I$JPEG_INCLUDE_PATH\"] \
    extra_ldflags=[\"-L$JPEG_LIB_PATH\", \"-ljpeg\" ]"

ninja.exe -C $cur__target_dir/out/Static