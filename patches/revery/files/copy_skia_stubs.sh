#! /bin/sh

set -e
set -u

cp $HOST_REVERY/default/packages/reason-skia/src/wrapped/lib/skia_generated_stubs.ml packages/reason-skia/src/wrapped/lib
cp $HOST_REVERY/default/packages/reason-skia/src/wrapped/lib/skia_generated_stubs.c packages/reason-skia/src/wrapped/lib
cp $HOST_REVERY/default/packages/reason-skia/src/wrapped/bindings/skia_generated_type_stubs.ml packages/reason-skia/src/wrapped/bindings
