// bcdump FMT W H < blocks > rgba : decodes a raw BC level for comparison against a reference decoder.
#include "../bc_decode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int c, char **v) {
    bcd_fmt f = !strcmp(v[1], "bc1") ? BCD_1 : !strcmp(v[1], "bc2") ? BCD_2 : !strcmp(v[1], "bc3") ? BCD_3 : !strcmp(v[1], "bc4") ? BCD_4U : !strcmp(v[1], "bc5") ? BCD_5U : !strcmp(v[1], "bc5s") ? BCD_5S : !strcmp(v[1], "bc4s") ? BCD_4S : !strcmp(v[1], "bc6h") ? BCD_6U : !strcmp(v[1], "bc6hs") ? BCD_6S : BCD_7;
    unsigned w = atoi(v[2]), h = atoi(v[3]);
    size_t n = (size_t)((w + 3) / 4) * ((h + 3) / 4) * bcd_block_bytes(f), o = (size_t)w * h * bcd_out_bpp(f);
    uint8_t *in = malloc(n), *out = malloc(o);
    if (fread(in, 1, n, stdin) != n) return 1;
    bcd_decode_image(f, in, w, h, out, (size_t)w * bcd_out_bpp(f));
    fwrite(out, 1, o, stdout);
    return 0;
}
