#! /bin/sh

set -e
set -u

ln -s $NDK_ROOT $cur__install/ndk

for FILE in $NDK_PREBUILT/bin/aarch64-linux-android*; do

FILE_ALIAS="$cur__bin/$(basename $FILE)"
cat > "$FILE_ALIAS" <<EOF
#!/bin/sh
$FILE "\$@"
EOF
chmod +x "$FILE_ALIAS"

done

cat > $cur__install/toolchain.cmake <<EOF
set(ANDROID_ABI arm64-v8a)
set(ANDROID_NATIVE_API_LEVEL 24) # API level

include($NDK_ROOT/build/cmake/android.toolchain.cmake)
EOF
