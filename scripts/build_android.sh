#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/.."
BUILD_DIR="$PROJECT_DIR/build/android-arm64"
OUTPUT_DIR="$PROJECT_DIR/../AnoawaWorkspace/Anoawa/Assets/Plugins/NativeOggEncoder/aarch64-android"

NDK=${ANDROID_NDK_ROOT:?Set ANDROID_NDK_ROOT}
TOOLCHAIN=$NDK/build/cmake/android.toolchain.cmake

cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-21 \
    -DNOE_BUILD_SHARED=ON \
    -DNOE_BUILD_STATIC=OFF \
    -DCMAKE_BUILD_TYPE=Release

cmake --build "$BUILD_DIR" --parallel

mkdir -p "$OUTPUT_DIR"
cp "$BUILD_DIR/libnative_ogg_encoder.so" "$OUTPUT_DIR/"
echo "Done: $OUTPUT_DIR/libnative_ogg_encoder.so"
