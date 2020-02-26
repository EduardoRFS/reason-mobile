#! /bin/bash

export NDK=/opt/android/ndk-r20b
export NDK_PATH=$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin
export NDK_SYSROOT=$NDK/sysroot

rmdir $cur__install/bin
ln -s $NDK_PATH $cur__install/bin
ln -s $NDK_SYSROOT $cur__install/sysroot

ar x $NDK_SYSROOT/usr/lib/x86_64-linux-android/libc.a
cp sys_shm.o $cur__install/lib
