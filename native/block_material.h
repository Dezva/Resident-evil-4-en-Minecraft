#ifndef RE4_BLOCK_MATERIAL_H
#define RE4_BLOCK_MATERIAL_H
#include <stdint.h>
#define BLOCK_MAX_TILES 64u
typedef struct {uint32_t tile;float uv[8];} BlockFaceMaterial;
_Static_assert(sizeof(BlockFaceMaterial)==36,"Face material layout");
#endif
