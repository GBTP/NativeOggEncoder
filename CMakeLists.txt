cmake_minimum_required(VERSION 3.16)
project(NativeOggEncoder C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

option(NOE_BUILD_SHARED "Build shared library" ON)
option(NOE_BUILD_STATIC "Build static library" OFF)

add_subdirectory(deps)

set(NOE_SOURCES
    src/native_ogg_encoder.c
    src/time_stretch.cpp
)

if(NOE_BUILD_SHARED)
    add_library(native_ogg_encoder SHARED ${NOE_SOURCES})
    target_include_directories(native_ogg_encoder PUBLIC include)
    target_link_libraries(native_ogg_encoder PRIVATE ogg vorbis vorbisenc speex_resampler signalsmith_stretch)

    if(ANDROID)
        target_link_options(native_ogg_encoder PRIVATE -Wl,--gc-sections -Wl,--strip-all -Wl,-z,max-page-size=16384)
    elseif(UNIX AND NOT APPLE)
        target_link_options(native_ogg_encoder PRIVATE -Wl,-z,max-page-size=16384)
    endif()
endif()

if(NOE_BUILD_STATIC)
    add_library(native_ogg_encoder_static STATIC ${NOE_SOURCES})
    target_include_directories(native_ogg_encoder_static PUBLIC include)
    target_link_libraries(native_ogg_encoder_static PRIVATE ogg vorbis vorbisenc speex_resampler signalsmith_stretch)
    set_target_properties(native_ogg_encoder_static PROPERTIES OUTPUT_NAME native_ogg_encoder)
endif()
