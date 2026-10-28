// bc6h_decode.c: BC6H (HDR) blocks through the third-party bcdec library (MIT / Unlicense, dm/third_party/bcdec), expanded to RGBA16F with alpha 1.
#define BCDEC_IMPLEMENTATION
#include "../../third_party/bcdec/bcdec.h"
#include "bc_decode.h"

void bcd_bc6h_block(const uint8_t *src, uint8_t *dst, size_t stride, int is_signed) {
    uint16_t rgb[4 * 4 * 3];
    bcdec_bc6h_half(src, rgb, 4 * 3, is_signed);
    for (int i = 0; i < 16; i++) {
        uint16_t *d = (uint16_t *)(dst + (i >> 2) * stride + (i & 3) * 8);
        d[0] = rgb[i * 3]; d[1] = rgb[i * 3 + 1]; d[2] = rgb[i * 3 + 2]; d[3] = 0x3C00;   // 1.0
    }
}
