// bc_decode.c: see bc_decode.h. Block layouts follow the public BC1-BC5 and BPTC (BC7) definitions.
#include "bc_decode.h"
#include <string.h>

#include "bc7_tables.inc"

static inline uint32_t rd16(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8; }

static void put_rgba(uint8_t *d, int r, int g, int b, int a) { d[0] = (uint8_t)r; d[1] = (uint8_t)g; d[2] = (uint8_t)b; d[3] = (uint8_t)a; }

// BC1 colour block. four=1 forces the 4-colour mode (BC2/BC3 colour halves).
static void bc1_color(const uint8_t *s, uint8_t *dst, size_t stride, int four, int with_alpha) {
    uint32_t c0 = rd16(s), c1 = rd16(s + 2), bits = s[4] | s[5] << 8 | s[6] << 16 | (uint32_t)s[7] << 24;
    int p[4][4];
    uint32_t e[2] = {c0, c1};
    for (int i = 0; i < 2; i++) {
        int r = e[i] >> 11, g = (e[i] >> 5) & 63, b = e[i] & 31;
        p[i][0] = r << 3 | r >> 2; p[i][1] = g << 2 | g >> 4; p[i][2] = b << 3 | b >> 2; p[i][3] = 255;
    }
    if (four || c0 > c1) {
        for (int k = 0; k < 3; k++) { p[2][k] = (2 * p[0][k] + p[1][k]) / 3; p[3][k] = (p[0][k] + 2 * p[1][k]) / 3; }
        p[2][3] = p[3][3] = 255;
    } else {
        for (int k = 0; k < 3; k++) { p[2][k] = (p[0][k] + p[1][k]) / 2; p[3][k] = 0; }
        p[2][3] = 255; p[3][3] = 0;
    }
    for (int i = 0; i < 16; i++) {
        int idx = (bits >> (2 * i)) & 3; uint8_t *d = dst + (i >> 2) * stride + (i & 3) * 4;
        d[0] = (uint8_t)p[idx][0]; d[1] = (uint8_t)p[idx][1]; d[2] = (uint8_t)p[idx][2];
        if (with_alpha) d[3] = (uint8_t)p[idx][3];
    }
}

// BC4 single channel block into a strided plane (step = bytes between texels).
static void bc4_plane(const uint8_t *s, uint8_t *dst, size_t stride, size_t step) {
    int a0 = s[0], a1 = s[1], v[8];
    v[0] = a0; v[1] = a1;
    if (a0 > a1) for (int i = 1; i < 7; i++) v[i + 1] = ((7 - i) * a0 + i * a1) / 7;
    else { for (int i = 1; i < 5; i++) v[i + 1] = ((5 - i) * a0 + i * a1) / 5; v[6] = 0; v[7] = 255; }
    uint64_t bits = 0;
    for (int i = 0; i < 6; i++) bits |= (uint64_t)s[2 + i] << (8 * i);
    for (int i = 0; i < 16; i++) dst[(i >> 2) * stride + (i & 3) * step] = (uint8_t)v[(bits >> (3 * i)) & 7];
}

// BC7
static const uint8_t bc7_mode[8][9] = {  // ns, pb, rb, isb, cb, ab, epb, spb, ib
    {3, 4, 0, 0, 4, 0, 1, 0, 3}, {2, 6, 0, 0, 6, 0, 0, 1, 3}, {3, 6, 0, 0, 5, 0, 0, 0, 2}, {2, 6, 0, 0, 7, 0, 1, 0, 2},
    {1, 0, 2, 1, 5, 6, 0, 0, 2}, {1, 0, 2, 0, 7, 8, 0, 0, 2}, {1, 0, 0, 0, 7, 7, 1, 0, 4}, {2, 6, 0, 0, 5, 5, 1, 0, 2}};
static const uint8_t bc7_w2[4] = {0, 21, 43, 64}, bc7_w3[8] = {0, 9, 18, 27, 37, 46, 55, 64},
                     bc7_w4[16] = {0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64};

typedef struct { const uint8_t *p; int pos; } bitrd;
static uint32_t bits(bitrd *b, int n) {
    uint32_t v = 0;
    for (int i = 0; i < n; i++, b->pos++) v |= (uint32_t)((b->p[b->pos >> 3] >> (b->pos & 7)) & 1) << i;
    return v;
}
static int bc7_weight(int ib, int i) { return ib == 2 ? bc7_w2[i] : ib == 3 ? bc7_w3[i] : bc7_w4[i]; }
static int lerp(int a, int b, int w) { return (a * (64 - w) + b * w + 32) >> 6; }

