/*
 * engine/ui/assets.c -- the kit's font and archive access.
 *
 * The font
 * --------
 *
 * The game's three text fonts are NFTR files in archive a/0/2/3: member 0
 * is 12x15 (labels and names), member 1 is 10x10 (small labels and hints),
 * member 2 is 11x13 (descriptions).  The container is the usual chunked
 * NITRO shape; the file header is "RTFN" on disk, then the chunks in this
 * order:
 *
 *   FNIF  the font info: the line height at +1 of the body, and at +8, +12
 *         and +16 the offsets (from the start of the member) of the CGLP,
 *         CWDH and first CMAP bodies.
 *   PLGC  (CGLP) the glyphs: cell width and height at +0 and +1, the size
 *         of one glyph record at +2 and the bits per pixel at +6; the
 *         records start at +8.  A record is three metric bytes (left
 *         bearing as a signed byte, drawn width, advance) and then the
 *         bitmap: cell_w * cell_h values of `bpp` bits, MSB first, in one
 *         continuous bit stream.  The game's fonts use 2 bits: value 1 is
 *         the letter, value 2 its shadow.
 *   HDWC  (CWDH) per glyph widths; not read: the metric bytes in each
 *         glyph record are the ones the game's own renderer uses.
 *   PAMC  (CMAP) a chain of character maps (each holds the offset of the
 *         next): method 0 maps first..last to consecutive glyphs, method 1
 *         has a table of u16 per character (0xFFFF none), method 2 a list
 *         of (character, glyph) pairs.  The characters are Unicode.
 *
 * The kit flattens the chain into one table of u16 per code point below
 * 0x10000 when the font is loaded, so a lookup while drawing is one read.
 * An earlier chunk in the chain wins over a later one, as in the game.
 *
 * Reading archives
 * ----------------
 *
 * ui_arc_member() keeps a few archives open (the port's NARC reader, which
 * goes through the mod layer) and reads one member into a new block.  A
 * font loaded from memory (ui_font_load_nftr) is used instead of the game's.
 */

#include <stdlib.h>
#include <string.h>

#include "assets.h"
#include "port.h"

/* ------------------------------------------------------------------ */
/* Fonts                                                               */
/* ------------------------------------------------------------------ */

typedef struct UiFontData {
    u8  *data;              /* the whole NFTR member                       */
    u32  size;
    u16 *map;               /* code point -> glyph, 0xFFFF none            */
    int  cell_w, cell_h, cell_size, bpp;
    u32  glyph_count, glyph_off;
    int  tried;             /* a load was attempted                        */
    int  ok;
    u8   ugrave[64];        /* the small font's u-grave, made (small_fix)  */
    int  has_ugrave;
    u8   polish[18][64];    /* missing small-font Polish letters */
    u32  polish_mask;
} UiFontData;

static UiFontData g_font[3];

static const u16 k_polish[] = {
    0x104, 0x105, 0x106, 0x107, 0x118, 0x119, 0x141, 0x142, 0x143,
    0x144, 0xD3, 0xF3, 0x15A, 0x15B, 0x179, 0x17A, 0x17B, 0x17C
};

static u32 rd16(const u8 *p) { return (u32)p[0] | ((u32)p[1] << 8); }
static u32 rd32(const u8 *p) { return rd16(p) | (rd16(p + 2) << 16); }

static void font_free(UiFontData *f)
{
    free(f->data);
    free(f->map);
    memset(f, 0, sizeof *f);
}

