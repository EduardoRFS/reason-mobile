#! /bin/sh

set -e
set -u

cp $HOST_REVERY/default/src/reason-skia/wrapped/lib/skia_generated_stubs.ml src/reason-skia/wrapped/lib
cp $HOST_REVERY/default/src/reason-skia/wrapped/lib/skia_generated_stubs.c src/reason-skia/wrapped/lib
cp $HOST_REVERY/default/src/reason-skia/wrapped/bindings/skia_generated_type_stubs.ml src/reason-skia/wrapped/bindings
