#include "native_ogg_encoder.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <vorbis/vorbisenc.h>
#include <ogg/ogg.h>
#include <speex_resampler.h>

#ifdef _WIN32
    #define THREAD_LOCAL __declspec(thread)
#else
    #define THREAD_LOCAL _Thread_local
#endif

static THREAD_LOCAL char s_error_buf[256];
static THREAD_LOCAL int s_has_error = 0;

static void set_error(const char* msg) {
    s_has_error = 1;
    strncpy(s_error_buf, msg, sizeof(s_error_buf) - 1);
    s_error_buf[sizeof(s_error_buf) - 1] = '\0';
}

void noe_set_error(const char* msg) {
    set_error(msg);
}

void noe_clear_error(void) {
    s_has_error = 0;
}

NOE_EXPORT const char* noe_get_error(void) {
    return s_has_error ? s_error_buf : NULL;
}

NOE_EXPORT void noe_free(uint8_t* data) {
    free(data);
}

typedef struct {
    uint8_t* data;
    int32_t size;
    int32_t capacity;
} Buffer;

static int buffer_init(Buffer* buf, int32_t initial_cap) {
    buf->data = (uint8_t*)malloc(initial_cap);
    if (!buf->data) return -1;
    buf->size = 0;
    buf->capacity = initial_cap;
    return 0;
}

static int buffer_append(Buffer* buf, const uint8_t* data, int32_t len) {
    if (buf->size + len > buf->capacity) {
        int32_t new_cap = buf->capacity * 2;
        while (new_cap < buf->size + len) new_cap *= 2;
        uint8_t* new_data = (uint8_t*)realloc(buf->data, new_cap);
        if (!new_data) return -1;
        buf->data = new_data;
        buf->capacity = new_cap;
    }
    memcpy(buf->data + buf->size, data, len);
    buf->size += len;
    return 0;
}

static int flush_ogg_pages(ogg_stream_state* os, Buffer* buf, int force) {
    ogg_page og;
    int (*page_func)(ogg_stream_state*, ogg_page*) = force ? ogg_stream_flush : ogg_stream_pageout;
    while (page_func(os, &og)) {
        if (buffer_append(buf, og.header, og.header_len) != 0) return -1;
        if (buffer_append(buf, og.body, og.body_len) != 0) return -1;
    }
    return 0;
}

static float* resample_channel_impl(const float* input, int32_t in_frames,
                               int32_t in_rate, int32_t out_rate, int32_t* out_frames) {
    *out_frames = (int32_t)((int64_t)in_frames * out_rate / in_rate);
    float* output = (float*)malloc(*out_frames * sizeof(float));
    if (!output) return NULL;

    int err;
    SpeexResamplerState* resampler = speex_resampler_init(1, in_rate, out_rate, SPEEX_RESAMPLER_QUALITY_DEFAULT, &err);
    if (!resampler || err != RESAMPLER_ERR_SUCCESS) {
        free(output);
        return NULL;
    }

    spx_uint32_t in_len = (spx_uint32_t)in_frames;
    spx_uint32_t out_len = (spx_uint32_t)*out_frames;
    speex_resampler_process_float(resampler, 0, input, &in_len, output, &out_len);
    *out_frames = (int32_t)out_len;

    speex_resampler_destroy(resampler);
    return output;
}

static int32_t encode_core_impl(const float* const* channels, int32_t num_frames,
                           int32_t num_channels, int32_t sample_rate, float quality,
                           uint8_t** out_data, int32_t* out_size) {
    vorbis_info vi;
    vorbis_info_init(&vi);

    if (vorbis_encode_init_vbr(&vi, num_channels, sample_rate, quality) != 0) {
        vorbis_info_clear(&vi);
        set_error("vorbis_encode_init_vbr failed");
        return -3;
    }

    vorbis_comment vc;
    vorbis_comment_init(&vc);

    vorbis_dsp_state vd;
    if (vorbis_analysis_init(&vd, &vi) != 0) {
        vorbis_comment_clear(&vc);
        vorbis_info_clear(&vi);
        set_error("vorbis_analysis_init failed");
        return -3;
    }

    vorbis_block vb;
    vorbis_block_init(&vd, &vb);

    ogg_stream_state os;
    ogg_stream_init(&os, (int)time(NULL));

    ogg_packet header, header_comm, header_code;
    vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
    ogg_stream_packetin(&os, &header);
    ogg_stream_packetin(&os, &header_comm);
    ogg_stream_packetin(&os, &header_code);

    Buffer buf;
    if (buffer_init(&buf, 1024 * 256) != 0) {
        set_error("memory allocation failed");
        goto cleanup_err;
    }

    if (flush_ogg_pages(&os, &buf, 1) != 0) {
        set_error("memory allocation failed");
        goto cleanup_err;
    }

    #define CHUNK_SIZE 4096
    int32_t pos = 0;
    while (pos < num_frames) {
        int32_t chunk = num_frames - pos;
        if (chunk > CHUNK_SIZE) chunk = CHUNK_SIZE;

        float** analysis_buf = vorbis_analysis_buffer(&vd, chunk);
        for (int ch = 0; ch < num_channels; ch++) {
            memcpy(analysis_buf[ch], channels[ch] + pos, chunk * sizeof(float));
        }
        vorbis_analysis_wrote(&vd, chunk);

        ogg_packet op;
        while (vorbis_analysis_blockout(&vd, &vb) == 1) {
            vorbis_analysis(&vb, NULL);
            vorbis_bitrate_addblock(&vb);
            while (vorbis_bitrate_flushpacket(&vd, &op)) {
                ogg_stream_packetin(&os, &op);
                if (flush_ogg_pages(&os, &buf, 0) != 0) {
                    set_error("memory allocation failed");
                    goto cleanup_err;
                }
            }
        }
        pos += chunk;
    }

    vorbis_analysis_wrote(&vd, 0);
    {
        ogg_packet op;
        while (vorbis_analysis_blockout(&vd, &vb) == 1) {
            vorbis_analysis(&vb, NULL);
            vorbis_bitrate_addblock(&vb);
            while (vorbis_bitrate_flushpacket(&vd, &op)) {
                ogg_stream_packetin(&os, &op);
            }
        }
    }
    if (flush_ogg_pages(&os, &buf, 1) != 0) {
        set_error("memory allocation failed");
        goto cleanup_err;
    }

    *out_data = buf.data;
    *out_size = buf.size;

    ogg_stream_clear(&os);
    vorbis_block_clear(&vb);
    vorbis_dsp_clear(&vd);
    vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi);
    s_has_error = 0;
    return 0;

