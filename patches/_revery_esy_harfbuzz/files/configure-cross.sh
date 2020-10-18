#! /bin/bash

cd _build

## TODO: should icu=no?
"./configure" \
  "--prefix=$cur__install" \
  "--build=$ESY_TOOLCHAIN_BUILD" \
  "--host=$ESY_TOOLCHAIN_HOST" \
  "--enable-shared=no" \
  "--enable-static=yes" \
  "--with-pic=yes" \
  "--disable-dependency-tracking" \
  "--with-icu=no" \
  "CC=$ESY_TOOLCHAIN_CC" \
  "CXX=$ESY_TOOLCHAIN_CXX"
