// bc_decode.h: CPU decoder for BC1-BC5 and BC7 blocks, for GPUs without BC texture support (A14 and older).
#ifndef BCD_DECODE_H
#define BCD_DECODE_H
#include <stddef.h>
#include <stdint.h>

typedef enum { BCD_1, BCD_2, BCD_3, BCD_4U, BCD_5U, BCD_7 } bcd_fmt;

// Bytes per 4x4 block of the compressed data.
static inline int bcd_block_bytes(bcd_fmt f) { return f == BCD_1 || f == BCD_4U ? 8 : 16; }
// Bytes per texel of the decoded output: BC4 -> R8, BC5 -> RG8, the rest -> RGBA8.
static inline int bcd_out_bpp(bcd_fmt f) { return f == BCD_4U ? 1 : f == BCD_5U ? 2 : 4; }

// Decodes one block into a 4x4 texel window of dst (row stride in bytes).
void bcd_decode_block(bcd_fmt f, const uint8_t *src, uint8_t *dst, size_t stride);
// Decodes a w x h level (blocks row-major, tightly packed) into dst; edge blocks are clipped.
void bcd_decode_image(bcd_fmt f, const uint8_t *src, uint32_t w, uint32_t h, uint8_t *dst, size_t stride);

#endif
