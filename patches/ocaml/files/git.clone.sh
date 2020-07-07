#! /bin/sh

set -e
set -x

BRANCH="4.10"
if [ "$ESY_TOOLCHAIN_SYSTEM" == "android" ]; then
  BRANCH="4.10+android"
fi
if [ "$ESY_TOOLCHAIN_SYSTEM" == "ios" ]; then
  BRANCH="4.10+ios"
fi

git clone \
  --single-branch --depth 1 \
  --branch $BRANCH \
  https://github.com/EduardoRFS/ocaml.git
