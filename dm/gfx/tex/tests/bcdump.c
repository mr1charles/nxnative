// bcdump FMT W H < blocks > rgba : decodes a raw BC level for comparison against a reference decoder.
#include "../bc_decode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int c, char **v) {
    bc_fmt f = !strcmp(v[1], "bc1") ? BC_1 : !strcmp(v[1], "bc2") ? BC_2 : !strcmp(v[1], "bc3") ? BC_3 : !strcmp(v[1], "bc4") ? BC_4U : !strcmp(v[1], "bc5") ? BC_5U : BC_7;
    unsigned w = atoi(v[2]), h = atoi(v[3]);
    size_t n = (size_t)((w + 3) / 4) * ((h + 3) / 4) * bc_block_bytes(f), o = (size_t)w * h * bc_out_bpp(f);
    uint8_t *in = malloc(n), *out = malloc(o);
    if (fread(in, 1, n, stdin) != n) return 1;
    bc_decode_image(f, in, w, h, out, (size_t)w * bc_out_bpp(f));
    fwrite(out, 1, o, stdout);
    return 0;
}
