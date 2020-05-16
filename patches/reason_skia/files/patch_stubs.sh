#! /bin/sh

set -e
set -u

STUBS_PATCH="stubs.$ESY_TOOLCHAIN_SYSTEM.$ESY_TOOLCHAIN_PROCESSOR.patch"

## TODO: it seems like android.arm64 and ios.arm64 are equal

if test -f "$STUBS_PATCH"; then
  patch -p1 < "$STUBS_PATCH"
else
  echo "we don't have stubs for this system: $STUBS_PATCH"
  exit 1
fi