/* Parse `data` into f (f->data and f->size are the caller's to set). */
static int font_parse(UiFontData *f, const u8 *data, u32 size)
{
    u32 finf, cglp, cmap, glen, off, guard;
    u16 *map;

    if (size < 0x30 || memcmp(data, "RTFN", 4) != 0) {
        return -1;
    }
    finf = rd16(data + 0x0C);
    if (finf + 0x1C > size || memcmp(data + finf, "FNIF", 4) != 0) {
        return -1;
    }
    cglp = rd32(data + finf + 8 + 8);
    cmap = rd32(data + finf + 8 + 16);
    if (cglp < 8 || cglp + 8 > size || memcmp(data + cglp - 8, "PLGC", 4) != 0) {
        return -1;
    }
    f->cell_w = data[cglp];
    f->cell_h = data[cglp + 1];
    f->cell_size = (int)rd16(data + cglp + 2);
    f->bpp = (int)rd16(data + cglp + 6);
    if (f->cell_w == 0 || f->cell_h == 0 || f->bpp != 2 ||
        f->cell_size != 3 + (f->cell_w * f->cell_h * f->bpp + 7) / 8) {
        return -1;
    }
    glen = rd32(data + cglp - 4) - 8 - 8;
    f->glyph_off = cglp + 8;
    if (f->glyph_off + glen > size) {
        return -1;
    }
    f->glyph_count = glen / (u32)f->cell_size;

    map = (u16 *)malloc(0x10000u * sizeof(u16));
    if (map == NULL) {
        return -1;
    }
    memset(map, 0xFF, 0x10000u * sizeof(u16));
    off = cmap;
    for (guard = 0; off != 0 && guard < 64; guard++) {
        const u8 *b;
        u32 first, last, method, next, c;

        if (off < 8 || off + 12 > size || memcmp(data + off - 8, "PAMC", 4) != 0) {
            break;
        }
        b = data + off;
        first = rd16(b);
        last = rd16(b + 2);
        method = rd16(b + 4);
        next = rd32(b + 8);
        if (method == 0 && off + 14 <= size) {
            u32 base = rd16(b + 12);
            for (c = first; c <= last; c++) {
                if (map[c] == 0xFFFF) {
                    map[c] = (u16)(base + (c - first));
                }
            }
        } else if (method == 1 && last >= first &&
                   off + 12 + (last - first + 1) * 2 <= size) {
            for (c = first; c <= last; c++) {
                u32 g = rd16(b + 12 + (c - first) * 2);
                if (g != 0xFFFF && map[c] == 0xFFFF) {
                    map[c] = (u16)g;
                }
            }
        } else if (method == 2 && off + 14 <= size) {
            u32 n = rd16(b + 12), i;
            if (off + 14 + n * 4 <= size) {
                for (i = 0; i < n; i++) {
                    u32 ch = rd16(b + 14 + i * 4), g = rd16(b + 16 + i * 4);
                    if (map[ch] == 0xFFFF) {
                        map[ch] = (u16)g;
                    }
                }
            }
        }
        off = next;
    }
    f->map = map;
    return 0;
}

/*
 * The small font (member 1) holds Greek and Cyrillic letters in some of
 * the accented Latin slots the games never print with it: O-umlaut is a
 * psi, I-circumflex an omicron, C-cedilla a theta, u-grave a zhe.  Those
 * codes take the font's own nearest letter (its capitals with an accent
 * are drawn small, as its A-umlaut and U-umlaut are, so the lower case
 * letter with the same accent stands in), and u-grave is its u-acute with
 * the accent turned round.
 */
static void small_fix(UiFontData *f)
{
    static const u16 sub[][2] = {
        { 0xC2, 0xE2 }, { 0xC3, 0xE3 }, { 0xC5, 'A' }, { 0xC6, 'A' }, { 0xC7, 0xE7 },
        { 0xCB, 0xEB }, { 0xCE, 0xEE }, { 0xCF, 0xEF }, { 0xD4, 0xF4 }, { 0xD5, 'O' },
        { 0xD6, 0xF6 }, { 0xD8, 'O' }, { 0xDB, 0xFB }, { 0xDD, 'Y' }, { 0xDE, 'P' },
        { 0xE5, 'a' }, { 0xE6, 'a' }, { 0xF0, 'd' }, { 0xF5, 'o' }, { 0xF8, 'o' },
        { 0xFD, 'y' }, { 0xFE, 'p' }, { 0xFF, 'y' },
    };
    u32 i, g = f->map[0xFA];

    for (i = 0; i < sizeof sub / sizeof sub[0]; i++) {
        if (f->map[sub[i][1]] != 0xFFFF) {
            f->map[sub[i][0]] = f->map[sub[i][1]];
        }
    }
    /* u-grave: u-acute's accent rows (above the letter) mirrored */
    if (g != 0xFFFF && g < f->glyph_count && (u32)f->cell_size <= sizeof f->ugrave) {
        const u8 *rec = f->data + f->glyph_off + g * (u32)f->cell_size;
        int adv = rec[2], x, y, w = adv < f->cell_w ? adv : f->cell_w;
        u8 *bm = f->ugrave + 3;

        memcpy(f->ugrave, rec, (size_t)f->cell_size);
        for (y = 0; y < 3 && y < f->cell_h; y++) {
            int px[32];

            for (x = 0; x < f->cell_w && x < 32; x++) {
                int i2 = y * f->cell_w + x;

                px[x] = (rec[3 + (i2 * 2 >> 3)] >> (6 - ((i2 & 3) * 2))) & 3;
            }
            for (x = 0; x < f->cell_w && x < 32; x++) {
                int i2 = y * f->cell_w + x, v = x < w - 1 ? px[w - 2 - x] : px[x];

                bm[i2 * 2 >> 3] = (u8)((bm[i2 * 2 >> 3] & ~(3 << (6 - ((i2 & 3) * 2)))) | (v << (6 - ((i2 & 3) * 2))));
            }
        }
        f->has_ugrave = 1;
    }
}

