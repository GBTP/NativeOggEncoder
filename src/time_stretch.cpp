#include "time_stretch.h"
#include "native_ogg_encoder.h"

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>

#include "signalsmith-stretch.h"

extern "C" {

int32_t noe_time_stretch_planar(
    const float* const* input_channels,
    int32_t input_frames,
    int32_t channels,
    int32_t sample_rate,
    float speed,
    float** output_channels,
    int32_t* output_frames)
{
    if (!input_channels || !output_channels || !output_frames)
        return -1;
    if (channels < 1 || channels > 2 || input_frames <= 0 || sample_rate <= 0)
        return -1;
    if (speed <= 0.0f)
        return -1;

    int32_t out_frames = (int32_t)std::round((double)input_frames / (double)speed);
    *output_frames = out_frames;

    for (int ch = 0; ch < channels; ch++) {
        output_channels[ch] = (float*)malloc(out_frames * sizeof(float));
        if (!output_channels[ch]) {
            for (int j = 0; j < ch; j++) free(output_channels[j]);
            return -1;
        }
        memset(output_channels[ch], 0, out_frames * sizeof(float));
    }

    signalsmith::stretch::SignalsmithStretch<float> stretcher;
    stretcher.presetDefault(channels, (float)sample_rate);
    stretcher.setTransposeFactor(1.0f);

    int input_latency = stretcher.inputLatency();
    int output_latency = stretcher.outputLatency();

    int block_size = 1024;
    int input_pos = 0;
    int output_pos = 0;

    // Pre-roll: feed silence to fill internal latency
    if (input_latency > 0) {
        std::vector<float> silence(input_latency, 0.0f);
        std::vector<float> discard(output_latency + block_size, 0.0f);
        std::vector<const float*> in_ptrs(channels);
        std::vector<float*> out_ptrs(channels);
        for (int ch = 0; ch < channels; ch++) {
            in_ptrs[ch] = silence.data();
            out_ptrs[ch] = discard.data();
        }
        int pre_out = (int)std::round((double)input_latency / (double)speed);
        stretcher.process(in_ptrs.data(), input_latency, out_ptrs.data(), pre_out);
    }

    while (output_pos < out_frames) {
        int out_chunk = std::min(block_size, out_frames - output_pos);
        int in_chunk = (int)std::round((double)out_chunk * (double)speed);
        in_chunk = std::min(in_chunk, input_frames - input_pos);

        if (in_chunk <= 0 && output_pos < out_frames) {
            // Input exhausted, feed silence for remaining output
            std::vector<float> silence(block_size, 0.0f);
            std::vector<const float*> in_ptrs(channels);
            std::vector<float*> out_ptrs(channels);
            for (int ch = 0; ch < channels; ch++) {
                in_ptrs[ch] = silence.data();
                out_ptrs[ch] = output_channels[ch] + output_pos;
            }
            int remaining = out_frames - output_pos;
            stretcher.process(in_ptrs.data(), 0, out_ptrs.data(), remaining);
            output_pos = out_frames;
            break;
        }

        std::vector<const float*> in_ptrs(channels);
        std::vector<float*> out_ptrs(channels);
        for (int ch = 0; ch < channels; ch++) {
            in_ptrs[ch] = input_channels[ch] + input_pos;
            out_ptrs[ch] = output_channels[ch] + output_pos;
        }

        stretcher.process(in_ptrs.data(), in_chunk, out_ptrs.data(), out_chunk);

        input_pos += in_chunk;
        output_pos += out_chunk;
    }

    return 0;
}

void noe_time_stretch_free(float** channels, int32_t num_channels)
{
    if (!channels) return;
    for (int32_t ch = 0; ch < num_channels; ch++) {
        free(channels[ch]);
        channels[ch] = NULL;
    }
}

NOE_EXPORT int32_t noe_encode_planar_stretched(
    const float* const* channel_ptrs,
    int32_t num_frames,
    int32_t channels,
    int32_t input_sample_rate,
    int32_t output_sample_rate,
    float speed,
    float quality,
    uint8_t** out_data,
    int32_t* out_size)
{
    extern void noe_clear_error(void);
    extern void noe_set_error(const char* msg);

    noe_clear_error();

    if (std::fabs(speed - 1.0f) < 0.001f) {
        return noe_encode_planar(channel_ptrs, num_frames, channels,
                                 input_sample_rate, output_sample_rate,
                                 quality, out_data, out_size);
    }

    if (!channel_ptrs || !out_data || !out_size || channels < 1 || channels > 2 || num_frames <= 0) {
        noe_set_error("invalid parameters");
        return -1;
    }
    if (input_sample_rate <= 0 || output_sample_rate <= 0 || speed <= 0.0f) {
        noe_set_error("invalid parameters");
        return -1;
    }

    // Resample if needed
    const float* resample_input[2];
    float* resampled_bufs[2] = {NULL, NULL};
    int32_t resample_frames = num_frames;

    if (input_sample_rate != output_sample_rate) {
        extern float* resample_channel(const float* input, int32_t in_frames,
                                       int32_t in_rate, int32_t out_rate, int32_t* out_frames);

        for (int ch = 0; ch < channels; ch++) {
            int32_t out_frames;
            resampled_bufs[ch] = resample_channel(channel_ptrs[ch], num_frames,
                                                   input_sample_rate, output_sample_rate, &out_frames);
            if (!resampled_bufs[ch]) {
                for (int j = 0; j < ch; j++) free(resampled_bufs[j]);
                noe_set_error("resampler failed");
                return -2;
            }
            resample_input[ch] = resampled_bufs[ch];
            resample_frames = out_frames;
        }
    } else {
        for (int ch = 0; ch < channels; ch++) {
            resample_input[ch] = channel_ptrs[ch];
        }
    }

    // Time stretch
    float* stretched_bufs[2] = {NULL, NULL};
    int32_t stretched_frames = 0;

    int32_t stretch_result = noe_time_stretch_planar(
        resample_input, resample_frames, channels,
        output_sample_rate, speed,
        stretched_bufs, &stretched_frames);

    // Free resample buffers
    for (int ch = 0; ch < channels; ch++) free(resampled_bufs[ch]);

    if (stretch_result != 0) {
        noe_set_error("time stretch failed");
        return -5;
    }

    // Encode the stretched PCM
    const float* encode_input[2] = {stretched_bufs[0], stretched_bufs[1]};

    extern int32_t encode_core(const float* const* channels, int32_t num_frames,
                               int32_t num_channels, int32_t sample_rate, float quality,
                               uint8_t** out_data, int32_t* out_size);

    int32_t result = encode_core(encode_input, stretched_frames, channels,
                                  output_sample_rate, quality, out_data, out_size);

    noe_time_stretch_free(stretched_bufs, channels);

    return result;
}

} // extern "C"
