#ifndef D2OPENGL_RENDERER_TILES_H
#define D2OPENGL_RENDERER_TILES_H

// Tiles (Mac 002e07fe, 002e4811-002e5ea1): the ground tile, oglVertex, the global light, the
// shadow tile and the wall tiles.
// Each prototype carries its Mac provenance; "Mac fastcc" marks clang internal register
// arguments (ECX, EDX) that this build takes as ordinary parameters.

#include "types.h"

// Mac 002e07fe OGL_DrawGroundTile: slot 31; pLight is 36 vertices; only pTile, pLight, nAlpha are read
BOOL OGL_DrawGroundTile(D2TileLibraryEntryStrc *pTile, D2GfxLightExStrc *pLight, int nX, int nY, int nWorldX, int nWorldY, BYTE nAlpha, int nScreenPanels, BOOL bFilterFloor);

// Mac 002e4811: oglVertex.cpp line 24
void OGL_AllocModulateTable(void);

// Mac 002e4850: line 31; NULL-safe
void OGL_FreeModulateTable(void);

// Mac 002e4899: [a][b] = a * b / 255
void OGL_BuildModulateTable(void);

// Mac 002e48fc: alpha untouched
void OGL_SetVertexColorFromGlobalLight(OGLVertex *pVertex, BYTE nIntensity);

// Mac 002e4940: dwLight = D2GfxLightStrc packed (OGL_LightToDword); alpha untouched
void OGL_SetVertexColorFromLight(OGLVertex *pVertex, DWORD dwLight);

// Mac 002e4981 OGL_SetGlobalLight: slot 30
void OGL_SetGlobalLight(BYTE nRed, BYTE nGreen, BYTE nBlue);

// Mac 002e49a7: normalised UVs; converts g_aWallCornerTable once
void OGL_InitTileVertices(void);

// Mac 002e4ae1 OGL_DrawShadowTile: slot 41; the low byte of nDrawMode is the shadow intensity
BOOL OGL_DrawShadowTile(D2TileLibraryEntryStrc *pTile, int nX, int nY, int nDrawMode, int nScreenPanels);

// Mac 002e531b: 32x32 texel UVs for rectangle textures
void OGL_SetTileVerticesTexelUV(void);

// Mac 002e5419 OGL_DrawTransWallTile: slot 40
BOOL OGL_DrawTransWallTile(D2TileLibraryEntryStrc *pTile, int nX, int nY, D2GfxLightStrc *pLight, int nScreenPanels, BYTE nAlpha);

// Mac 002e547c: Mac fastcc: ECX pTile, EDX nX, 4 stack args
BOOL OGL_DrawWallTileBlocks(D2TileLibraryEntryStrc *pTile, int nX, int nY, const D2GfxLightStrc *pLight, BYTE nAlpha, int nScreenPanels);

// Mac 002e5ea1 OGL_DrawWallTile: slot 39
BOOL OGL_DrawWallTile(D2TileLibraryEntryStrc *pTile, int nX, int nY, D2GfxLightStrc *pLight, int nScreenPanels);

#endif
