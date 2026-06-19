#ifndef NATIVE_OGG_ENCODER_H
#define NATIVE_OGG_ENCODER_H

#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32
    #define NOE_EXPORT __declspec(dllexport)
#else
    #define NOE_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

NOE_EXPORT int32_t noe_encode_interleaved(
    const float* interleaved_samples,
    int32_t num_frames,
    int32_t channels,
    int32_t input_sample_rate,
    int32_t output_sample_rate,
    float quality,
    uint8_t** out_data,
    int32_t* out_size
);

NOE_EXPORT int32_t noe_encode_planar(
    const float* const* channel_ptrs,
    int32_t num_frames,
    int32_t channels,
    int32_t input_sample_rate,
    int32_t output_sample_rate,
    float quality,
    uint8_t** out_data,
    int32_t* out_size
);

NOE_EXPORT void noe_free(uint8_t* data);

NOE_EXPORT const char* noe_get_error(void);

#ifdef __cplusplus
}
#endif

#endif
