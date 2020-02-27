#! /bin/bash

set -u
set -e
set -o pipefail

NDK=/opt/android/ndk-r20b

# PATH
NDK_PATH=$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin
rmdir $cur__install/bin
ln -s $NDK_PATH $cur__install/bin

# sysroot
NDK_SYSROOT=$NDK/sysroot
ln -s $NDK_SYSROOT $cur__install/sysroot

# lib
ar x $NDK_SYSROOT/usr/lib/x86_64-linux-android/libc.a
cp sys_shm.o $cur__install/lib

# switch
platform x86_64 24

# cross-build.sh
NDK_COMPILER="x86_64-linux-android24-clang $cur__install/lib/sys_shm.o"
echo "#! /bin/bash
set -u
set -e
set -o pipefail


./esy-configure \
  -target x86_64-linux-android \
  -host x86_64-linux-android24 \
  -prefix \$cur__install \
  -flambda \
  -no-debugger \
  -no-cfi \
  -cc \"$NDK_COMPILER\" \
  -aspp \"$NDK_COMPILER -c\"

make -j12 world.opt
make install
" > $cur__install/lib/cross-build.sh
chmod +x $cur__install/lib/cross-build.sh