static void bc7_block(const uint8_t *s, uint8_t *dst, size_t stride) {
    int mode = 0;
    while (mode < 8 && !((s[0] >> mode) & 1)) mode++;
    if (mode == 8) { for (int y = 0; y < 4; y++) memset(dst + y * stride, 0, 16); return; }  // reserved: transparent black
    const uint8_t *m = bc7_mode[mode];
    int ns = m[0], pb = m[1], rb = m[2], isb = m[3], cb = m[4], ab = m[5], epb = m[6], spb = m[7], ib = m[8];
    bitrd b = {s, mode + 1};
    int part = (int)bits(&b, pb), rot = (int)bits(&b, rb), idxmode = (int)bits(&b, isb);
    int ep[6][2][4];  // subset, endpoint, rgba
    for (int c = 0; c < 3; c++) for (int i = 0; i < ns * 2; i++) ep[i >> 1][i & 1][c] = (int)bits(&b, cb);
    for (int i = 0; i < ns * 2; i++) ep[i >> 1][i & 1][3] = ab ? (int)bits(&b, ab) : 255;
    int pbit[6][2] = {{0}};
    if (epb) for (int i = 0; i < ns * 2; i++) pbit[i >> 1][i & 1] = (int)bits(&b, 1);
    else if (spb) for (int i = 0; i < ns; i++) pbit[i][0] = pbit[i][1] = (int)bits(&b, 1);
    int has_p = epb || spb;
    for (int i = 0; i < ns * 2; i++) for (int c = 0; c < 4; c++) {
        int nb = c < 3 ? cb : ab; if (!nb) continue;
        int v = ep[i >> 1][i & 1][c];
        if (has_p) { v = v << 1 | pbit[i >> 1][i & 1]; nb++; }
        ep[i >> 1][i & 1][c] = v << (8 - nb) | v >> (2 * nb - 8);
    }
    uint32_t pm = ns == 1 ? 0 : ns == 2 ? bc7_part2[part] : bc7_part3[part];
    int anc[3] = {0, ns == 2 ? bc7_anchor2[part] : bc7_anchor3a[part], bc7_anchor3b[part]};
    int sub[16], isanchor[16];
    for (int i = 0; i < 16; i++) { sub[i] = (pm >> (2 * i)) & 3; isanchor[i] = i == anc[sub[i]]; }
    int ib2 = mode == 4 ? 3 : mode == 5 ? 2 : 0, idx[16], idx2[16];
    for (int i = 0; i < 16; i++) idx[i] = (int)bits(&b, isanchor[i] ? ib - 1 : ib);
    if (ib2) for (int i = 0; i < 16; i++) idx2[i] = (int)bits(&b, i == 0 ? ib2 - 1 : ib2);
    for (int i = 0; i < 16; i++) {
        int *e0 = ep[sub[i]][0], *e1 = ep[sub[i]][1], px[4];
        int cbits = ib, abits = ib, ci = idx[i], ai = idx[i];
        if (ib2) {
            if (mode == 4 && idxmode) { cbits = ib2; ci = idx2[i]; abits = ib; ai = idx[i]; }
            else { abits = ib2; ai = idx2[i]; }
        }
        int wc = bc7_weight(cbits, ci), wa = bc7_weight(abits, ai);
        for (int c = 0; c < 3; c++) px[c] = lerp(e0[c], e1[c], wc);
        px[3] = lerp(e0[3], e1[3], wa);
        if (rot) { int t = px[3]; px[3] = px[rot - 1]; px[rot - 1] = t; }
        put_rgba(dst + (i >> 2) * stride + (i & 3) * 4, px[0], px[1], px[2], px[3]);
    }
}

void bc_decode_block(bc_fmt f, const uint8_t *s, uint8_t *dst, size_t stride) {
    switch (f) {
    case BC_1: bc1_color(s, dst, stride, 0, 1); break;
    case BC_2:
        bc1_color(s + 8, dst, stride, 1, 0);
        for (int i = 0; i < 16; i++) { int a = (s[i >> 1] >> ((i & 1) * 4)) & 15; dst[(i >> 2) * stride + (i & 3) * 4 + 3] = (uint8_t)(a << 4 | a); }
        break;
    case BC_3: bc1_color(s + 8, dst, stride, 1, 0); bc4_plane(s, dst + 3, stride, 4); break;
    case BC_4U: bc4_plane(s, dst, stride, 1); break;
    case BC_5U: bc4_plane(s, dst, stride, 2); bc4_plane(s + 8, dst + 1, stride, 2); break;
    case BC_7: bc7_block(s, dst, stride); break;
    }
}

void bc_decode_image(bc_fmt f, const uint8_t *src, uint32_t w, uint32_t h, uint8_t *dst, size_t stride) {
    int bb = bc_block_bytes(f), bpp = bc_out_bpp(f);
    uint32_t bw = (w + 3) / 4, bh = (h + 3) / 4;
    uint8_t tmp[4 * 16];
    for (uint32_t by = 0; by < bh; by++) for (uint32_t bx = 0; bx < bw; bx++) {
        const uint8_t *s = src + ((size_t)by * bw + bx) * bb;
        uint32_t cw = w - bx * 4 < 4 ? w - bx * 4 : 4, ch = h - by * 4 < 4 ? h - by * 4 : 4;
        if (cw == 4 && ch == 4) { bc_decode_block(f, s, dst + (size_t)by * 4 * stride + (size_t)bx * 4 * bpp, stride); continue; }
        bc_decode_block(f, s, tmp, 4 * bpp);
        for (uint32_t y = 0; y < ch; y++) memcpy(dst + ((size_t)by * 4 + y) * stride + (size_t)bx * 4 * bpp, tmp + y * 4 * bpp, (size_t)cw * bpp);
    }
}
