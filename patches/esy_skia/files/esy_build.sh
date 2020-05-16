#!/bin/sh

set -e
set -x

export PYTHON="$(which python2 || which python)"

$PYTHON tools/git-sync-deps
ln -s third_party/externals/gyp tools/gyp

TARGET_OS=$ESY_TOOLCHAIN_SYSTEM
if [ "$ESY_TOOLCHAIN_SYSTEM" == "windows" ]; then
  TARGET_OS="win"
fi

TARGET_CPU=$ESY_TOOLCHAIN_PROCESSOR
if [ "$ESY_TOOLCHAIN_PROCESSOR" == "x86_64" ]; then
  TARGET_CPU="x64"
fi

ADDITIONAL_ARGS=""
if [ "$ESY_TOOLCHAIN_SYSTEM" == "android" ]; then
  ADDITIONAL_ARGS="ndk=\"$NDK_ROOT\""
fi

# fix to compile from Linux to iOS and macOS
echo "print \"$ESY_TOOLCHAIN_SYSROOT\"" > "gn/find_ios_sysroot.py"
cat > gn/ar.py <<EOF
import os
import sys

ar, output, rspfile = sys.argv[1:]
os.system('rm -f %s; %s rcs %s \$(cat %s)' % (output, ar, output, rspfile))
EOF

bin/gn gen $cur__target_dir/out/Static \
  --script-executable=$PYTHON \
  --args=" \
    cc=\"$ESY_TOOLCHAIN_CC\" \
    cxx=\"$ESY_TOOLCHAIN_CXX\" \
    ar=\"$ESY_TOOLCHAIN_AR\" \
    target_os=\"$TARGET_OS\" \
    target_cpu=\"$TARGET_CPU\" \
    $ADDITIONAL_ARGS \
    is_debug=false \
    skia_use_system_libjpeg_turbo=true \
    extra_cflags=[\"-I$JPEG_INCLUDE_PATH\"] \
    extra_ldflags=[\"-L$JPEG_LIB_PATH\", \"-ljpeg\" ]"

ninja.exe -C $cur__target_dir/out/Static