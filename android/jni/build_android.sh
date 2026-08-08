#!/bin/bash
set -euo pipefail

# Build `libljd.so` for Android ABIs using the NDK standalone toolchain or
# CMake with the NDK. This script expects:
#   ANDROID_NDK=/path/to/android-ndk<ver>
#   NDK_TOOLCHAIN=$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64
#
# Usage:
#   ./android/jni/build_android.sh
#   ./android/jni/build_android.sh arm64-v8a armeabi-v7a x86 x86_64

NDK="${ANDROID_NDK:-/opt/android-ndk}"
SYSROOT="${NDK}/toolchains/llvm/prebuilt/linux-x86_64"
API=21

build_one() {
  local ABI="$1"
  local TRIPLE="$2"
  local CC="${SYSROOT}/bin/${TRIPLE}${API}-clang"
  local CXX="${SYSROOT}/bin/${TRIPLE}${API}-clang++"
  local OUTDIR="$3"

  mkdir -p "$OUTDIR"

  $CC -c \
    -I. -Ilibrary/include \
    -DLJD_BUILD=1 \
    -fPIC \
    -std=c++17 -Wall -Wextra -Werror \
    bytecode/bytecode.cpp \
    bytecode/prototype.cpp \
    ast/ast.cpp \
    lua/lua.cpp \
    library/src/ljd_api.cpp \
    -o "$OUTDIR/ljd.o"

  $CXX -c \
    -I. -Ilibrary/include -Iandroid/native-lib/src/main/jni \
    -DLJD_BUILD=1 \
    -fPIC \
    -std=c++17 -Wall -Wextra -Werror \
    library/src/ljd_jni.cpp \
    -o "$OUTDIR/ljd_jni.o"

  $CXX -shared \
    "$OUTDIR/ljd.o" "$OUTDIR/ljd_jni.o" \
    -llog \
    -o "$OUTDIR/libljd.so"

  echo "Built $OUTDIR/libljd.so"
}

mkdir -p android-build/armeabi-v7a android-build/arm64-v8a android-build/x86 android-build/x86_64

if [[ $# -eq 0 ]]; then
  set -- arm64-v8a armeabi-v7a x86 x86_64
fi

for ABI; do
  case "$ABI" in
    armeabi-v7a)
      build_one "$ABI" "armv7a-linux-androideabi" "android-build/$ABI"
      ;;
    arm64-v8a)
      build_one "$ABI" "aarch64-linux-android" "android-build/$ABI"
      ;;
    x86)
      build_one "$ABI" "i686-linux-android" "android-build/$ABI"
      ;;
    x86_64)
      build_one "$ABI" "x86_64-linux-android" "android-build/$ABI"
      ;;
    *)
      echo "Unknown ABI: $ABI" >&2
      exit 1
      ;;
  esac
done
