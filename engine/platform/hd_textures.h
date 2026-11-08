#ifndef EIDOS_HD_TEXTURES_H
#define EIDOS_HD_TEXTURES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void hd_textures_init(void);
uint64_t hd_textures_calc_hash(uint32_t w, uint32_t h, const uint8_t *rgba);
uint8_t *hd_textures_load(uint64_t hash, int *out_w, int *out_h);
void hd_textures_dump(uint64_t hash, uint32_t w, uint32_t h, const uint8_t *rgba);
int hd_textures_dump_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* EIDOS_HD_TEXTURES_H */
