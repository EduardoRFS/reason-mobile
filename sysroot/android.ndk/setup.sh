#! /bin/sh

set -e
set -u

NDK_LINUX="https://dl.google.com/android/repository/android-ndk-r21b-linux-x86_64.zip"
NDK_DARWIN="https://dl.google.com/android/repository/android-ndk-r21b-darwin-x86_64.zip"
NDK_FOLDER="android-ndk-r21b"

if [[ "$(uname)" == 'Linux' ]]; then
  curl -o $cur__target_dir/ndk.zip $NDK_LINUX
elif [[ "$(uname)" == 'Darwin' ]]; then
  curl -o $cur__target_dir/ndk.zip $NDK_DARWIN
fi

unzip $cur__target_dir/ndk.zip -d $cur__target_dir
mv "$cur__target_dir/$NDK_FOLDER" $cur__install/ndk