cleanup_err:
    free(buf.data);
    ogg_stream_clear(&os);
    vorbis_block_clear(&vb);
    vorbis_dsp_clear(&vd);
    vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi);
    return -4;
}

NOE_EXPORT int32_t noe_encode_planar(
    const float* const* channel_ptrs,
    int32_t num_frames,
    int32_t channels,
    int32_t input_sample_rate,
    int32_t output_sample_rate,
    float quality,
    uint8_t** out_data,
    int32_t* out_size)
{
    s_has_error = 0;

    if (!channel_ptrs || !out_data || !out_size || channels < 1 || channels > 2 || num_frames <= 0) {
        set_error("invalid parameters");
        return -1;
    }
    if (input_sample_rate <= 0 || output_sample_rate <= 0) {
        set_error("invalid sample rate");
        return -1;
    }

    const float* final_channels[2];
    float* resampled[2] = {NULL, NULL};
    int32_t final_frames = num_frames;

    if (input_sample_rate != output_sample_rate) {
        for (int ch = 0; ch < channels; ch++) {
            int32_t out_frames;
            resampled[ch] = resample_channel_impl(channel_ptrs[ch], num_frames,
                                             input_sample_rate, output_sample_rate, &out_frames);
            if (!resampled[ch]) {
                for (int j = 0; j < ch; j++) free(resampled[j]);
                set_error("resampler failed");
                return -2;
            }
            final_channels[ch] = resampled[ch];
            final_frames = out_frames;
        }
    } else {
        for (int ch = 0; ch < channels; ch++) {
            final_channels[ch] = channel_ptrs[ch];
        }
    }

    int32_t result = encode_core_impl(final_channels, final_frames, channels,
                                 output_sample_rate, quality, out_data, out_size);

    for (int ch = 0; ch < channels; ch++) {
        free(resampled[ch]);
    }

    return result;
}

NOE_EXPORT int32_t noe_encode_interleaved(
    const float* interleaved_samples,
    int32_t num_frames,
    int32_t channels,
    int32_t input_sample_rate,
    int32_t output_sample_rate,
    float quality,
    uint8_t** out_data,
    int32_t* out_size)
{
    s_has_error = 0;

    if (!interleaved_samples || !out_data || !out_size || channels < 1 || channels > 2 || num_frames <= 0) {
        set_error("invalid parameters");
        return -1;
    }

    float* deinterleaved[2] = {NULL, NULL};
    for (int ch = 0; ch < channels; ch++) {
        deinterleaved[ch] = (float*)malloc(num_frames * sizeof(float));
        if (!deinterleaved[ch]) {
            for (int j = 0; j < ch; j++) free(deinterleaved[j]);
            set_error("memory allocation failed");
            return -4;
        }
        for (int32_t i = 0; i < num_frames; i++) {
            deinterleaved[ch][i] = interleaved_samples[i * channels + ch];
        }
    }

    const float* ptrs[2] = {deinterleaved[0], deinterleaved[1]};
    int32_t result = noe_encode_planar(ptrs, num_frames, channels,
                                       input_sample_rate, output_sample_rate,
                                       quality, out_data, out_size);

    for (int ch = 0; ch < channels; ch++) {
        free(deinterleaved[ch]);
    }
    return result;
}

float* resample_channel(const float* input, int32_t in_frames,
                        int32_t in_rate, int32_t out_rate, int32_t* out_frames) {
    return resample_channel_impl(input, in_frames, in_rate, out_rate, out_frames);
}

int32_t encode_core(const float* const* channels, int32_t num_frames,
                    int32_t num_channels, int32_t sample_rate, float quality,
                    uint8_t** out_data, int32_t* out_size) {
    return encode_core_impl(channels, num_frames, num_channels, sample_rate, quality, out_data, out_size);
}
