#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_ONLY_PNG
#include "stb_image.h"

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "hd_textures.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#include <direct.h>
#define MKDIR_ONE(p) _mkdir(p)
#else
#include <sys/stat.h>
#include <unistd.h>
#define MKDIR_ONE(p) mkdir((p), 0755)
#endif

static char g_hires_dir[512];
static char g_dump_dir[512];
static int g_inited;
static int g_dump_enabled = -1;

#define DUMP_CACHE_MAX 1024
static uint64_t g_dump_cache[DUMP_CACHE_MAX];
static int g_dump_cache_count;

static void ensure_dir(const char *dir)
{
    char buf[512];
    char *p;
    size_t len = strlen(dir);
    if (len >= sizeof(buf) - 1) return;
    memcpy(buf, dir, len + 1);

    for (p = buf + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            char c = *p;
            *p = '\0';
            MKDIR_ONE(buf);
            *p = c;
        }
    }
    MKDIR_ONE(buf);
}

void hd_textures_init(void)
{
    if (g_inited) return;
    g_inited = 1;

    const char *appdata = getenv("LOCALAPPDATA");
    if (appdata != NULL && appdata[0] != '\0') {
        snprintf(g_hires_dir, sizeof(g_hires_dir), "%s/eidos/textures/hires", appdata);
        snprintf(g_dump_dir, sizeof(g_dump_dir), "%s/eidos/textures/dump", appdata);
    } else {
        snprintf(g_hires_dir, sizeof(g_hires_dir), "textures/hires");
        snprintf(g_dump_dir, sizeof(g_dump_dir), "textures/dump");
    }

    ensure_dir(g_hires_dir);
    ensure_dir(g_dump_dir);
}

int hd_textures_dump_enabled(void)
{
    if (g_dump_enabled < 0) {
        const char *e = getenv("EIDOS_DUMP_TEXTURES");
        g_dump_enabled = (e != NULL && e[0] != '\0' && strcmp(e, "0") != 0);
    }
    return g_dump_enabled;
}

uint64_t hd_textures_calc_hash(uint32_t w, uint32_t h, const uint8_t *rgba)
{
    uint64_t hash = 14695981039346656037ull;
    size_t i, n = (size_t)w * h * 4u;

    hash = (hash ^ (uint64_t)w) * 1099511628211ull;
    hash = (hash ^ (uint64_t)h) * 1099511628211ull;
    if (rgba != NULL) {
        for (i = 0; i < n; i++) {
            hash = (hash ^ (uint64_t)rgba[i]) * 1099511628211ull;
        }
    }
    return hash;
}

uint8_t *hd_textures_load(uint64_t hash, int *out_w, int *out_h)
{
    char path[600];
    uint32_t hash32 = (uint32_t)(hash ^ (hash >> 32));
    int w = 0, h = 0, comp = 0;
    stbi_uc *data = NULL;

    hd_textures_init();

    /* Try 64-bit hex hash in primary hires dir */
    snprintf(path, sizeof(path), "%s/%016llx.png", g_hires_dir, (unsigned long long)hash);
    data = stbi_load(path, &w, &h, &comp, 4);

    /* Try 32-bit hex hash in primary hires dir */
    if (data == NULL) {
        snprintf(path, sizeof(path), "%s/%08x.png", g_hires_dir, hash32);
        data = stbi_load(path, &w, &h, &comp, 4);
    }

    /* Try local ./textures/hires directory as fallback */
    if (data == NULL) {
        snprintf(path, sizeof(path), "textures/hires/%016llx.png", (unsigned long long)hash);
        data = stbi_load(path, &w, &h, &comp, 4);
    }
    if (data == NULL) {
        snprintf(path, sizeof(path), "textures/hires/%08x.png", hash32);
        data = stbi_load(path, &w, &h, &comp, 4);
    }

    if (data != NULL && w > 0 && h > 0) {
        *out_w = w;
        *out_h = h;
        return (uint8_t *)data;
    }
    return NULL;
}

void hd_textures_dump(uint64_t hash, uint32_t w, uint32_t h, const uint8_t *rgba)
{
    char path[600];
    int i;

    if (!hd_textures_dump_enabled() || rgba == NULL || w == 0 || h == 0) {
        return;
    }
    hd_textures_init();

    /* Check in-memory dump cache to prevent repeated disk writes */
    for (i = 0; i < g_dump_cache_count; i++) {
        if (g_dump_cache[i] == hash) return;
    }
    if (g_dump_cache_count < DUMP_CACHE_MAX) {
        g_dump_cache[g_dump_cache_count++] = hash;
    } else {
        g_dump_cache[rand() % DUMP_CACHE_MAX] = hash;
    }

    snprintf(path, sizeof(path), "%s/%016llx.png", g_dump_dir, (unsigned long long)hash);
    stbi_write_png(path, (int)w, (int)h, 4, rgba, (int)w * 4);
}
