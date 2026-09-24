#ifndef D2OPENGL_RENDERER_DRAW2D_H
#define D2OPENGL_RENDERER_DRAW2D_H

// Screen read, palette, texture/blend/filter state, strings, rects, lines and clear
// (Mac 002df9e2-002e07ec).
// Each prototype carries its Mac provenance.

#include "types.h"

// Mac 002df9e2: slot 10: 24-bit RGB, top row first
BOOL OGL_GetBackBuffer(BYTE *pBuffer);

// Mac 002dfa7d OGL_GetScreenSize: slot 20
void OGL_GetScreenSize(int *pWidth, int *pHeight);

// Mac 002dfa9e OGL_SetPalette: slot 28: 256 entries
void OGL_SetPalette(const PALETTEENTRY *pPalette);

// Mac 002dfb94: g_aPaletteRGBA[nIndex]
uint32_t OGL_GetPaletteColor(int nIndex);

// Mac 002dfbab OGL_SetPaletteTable: slot 29: copies 72 pointers
void OGL_SetPaletteTable(BYTE **pPaletteTable);

// Mac 002dfbe3: slot 9; hWnd unused (the adapter stores it in g_hWnd)
BOOL OGL_ResizeWindow(HWND hWnd, int nResolutionMode);

// Mac 002dfc65: OGLTextureMode, cached in g_nCurTextureMode
void OGL_SetTextureMode(int nMode);

// Mac 002dfcf5: OGLBlendMode, cached in g_nCurBlendMode
void OGL_SetBlendMode(int nBlendMode);

// Mac 002dfdb0: OGLTextureFilter, on GL_TEXTURE_2D
void OGL_SetTextureFilter(int nFilter);

// Mac 002dfe26 OGL_GetRenderStatistics: slot 19: NULL
D2RenderStatisticsStrc *OGL_GetRenderStatistics(void);

// Mac 002dfe2d: empty; called by OGL_OpenOpenGLWindow
void OGL_OpenWindowHook2(void);

// Mac 002dfe32: empty; called by OGL_CloseOpenGLWindow
void OGL_CloseWindowHook1(void);

// Mac 002dfe37: empty; called by OGL_OpenOpenGLWindow
void OGL_OpenWindowHook1(void);

// Mac 002dfe3c: empty; called by OGL_CloseOpenGLWindow
void OGL_CloseWindowHook2(void);

// Mac 002dfe41 OGL_DrawString: slot 50
void OGL_DrawString(int nX, int nY, const char *szFormat, va_list va);

// Mac 002dfefa OGL_DrawLight: slot 51: empty
void OGL_DrawLight(DWORD *pLight, DWORD *pPlayerLight, int nX, int nY);

// Mac 002dfeff: g_pGfxSettings->bBlendedShadows
BOOL OGL_GetBlendedShadows(void);

// Mac 002dff15 OGL_DrawSolidRectEx: slot 46
void OGL_DrawSolidRectEx(int nX0, int nY0, int nX1, int nY1, DWORD dwColor, int nDrawMode);

// Mac 002e00c0 OGL_DrawSolidRectAlpha: slot 47
void OGL_DrawSolidRectAlpha(int nX0, int nY0, int nX1, int nY1, DWORD dwColor, BYTE nAlpha);

// Mac 002e0245 OGL_DrawLine: slot 48
void OGL_DrawLine(int nX0, int nY0, int nX1, int nY1, DWORD dwColor, BYTE nAlpha);

// Mac 002e03c0 OGL_DrawRect: slot 42: a diamond inscribed in the rect
void OGL_DrawRect(RECT *pRect, BYTE nPaletteIndex);

// Mac 002e0534 OGL_DrawRectEx: slot 43: the rect outline
void OGL_DrawRectEx(RECT *pRect, BYTE nPaletteIndex);

// Mac 002e06bd OGL_DrawSolidRect: slot 44
void OGL_DrawSolidRect(RECT *pRect, BYTE nPaletteIndex);

// Mac 002e0757 OGL_DrawSolidSquare: slot 45
void OGL_DrawSolidSquare(POINT *pPoint, BYTE nSize, BYTE nPaletteIndex);

// Mac 002e07d5 OGL_ClearScreen: slot 49; bPartial ignored
void OGL_ClearScreen(BOOL bPartial);

// Mac 002e07ec: g_bGroundMeshDirty = true
void OGL_InvalidateGroundMesh(void);

#endif