/* Translation patches need not supply the small font used by long UI
 * descriptions. Build only its missing Polish letters from its own Latin
 * glyphs, keeping their advance and baseline. Native patched glyphs win.
 * Two bottom rows hold ogoneks; two top rows hold acute/dot accents. */
static void small_polish(UiFontData *f)
{
    static const char base[] = "AaCcEeLlNnOoSsZzZz";
    int i;

    f->polish_mask = 0;
    if (f->cell_w != 10 || f->cell_h != 10 || f->cell_size > 64) return;
    for (i = 0; i < 18; i++) {
        u32 g = f->map[(u8)base[i]], old = f->map[k_polish[i]];
        const u8 *src;
        u8 *dst = f->polish[i];
        int ink[10][10] = {{0}}, x, y, left = 10, right = 0, top = 10, bottom = 0;
        int ogonek = i < 2 || (i >= 4 && i < 6);
        int slash = i == 6 || i == 7;
        int dot = i >= 16;

        if ((old != 0xFFFFu && old < f->glyph_count) ||
            g == 0xFFFFu || g >= f->glyph_count) continue;
        src = f->data + f->glyph_off + g * (u32)f->cell_size;
        for (y = 0; y < 10; y++) for (x = 0; x < 10; x++) {
            int p = y * 10 + x;
            if (((src[3 + p / 4] >> (6 - (p % 4) * 2)) & 3) == 1) {
                ink[y][x] = 1;
                if (x < left) left = x;
                if (x > right) right = x;
                if (y < top) top = y;
                if (y > bottom) bottom = y;
            }
        }
        if (left > right) continue;
        if (!ogonek && !slash) {
            /* Capitals reach row zero: compress their body into rows 3..7
             * before adding the accent. Lowercase already has headroom. */
            if (top < 3) {
                int body[10][10];
                memcpy(body, ink, sizeof body);
                memset(ink, 0, sizeof ink);
                for (y = 3; y <= bottom; y++) for (x = 0; x < 10; x++)
                    ink[y][x] = body[top + (y - 3) * (bottom - top + 1) / (bottom - 2)][x];
            }
            x = (left + right) / 2;
            ink[0][x + (dot ? 0 : 1)] = 1;
            ink[1][x] = 1;
        } else if (ogonek) {
            x = right > 0 ? right - 1 : right;
            ink[8][x] = 1;
            ink[9][x] = 1;
            if (x < 9) ink[9][x + 1] = 1;
        } else {
            y = (top + bottom) / 2;
            for (x = left > 0 ? left - 1 : 0; x <= right + 1 && x < 9; x++)
                ink[y + (x < (left + right) / 2 ? 1 : 0)][x] = 1;
        }
        memcpy(dst, src, 3);
        memset(dst + 3, 0, (size_t)f->cell_size - 3);
        /* Rebuild the same one-pixel shadow without painting over ink. */
        for (y = 0; y < 10; y++) for (x = 0; x < 10; x++) {
            int p = y * 10 + x;
            int v = ink[y][x] ? 1 : (x > 0 && y > 0 && ink[y - 1][x - 1] ? 2 : 0);
            dst[3 + p / 4] |= (u8)(v << (6 - (p % 4) * 2));
        }
        f->polish_mask |= 1u << i;
    }
}

int ui_font_load_nftr(int font, const u8 *data, u32 size)
{
    UiFontData *f;
    UiFontData tmp;
    u8 *copy;

    if (font < 0 || font > 2 || data == NULL) {
        return -1;
    }
    copy = (u8 *)malloc(size);
    if (copy == NULL) {
        return -1;
    }
    memcpy(copy, data, size);
    memset(&tmp, 0, sizeof tmp);
    if (font_parse(&tmp, copy, size) != 0) {
        free(copy);
        return -1;
    }
    f = &g_font[font];
    font_free(f);
    *f = tmp;
    f->data = copy;
    f->size = size;
    f->tried = 1;
    f->ok = 1;
    if (font == 1) {
        small_fix(f);
        small_polish(f);
    }
    return 0;
}

static void font_ensure(int font)
{
    UiFontData *f = &g_font[font];
    u32 size = 0;
    u8 *m;

    if (f->tried) {
        return;
    }
    f->tried = 1;
    m = ui_arc_member(UI_ARC_FONT, (u32)font, &size);
    if (m == NULL) {
        return;
    }
    if (font_parse(f, m, size) == 0) {
        f->data = m;
        f->size = size;
        f->ok = 1;
        if (font == 1) {
            small_fix(f);
            small_polish(f);
        }
    } else {
        free(m);
    }
}

