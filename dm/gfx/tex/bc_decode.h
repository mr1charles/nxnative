// bc_decode.h: CPU decoder for BC1-BC5 and BC7 blocks, for GPUs without BC texture support (A14 and older).
#ifndef BC_DECODE_H
#define BC_DECODE_H
#include <stddef.h>
#include <stdint.h>

typedef enum { BC_1, BC_2, BC_3, BC_4U, BC_5U, BC_7 } bc_fmt;

// Bytes per 4x4 block of the compressed data.
static inline int bc_block_bytes(bc_fmt f) { return f == BC_1 || f == BC_4U ? 8 : 16; }
// Bytes per texel of the decoded output: BC4 -> R8, BC5 -> RG8, the rest -> RGBA8.
static inline int bc_out_bpp(bc_fmt f) { return f == BC_4U ? 1 : f == BC_5U ? 2 : 4; }

// Decodes one block into a 4x4 texel window of dst (row stride in bytes).
void bc_decode_block(bc_fmt f, const uint8_t *src, uint8_t *dst, size_t stride);
// Decodes a w x h level (blocks row-major, tightly packed) into dst; edge blocks are clipped.
void bc_decode_image(bc_fmt f, const uint8_t *src, uint32_t w, uint32_t h, uint8_t *dst, size_t stride);

#endif
