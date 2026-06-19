/* Minimal arch.h for speex resampler in FLOATING_POINT + OUTSIDE_SPEEX mode */

#ifndef ARCH_H
#define ARCH_H

#ifndef FLOATING_POINT
#error "This build requires FLOATING_POINT to be defined"
#endif

typedef float spx_mem_t;
typedef float spx_coef_t;
typedef float spx_lsp_t;
typedef float spx_sig_t;
typedef float spx_word16_t;
typedef float spx_word32_t;

#define Q15ONE 1.0f
#define LPC_SCALING  1.f
#define SIG_SCALING  1.f
#define LSP_SCALING  1.f
#define GAMMA_SCALING 1.f
#define GAIN_SCALING 1.f
#define GAIN_SCALING_1 1.f

#define VERY_SMALL 1e-15f
#define VERY_LARGE32 1e15f
#define VERY_LARGE16 1e15f
#define Q15_ONE ((spx_word16_t)1.f)

#define QCONST16(x,bits) (x)
#define QCONST32(x,bits) (x)

#define NEG16(x) (-(x))
#define NEG32(x) (-(x))
#define EXTRACT16(x) (x)
#define EXTEND32(x) (x)
#define SHR16(a,shift) (a)
#define SHL16(a,shift) (a)
#define SHR32(a,shift) (a)
#define SHL32(a,shift) (a)
#define PSHR16(a,shift) (a)
#define PSHR32(a,shift) (a)
#define VSHR32(a,shift) (a)
#define SATURATE16(x,a) (x)
#define SATURATE32(x,a) (x)
#define SATURATE32PSHR(x,shift,a) (x)

#define PSHR(a,shift)       (a)
#define SHR(a,shift)       (a)
#define SHL(a,shift)       (a)
#define SATURATE(x,a) (x)

#define ADD16(a,b) ((a)+(b))
#define SUB16(a,b) ((a)-(b))
#define ADD32(a,b) ((a)+(b))
#define SUB32(a,b) ((a)-(b))
#define MULT16_16_16(a,b)     ((a)*(b))
#define MULT16_32_32(a,b)     ((a)*(b))
#define MULT16_16(a,b)     ((spx_word32_t)(a)*(spx_word32_t)(b))
#define MAC16_16(c,a,b)     ((c)+(spx_word32_t)(a)*(spx_word32_t)(b))

#define MULT16_32_Q15(a,b)     ((a)*(b))
#define MULT16_32_P15(a,b)     ((a)*(b))

#define MAC16_32_Q15(c,a,b)     ((c)+(a)*(b))

#define MAC16_16_Q11(c,a,b)     ((c)+(a)*(b))
#define MAC16_16_Q13(c,a,b)     ((c)+(a)*(b))
#define MAC16_16_P13(c,a,b)     ((c)+(a)*(b))
#define MULT16_16_Q11_32(a,b)     ((a)*(b))
#define MULT16_16_Q13(a,b)     ((a)*(b))
#define MULT16_16_Q14(a,b)     ((a)*(b))
#define MULT16_16_Q15(a,b)     ((a)*(b))
#define MULT16_16_P15(a,b)     ((a)*(b))
#define MULT16_16_P13(a,b)     ((a)*(b))
#define MULT16_16_P14(a,b)     ((a)*(b))

#define DIV32_16(a,b)     (((spx_word32_t)(a))/(spx_word16_t)(b))
#define PDIV32_16(a,b)     (((spx_word32_t)(a))/(spx_word16_t)(b))
#define DIV32(a,b)     (((spx_word32_t)(a))/(spx_word32_t)(b))
#define PDIV32(a,b)     (((spx_word32_t)(a))/(spx_word32_t)(b))

#include <math.h>
#define WORD2INT(x) ((x) < -32767.5f ? -32768 : \
                    ((x) > 32766.5f ? 32767 : (spx_int16_t)floor(.5 + (x))))

#define BYTES_PER_CHAR 1
#define BITS_PER_CHAR 8
#define LOG2_BITS_PER_CHAR 3

#endif
