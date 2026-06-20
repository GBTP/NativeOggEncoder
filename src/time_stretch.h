#ifndef NOE_TIME_STRETCH_H
#define NOE_TIME_STRETCH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t noe_time_stretch_planar(
    const float* const* input_channels,
    int32_t input_frames,
    int32_t channels,
    int32_t sample_rate,
    float speed,
    float** output_channels,
    int32_t* output_frames
);

void noe_time_stretch_free(float** channels, int32_t num_channels);

#ifdef __cplusplus
}
#endif

#endif
