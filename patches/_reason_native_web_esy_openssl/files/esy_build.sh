#! /bin/sh

set -e
set -u

export OCAMLFIND_TOOLCHAIN=$ESY_TOOLCHAIN
if [ "$ESY_TOOLCHAIN_SYSTEM" == "android" ]; then
  export ANDROID_NDK_HOME="$NDK_ROOT"
  export PATH="$NDK_PREBUILT/bin:$PATH"

  ## TODO: this API shouldn't be hardcoded
  ANDROID_API="24"

  ./openssl/Configure \
    --prefix=$cur__install \
    --openssldir=$cur__install/ssl \
    android-$ESY_TOOLCHAIN_PROCESSOR \
    -D__ANDROID_API__=$ANDROID_API
elif [ "$ESY_TOOLCHAIN_SYSTEM" == "ios" ]; then
  if [ "$ESY_TOOLCHAIN_PROCESSOR" == "x86_64" ]; then
    export CROSS_TOP="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer"
    export CROSS_SDK="iPhoneSimulator.sdk"
  else
    export CROSS_TOP="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer"
    export CROSS_SDK="iPhoneOS.sdk"
  fi
  export CC="$ESY_TOOLCHAIN_CC"

  ./openssl/Configure \
    --prefix=$cur__install \
    --openssldir=$cur__install/ssl \
    iphoneos-cross
else
  ./openssl/config --prefix=$cur__install --openssldir=$cur__install/ssl --cross-compile-prefix=${ESY_TOOLCHAIN_HOST}-;
fi

make -j8
