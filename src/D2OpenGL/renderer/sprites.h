#ifndef D2OPENGL_RENDERER_SPRITES_H
#define D2OPENGL_RENDERER_SPRITES_H

// Sprites (Mac 002e1ddf-002e480c): oglPerspective, oglSmack (the 468x60 banner), oglSprite and
// the image draw slots.
// Each prototype carries its Mac provenance; "Mac fastcc" marks clang internal register
// arguments (ECX, EDX) that this build takes as ordinary parameters.

#include "types.h"

// Mac 002e1ddf: oglPerspective.cpp line 57
void OGL_AllocPerspectiveTable(void);

// Mac 002e1e1e: line 64; NULL-safe
void OGL_FreePerspectiveTable(void);

// Mac 002e1e67: fills the table, g_nPerspectiveFactor = 0x100
void OGL_BuildPerspectiveTable(void);

// Mac 002e1ede: bPerspectiveCapable ? bPerspectiveEnabled : 0
BOOL OGL_IsPerspectiveEnabled(void);

// Mac 002e1efd OGL_SetPerspectiveScale: slot 24: the perspective centre
void OGL_SetPerspectiveScale(int nScaleX, int nScaleY);

// Mac 002e1f1a OGL_AdjustPerspectivePosition: slot 25; 32-bit wrapping arithmetic
void OGL_AdjustPerspectivePosition(int nX, int nY, int nBias, int *pXAdjust, int *pYAdjust);

// Mac 002e1fcc OGL_PerspectiveScalePosition: slot 26; nAngle unused; 32-bit wrapping arithmetic
void OGL_PerspectiveScalePosition(int nX, int nY, int nAngle, int *pXAdjust, int *pYAdjust, BOOL bOrder);

// Mac 002e20bf OGL_SetDefaultPerspectiveFactor: slot 27
void OGL_SetDefaultPerspectiveFactor(void);

// Mac 002e20d4: 800x600 only
void OGL_InitSmackerTextures(void);

// Mac 002e221e
void OGL_FreeSmackerTextures(void);

// Mac 002e22fb OGL_DecodeSmacker: slot 16; nVersion's low byte is the mode ('p' or 's')
void OGL_DecodeSmacker(char *szSmacker, D2SmackerContextStrc *pContext, int nVersion);

// Mac 002e2730 OGL_CloseSmacker: slot 18
void OGL_CloseSmacker(D2SmackerContextStrc *pContext);

// Mac 002e274d OGL_PlayerSmacker: slot 17: draws the 468x60 banner
void OGL_PlayerSmacker(D2SmackerContextStrc *pContext);

// Mac 002e2af8: Mac fastcc: ECX pIndices, EDX pDst, stack pRGB
void OGL_SmackExpandToRGBA(const BYTE *pIndices, uint32_t *pDst, const BYTE *pRGB);

// Mac 002e2b70: registers OGL_FreeSpriteTextures with D2CMP
void OGL_InitSpriteBuffers(void);

// Mac 002e2c8c: the pixel-data free callback: Windows D2CMP passes the slots in ECX
void OGL_FASTCALL OGL_FreeSpriteTextures(OGLSpriteTexSlot *pSlots);

// Mac 002e2cd0
void OGL_FreeSpriteBuffers(void);

// Mac 002e2d91 OGL_DrawPerspectiveImage: slot 32
void OGL_DrawPerspectiveImage(D2GfxDataStrc *pData, int nX, int nY, DWORD dwGamma, int nDrawMode, int nScreenMode, BYTE *pPalette);

// Mac 002e33a9: Mac fastcc: ECX pData, EDX pSprite
BOOL OGL_SpriteLoad(D2GfxDataStrc *pData, OGLSprite *pSprite);

// Mac 002e344e: Mac fastcc: ECX pSprite
void OGL_SpriteCalcTextureSize(OGLSprite *pSprite);

// Mac 002e34d7: Mac fastcc: ECX pSprite, EDX pPalette, stack nOutlineColor, nTexMode
BOOL OGL_SpriteBindTexture(const OGLSprite *pSprite, BYTE *pPalette, BYTE nOutlineColor, int nTexMode);

// Mac 002e3934: Mac fastcc: ECX nDrawMode; returns the vertex alpha
BYTE OGL_SetDrawModeBlend(int nDrawMode);

// Mac 002e39bc OGL_DrawImage: slot 33
void OGL_DrawImage(D2GfxDataStrc *pData, int nX, int nY, DWORD dwGamma, int nDrawMode, BYTE *pPalette);

// Mac 002e3ab2: Mac fastcc: ECX pSprite, EDX nX, 7 stack args
void OGL_DrawSpriteQuad(const OGLSprite *pSprite, int nX, int nY, DWORD dwGamma, int nColorIndex, BYTE nAlpha, BOOL bUseColorIndex, int nTexWidth, int nTexHeight);

// Mac 002e3e98 OGL_DrawShiftedImage: slot 34
void OGL_DrawShiftedImage(D2GfxDataStrc *pData, int nX, int nY, DWORD dwGamma, int nDrawMode, int nGlobalPaletteShift);

// Mac 002e3f6c OGL_DrawVerticalCropImage: slot 35
void OGL_DrawVerticalCropImage(D2GfxDataStrc *pData, int nX, int nY, int nSkipLines, int nDrawLines, int nDrawMode);

// Mac 002e40a2 OGL_DrawClippedImage: slot 38
void OGL_DrawClippedImage(D2GfxDataStrc *pData, int nX, int nY, const D2GfxTileRectStrc *pCropRect, int nDrawMode);

// Mac 002e4332 OGL_DrawShadow: slot 36
void OGL_DrawShadow(D2GfxDataStrc *pData, int nX, int nY);

// Mac 002e476f OGL_DrawImageFast: slot 37: one-pixel outline in nPaletteIndex
void OGL_DrawImageFast(D2GfxDataStrc *pData, int nX, int nY, BYTE nPaletteIndex);

// Mac 002e480c OGL_DebugFillBackBuffer: slot 52: empty
void OGL_DebugFillBackBuffer(int nX, int nY);

#endif
