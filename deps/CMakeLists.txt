# libogg
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(INSTALL_PKG_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
set(INSTALL_CMAKE_PACKAGE_MODULE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(libogg)

# libvorbis
set(OGG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libogg/include)
set(OGG_LIBRARY ogg)
add_subdirectory(libvorbis)

# speex resampler (vendored)
add_library(speex_resampler STATIC
    speex_resampler/resample.c
)
target_include_directories(speex_resampler PUBLIC speex_resampler)
target_compile_definitions(speex_resampler PUBLIC
    OUTSIDE_SPEEX
    RANDOM_PREFIX=noe
)
target_compile_definitions(speex_resampler PRIVATE
    FLOATING_POINT
    EXPORT=
)
if(NOT MSVC)
    target_link_libraries(speex_resampler PRIVATE m)
endif()

# signalsmith-stretch (header-only C++)
add_library(signalsmith_linear INTERFACE)
target_include_directories(signalsmith_linear INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/signalsmith-linear/include)

add_library(signalsmith_stretch INTERFACE)
target_include_directories(signalsmith_stretch INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/signalsmith-stretch)
target_link_libraries(signalsmith_stretch INTERFACE signalsmith_linear)