int ui_font_glyph(int font, u32 code, UiGlyph *out)
{
    UiFontData *f;
    u32 g;
    const u8 *rec;

    if (font < 0 || font > 2) {
        return 0;
    }
    font_ensure(font);
    f = &g_font[font];
    if (!f->ok) {
        return 0;
    }
    g = code < 0x10000u ? f->map[code] : 0xFFFFu;
    if (g == 0xFFFFu || g >= f->glyph_count) {
        g = f->map['?'];
        if (g == 0xFFFFu || g >= f->glyph_count) {
            return 0;
        }
    }
    rec = f->data + f->glyph_off + g * (u32)f->cell_size;
    if (code == 0xF9u && f->has_ugrave) {
        rec = f->ugrave;
    }
    if (font == 1 && f->polish_mask != 0) {
        int i;
        for (i = 0; i < 18; i++) {
            if (code == k_polish[i] && (f->polish_mask & (1u << i))) {
                rec = f->polish[i];
                break;
            }
        }
    }
    out->bearing = (int)(signed char)rec[0];
    out->adv = rec[2];
    out->cell_w = f->cell_w;
    out->cell_h = f->cell_h;
    out->bitmap = rec + 3;
    return 1;
}

int ui_font_fallback_adv(int font)
{
    return font == 0 ? 8 : 6;
}

/* ------------------------------------------------------------------ */
/* Archives                                                            */
/* ------------------------------------------------------------------ */

static const char *const k_arc_path[UI_ARC_COUNT] = {
    "a/0/2/3",              /* UI_ARC_FONT     */
    "a/0/2/5",              /* UI_ARC_ITEM     */
    "a/0/0/7",              /* UI_ARC_MON      */
    "a/0/1/6",              /* UI_ARC_PERSONAL */
    "a/0/3/1",              /* UI_ARC_WALK1    */
    "a/0/3/2",              /* UI_ARC_WALK2    */
    "a/0/4/0",              /* UI_ARC_CARD     */
};

static BwArcFile g_arc[UI_ARC_COUNT];
static int       g_arc_state[UI_ARC_COUNT];     /* 0 not tried, 1 open, -1 failed */

u8 *ui_arc_member(int slot, u32 member, u32 *size)
{
    long n;
    u8 *buf;

    if (slot < 0 || slot >= UI_ARC_COUNT) {
        return NULL;
    }
    if (g_arc_state[slot] == 0) {
        g_arc_state[slot] = bw_arc_open(&g_arc[slot], k_arc_path[slot]) == 0 ? 1 : -1;
    }
    if (g_arc_state[slot] != 1 || member >= g_arc[slot].count) {
        return NULL;
    }
    n = bw_arc_member_size(&g_arc[slot], member);
    if (n <= 0) {
        return NULL;
    }
    buf = (u8 *)malloc((size_t)n);
    if (buf == NULL) {
        return NULL;
    }
    if (bw_arc_read_member(&g_arc[slot], member, buf, (u32)n) != n) {
        free(buf);
        return NULL;
    }
    if (size != NULL) {
        *size = (u32)n;
    }
    return buf;
}

void ui_assets_reset(void)
{
    int i;

    for (i = 0; i < 3; i++) {
        font_free(&g_font[i]);
    }
    for (i = 0; i < UI_ARC_COUNT; i++) {
        if (g_arc_state[i] == 1) {
            bw_arc_close(&g_arc[i]);
        }
        g_arc_state[i] = 0;
    }
    ui_icons_reset();
    ui_sprites_reset();
}

/* ------------------------------------------------------------------ */
/* Images                                                              */
/* ------------------------------------------------------------------ */

u8 *ui_decode_4bpp(const u8 *tiles, int tiles_w, int tiles_h,
                   const u16 *pal16, int crop_w, int crop_h)
{
    u8 *rgba;
    int x, y;

    (void)tiles_h;
    rgba = (u8 *)calloc((size_t)crop_w * (size_t)crop_h, 4);
    if (rgba == NULL) {
        return NULL;
    }
    for (y = 0; y < crop_h; y++) {
        for (x = 0; x < crop_w; x++) {
            u32 t, b;
            int v;
            u8 *d;

            t = (u32)((y >> 3) * tiles_w + (x >> 3));
            b = tiles[t * 32 + (u32)(((y & 7) * 4) + ((x & 7) >> 1))];
            v = (x & 1) ? (int)(b >> 4) : (int)(b & 15);
            if (v == 0) {
                continue;
            }
            d = rgba + ((size_t)y * (size_t)crop_w + (size_t)x) * 4;
            d[0] = (u8)(((pal16[v] & 31) * 255) / 31);
            d[1] = (u8)((((pal16[v] >> 5) & 31) * 255) / 31);
            d[2] = (u8)((((pal16[v] >> 10) & 31) * 255) / 31);
            d[3] = 255;
        }
    }
    return rgba;
}
