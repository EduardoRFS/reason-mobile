#! /bin/sh

set -e
set -u

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
else
  exit 1;
fi

make -j8
