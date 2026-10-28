// bc_decode.h: CPU decoder for BC1-BC5 and BC7 blocks, for GPUs without BC texture support (A14 and older).
#ifndef BCD_DECODE_H
#define BCD_DECODE_H
#include <stddef.h>
#include <stdint.h>

typedef enum { BCD_1, BCD_2, BCD_3, BCD_4U, BCD_5U, BCD_7, BCD_4S, BCD_5S, BCD_6U, BCD_6S } bcd_fmt;   // 4S / 5S output signed bytes (R8Snorm / RG8Snorm); 6U / 6S (BC6H) output RGBA16F

// Bytes per 4x4 block of the compressed data.
static inline int bcd_block_bytes(bcd_fmt f) { return f == BCD_1 || f == BCD_4U || f == BCD_4S ? 8 : 16; }
// Bytes per texel of the decoded output: BC4 -> R8, BC5 -> RG8, BC6H -> RGBA16F, the rest -> RGBA8.
static inline int bcd_out_bpp(bcd_fmt f) { return f == BCD_4U || f == BCD_4S ? 1 : f == BCD_5U || f == BCD_5S ? 2 : f == BCD_6U || f == BCD_6S ? 8 : 4; }

void bcd_bc6h_block(const uint8_t *src, uint8_t *dst, size_t stride, int is_signed);   // bc6h_decode.c
// Decodes one block into a 4x4 texel window of dst (row stride in bytes).
void bcd_decode_block(bcd_fmt f, const uint8_t *src, uint8_t *dst, size_t stride);
// Decodes a w x h level (blocks row-major, tightly packed) into dst; edge blocks are clipped.
void bcd_decode_image(bcd_fmt f, const uint8_t *src, uint32_t w, uint32_t h, uint8_t *dst, size_t stride);

#endif
