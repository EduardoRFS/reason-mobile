#!/bin/sh

set -e
set -x

ctypes_make () {
  make -j6 "OCAMLFIND=ocamlfind -toolchain $ESY_TOOLCHAIN" XEN=disable $@
}

ctypes_make libffi.config
ctypes_make ctypes-base ctypes-stubs

if cat libffi.config | grep libffi_available=true; then
  ctypes_make ctypes-foreign
fi
