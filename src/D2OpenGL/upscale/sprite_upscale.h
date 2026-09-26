#ifndef D2OPENGL_UPSCALE_SPRITE_UPSCALE_H
#define D2OPENGL_UPSCALE_SPRITE_UPSCALE_H

// Sprite textures made larger in palette-index space. With a render scale above 1 and Upscale=mmpx,
// every sprite texture (DC6 and DCC alike: both reach the renderer as a decoded cel) is made from the
// cel's index image, after the draw's colour map, magnified before the palette turns it into colours.
// The texture is larger, its texture coordinates are the same fractions, so the sprite draws at the
// same place and size in game pixels; the engine's cel data (sizes, offsets, hit tests) is never
// touched.
//
// The image comes from the HD pack (hdpack.h) when the pack holds the cel's frame at the texture
// factor, else from libd2's MMPX (d2_upscale_indices).
//
// The texture factor follows the render scale: 2 for 2 and 3 (at 3 the GPU magnifies the 2x texture by
// 1.5, nearest), 4 for 4. 1 when the stage is off or Upscale=none.

#include <stdint.h>

struct OGLSprite;

// The factor for the texture manager being created; its staging buffer must hold the larger textures.
// Called by the texture manager's constructor, undone by Upscale_CloseTextures in its destructor. The
// first call also opens the HD pack.
int Upscale_OpenTextures(void);
void Upscale_CloseTextures(void);

// The texture image for a sprite cel: pIndices (*pnWidth x *pnHeight, tightly packed, top row first)
// is the cel as OGL_SpriteBindTexture decoded it for texture slot nSlot, with the draw's colour map
// pColorMap (NULL for none) already applied. Returns the image to upload, updating the size, or
// pIndices unchanged at factor 1 or when nothing could be made.
//
// An HD pack frame is looked up by the cel's own indices, before any colour map, so it is found for
// every colour variant; the draw's colour map, and slot 2's remap, are then applied to it as the
// decode applied them to the cel. Slot 3 (the outline) is always magnified.
const uint8_t *Upscale_Sprite(const OGLSprite *pSprite, const uint8_t *pIndices, int nSlot, uint8_t *pColorMap,
                              int *pnWidth, int *pnHeight);

// Floor tiles and wall blocks, made larger the same way before their textures are made: from the HD
// pack when it holds the image, else MMPX. pSrc is nWidth x nHeight at nPitch, index 0 transparent.
// Returns nWidth x nHeight times the factor, tightly packed, or NULL to upload pSrc as it is. Only for
// textures addressed by fractions (the texture-rectangle path addresses tiles in texels).
//
// A floor tile's empty corners are first filled from the tile itself one lattice step (half the cell,
// nCellWidth x nCellHeight, across and down) away, so its edges magnify as if its neighbours were copies
// of it; the result keeps the tile's own outline, so tiles still meet exactly.
const uint8_t *Upscale_FloorTile(const uint8_t *pSrc, int nWidth, int nHeight, int nPitch, int nCellWidth,
                                 int nCellHeight);
const uint8_t *Upscale_WallBlock(const uint8_t *pSrc, int nWidth, int nHeight);
int Upscale_TileFactor(void);  // the factor of the images above

// Switches between the upscaled sprites and tiles and the original ones (pixels repeated); the caller
// drops the textures so they are made again.
bool Upscale_ToggleOriginal(void);

#endif
