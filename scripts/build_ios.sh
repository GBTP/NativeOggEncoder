#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/.."
BUILD_DIR="$PROJECT_DIR/build/ios-arm64"
OUTPUT_DIR="$PROJECT_DIR/../AnoawaWorkspace/Anoawa/Assets/Plugins/NativeOggEncoder/aarch64-ios"

cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" \
    -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
    -DNOE_BUILD_SHARED=OFF \
    -DNOE_BUILD_STATIC=ON \
    -DCMAKE_BUILD_TYPE=Release

cmake --build "$BUILD_DIR" --config Release

mkdir -p "$OUTPUT_DIR"

# Merge all static libs into one
libtool -static -o "$OUTPUT_DIR/libnative_ogg_encoder.a" \
    "$BUILD_DIR/Release/libnative_ogg_encoder.a" \
    "$BUILD_DIR/deps/libogg/Release/libogg.a" \
    "$BUILD_DIR/deps/libvorbis/lib/Release/libvorbis.a" \
    "$BUILD_DIR/deps/libvorbis/lib/Release/libvorbisenc.a" \
    "$BUILD_DIR/deps/Release/libspeex_resampler.a"

echo "Done: $OUTPUT_DIR/libnative_ogg_encoder.a"
