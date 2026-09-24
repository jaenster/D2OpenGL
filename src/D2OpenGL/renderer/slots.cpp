// The 54-slot Windows renderer table. D2gfx calls every slot __fastcall (ECX, EDX, then the stack,
// callee-popped); each adapter below has the Windows arity and return type
// and calls the recovered Mac slot function. Windows slots 0-10 are Mac slots 0-10, Windows 11-53 are
// Mac 14-56; the Mac-only slots 11-13 have no Windows dispatcher.
//
// Beyond passing arguments, the adapters do only what the Windows interface needs and the Mac did not:
// - slots 3, 5 and 9 store the game's HWND in g_hWnd (the Mac opened its own window and ignores it);
// - where the Mac function returns nothing but the Windows slot returns BOOL, the adapter returns TRUE;
// - slot 31's pTileData and slot 38's pCropRect, untyped in the Windows interface, are converted to
//   what the Mac function takes (a non-NULL flag and a D2GfxTileRectStrc pointer).

#include "renderer.h"
#include "../renderer.h"

#define SLOT OGL_FASTCALL

namespace {

// 1.10f's D2gfx sends option 8 before its windowed flag is final; take the flag itself when the window is
// (re)created.
void SyncWindowedFlag()
{
    if (g_host.pD2gfxWindowed)
        g_bFullscreen = (*g_host.pD2gfxWindowed == 0);
}

// Win 0 / Mac 0 002df3c4
BOOL SLOT Slot00_Initialize(HINSTANCE hInstance)
{
    return OGL_Initialize(hInstance);
}

// Win 1 / Mac 1 002df3cd
BOOL SLOT Slot01_InitPerspective(D2GfxSettingsStrc *pSettings, D2GfxHelperStrc *pHelpers)
{
    return OGL_InitPerspective(pSettings, pHelpers);
}

// Win 2 / Mac 2 002df45c
BOOL SLOT Slot02_Release()
{
    return OGL_Release();
}

// Win 3 / Mac 3 002df47b
BOOL SLOT Slot03_CreateWindow(HWND hWnd, int nResolutionMode)
{
    SyncWindowedFlag();
    g_hWnd = hWnd;
    return OGL_CreateWindow(hWnd, nResolutionMode);
}

// Win 4 / Mac 4 002df760
BOOL SLOT Slot04_DestroyWindow()
{
    return OGL_DestroyWindow();
}

// Win 5 / Mac 5 002df81a
void SLOT Slot05_EndCutScene(HWND hWnd, int nResolutionMode, int nWindowState)
{
    g_hWnd = hWnd;
    OGL_EndCutScene(hWnd, nResolutionMode, nWindowState);
}

// Win 6 / Mac 6 002df903
BOOL SLOT Slot06_BeginScene(BOOL bClear, BYTE nRed, BYTE nGreen, BYTE nBlue)
{
    return OGL_BeginScene(bClear, nRed, nGreen, nBlue);
}

// Win 7 / Mac 7 002df978
BOOL SLOT Slot07_EndScene1()
{
    return OGL_EndScene1();
}

// Win 8 / Mac 8 002df9b9
BOOL SLOT Slot08_EndScene2()
{
    return OGL_EndScene2();
}

// Win 9 / Mac 9 002dfbe3
BOOL SLOT Slot09_ResizeWindow(HWND hWnd, int nResolutionMode)
{
    SyncWindowedFlag();
    g_hWnd = hWnd;
    return OGL_ResizeWindow(hWnd, nResolutionMode);
}

// Win 10 / Mac 10 002df9e2
BOOL SLOT Slot10_GetBackBuffer(BYTE *pBuffer)
{
    return OGL_GetBackBuffer(pBuffer);
}

// Win 11 / Mac 14 002e18cb
BOOL SLOT Slot11_ActivateWindow(BOOL bActive)
{
    return OGL_ActivateWindow(bActive);
}

// Win 12 / Mac 15 002e1c3c
BOOL SLOT Slot12_SetOption(int nOption, int nValue)
{
    return OGL_SetOption(nOption, nValue);
}

// Win 13 / Mac 16 002df864
BOOL SLOT Slot13_BeginCutScene()
{
    return OGL_BeginCutScene();
}

// Win 14 / Mac 17 002df86e
void SLOT Slot14_PlayCutScene(char *szFile, int nResolutionMode, void *fpFrame)
{
    OGL_PlayCutScene(szFile, nResolutionMode, reinterpret_cast<OGLFrameCallback>(fpFrame));
}

// Win 15 / Mac 18 002df8f1
BOOL SLOT Slot15_CheckCutScene()
{
    return OGL_CheckCutScene();
}

// Win 16 / Mac 19 002e22fb
void SLOT Slot16_DecodeSmacker(char *szSmacker, D2SmackerContextStrc *pContext, int nVersion)
{
    OGL_DecodeSmacker(szSmacker, pContext, nVersion);
}

// Win 17 / Mac 20 002e274d
void SLOT Slot17_PlayerSmacker(D2SmackerContextStrc *pContext)
{
    OGL_PlayerSmacker(pContext);
}

// Win 18 / Mac 21 002e2730
void SLOT Slot18_CloseSmacker(D2SmackerContextStrc *pContext)
{
    OGL_CloseSmacker(pContext);
}

// Win 19 / Mac 22 002dfe26
D2RenderStatisticsStrc *SLOT Slot19_GetRenderStatistics()
{
    return OGL_GetRenderStatistics();
}

// Win 20 / Mac 23 002dfa7d
int SLOT Slot20_GetScreenSize(int *pWidth, int *pHeight)
{
    OGL_GetScreenSize(pWidth, pHeight);
    return TRUE;
}

// Win 21 / Mac 24 002df992
void SLOT Slot21_UpdateScaleFactor(int nScaleFactor)
{
    OGL_UpdateScaleFactor(nScaleFactor);
}

// Win 22 / Mac 25 002df9a6
BOOL SLOT Slot22_SetGamma(int nGamma)
{
    OGL_SetGamma(nGamma);
    return TRUE;
}

// Win 23 / Mac 26 002df9af
int SLOT Slot23_CheckGamma()
{
    return OGL_CheckGamma();
}

// Win 24 / Mac 27 002e1efd
void SLOT Slot24_SetPerspectiveScale(int nScaleX, int nScaleY)
{
    OGL_SetPerspectiveScale(nScaleX, nScaleY);
}

// Win 25 / Mac 28 002e1f1a
void SLOT Slot25_AdjustPerspectivePosition(int nX, int nY, int nBias, int *pXAdjust, int *pYAdjust)
{
    OGL_AdjustPerspectivePosition(nX, nY, nBias, pXAdjust, pYAdjust);
}

// Win 26 / Mac 29 002e1fcc
void SLOT Slot26_PerspectiveScalePosition(int nX, int nY, int nAngle, int *pXAdjust, int *pYAdjust, BOOL bOrder)
{
    OGL_PerspectiveScalePosition(nX, nY, nAngle, pXAdjust, pYAdjust, bOrder);
}

// Win 27 / Mac 30 002e20bf
void SLOT Slot27_SetDefaultPerspectiveFactor()
{
    OGL_SetDefaultPerspectiveFactor();
}

// Win 28 / Mac 31 002dfa9e
BOOL SLOT Slot28_SetPalette(PALETTEENTRY *pPalette)
{
    OGL_SetPalette(pPalette);
    return TRUE;
}

// Win 29 / Mac 32 002dfbab
void SLOT Slot29_SetPaletteTable(BYTE **pPaletteTable)
{
    OGL_SetPaletteTable(pPaletteTable);
}

// Win 30 / Mac 33 002e4981
void SLOT Slot30_SetGlobalLight(BYTE nRed, BYTE nGreen, BYTE nBlue)
{
    OGL_SetGlobalLight(nRed, nGreen, nBlue);
}

// Win 31 / Mac 34 002e07fe
BOOL SLOT Slot31_DrawGroundTile(D2TileLibraryEntryStrc *pTile, D2GfxLightExStrc *pLight, int nX, int nY, int nWorldX,
                                int nWorldY, BYTE nAlpha, int nScreenPanels, void *pTileData)
{
    return OGL_DrawGroundTile(pTile, pLight, nX, nY, nWorldX, nWorldY, nAlpha, nScreenPanels, pTileData != NULL);
}

// Win 32 / Mac 35 002e2d91
void SLOT Slot32_DrawPerspectiveImage(D2GfxDataStrc *pData, int nX, int nY, DWORD dwGamma, int nDrawMode,
                                      int nScreenMode, BYTE *pPalette)
{
    OGL_DrawPerspectiveImage(pData, nX, nY, dwGamma, nDrawMode, nScreenMode, pPalette);
}

// Win 33 / Mac 36 002e39bc
void SLOT Slot33_DrawImage(D2GfxDataStrc *pData, int nX, int nY, DWORD dwGamma, int nDrawMode, BYTE *pPalette)
{
    OGL_DrawImage(pData, nX, nY, dwGamma, nDrawMode, pPalette);
}

// Win 34 / Mac 37 002e3e98
void SLOT Slot34_DrawShiftedImage(D2GfxDataStrc *pData, int nX, int nY, DWORD dwGamma, int nDrawMode,
                                  int nGlobalPaletteShift)
{
    OGL_DrawShiftedImage(pData, nX, nY, dwGamma, nDrawMode, nGlobalPaletteShift);
}

// Win 35 / Mac 38 002e3f6c
void SLOT Slot35_DrawVerticalCropImage(D2GfxDataStrc *pData, int nX, int nY, int nSkipLines, int nDrawLines,
                                       int nDrawMode)
{
    OGL_DrawVerticalCropImage(pData, nX, nY, nSkipLines, nDrawLines, nDrawMode);
}

// Win 36 / Mac 39 002e4332
void SLOT Slot36_DrawShadow(D2GfxDataStrc *pData, int nX, int nY)
{
    OGL_DrawShadow(pData, nX, nY);
}

// Win 37 / Mac 40 002e476f
void SLOT Slot37_DrawImageFast(D2GfxDataStrc *pData, int nX, int nY, BYTE nPaletteIndex)
{
    OGL_DrawImageFast(pData, nX, nY, nPaletteIndex);
}

// Win 38 / Mac 41 002e40a2
void SLOT Slot38_DrawClippedImage(D2GfxDataStrc *pData, int nX, int nY, void *pCropRect, int nDrawMode)
{
    OGL_DrawClippedImage(pData, nX, nY, static_cast<const D2GfxTileRectStrc *>(pCropRect), nDrawMode);
}

// Win 39 / Mac 42 002e5ea1
BOOL SLOT Slot39_DrawWallTile(D2TileLibraryEntryStrc *pTile, int nX, int nY, D2GfxLightStrc *pLight, int nScreenPanels)
{
    return OGL_DrawWallTile(pTile, nX, nY, pLight, nScreenPanels);
}

// Win 40 / Mac 43 002e5419
BOOL SLOT Slot40_DrawTransWallTile(D2TileLibraryEntryStrc *pTile, int nX, int nY, D2GfxLightStrc *pLight,
                                   int nScreenPanels, BYTE nAlpha)
{
    return OGL_DrawTransWallTile(pTile, nX, nY, pLight, nScreenPanels, nAlpha);
}

// Win 41 / Mac 44 002e4ae1
BOOL SLOT Slot41_DrawShadowTile(D2TileLibraryEntryStrc *pTile, int nX, int nY, int nDrawMode, int nScreenPanels)
{
    return OGL_DrawShadowTile(pTile, nX, nY, nDrawMode, nScreenPanels);
}

// Win 42 / Mac 45 002e03c0
void SLOT Slot42_DrawRect(RECT *pRect, BYTE nPaletteIndex)
{
    OGL_DrawRect(pRect, nPaletteIndex);
}

// Win 43 / Mac 46 002e0534
void SLOT Slot43_DrawRectEx(RECT *pRect, BYTE nPaletteIndex)
{
    OGL_DrawRectEx(pRect, nPaletteIndex);
}

// Win 44 / Mac 47 002e06bd
void SLOT Slot44_DrawSolidRect(RECT *pRect, BYTE nPaletteIndex)
{
    OGL_DrawSolidRect(pRect, nPaletteIndex);
}

// Win 45 / Mac 48 002e0757
void SLOT Slot45_DrawSolidSquare(POINT *pPoint, BYTE nSize, BYTE nPaletteIndex)
{
    OGL_DrawSolidSquare(pPoint, nSize, nPaletteIndex);
}

// Win 46 / Mac 49 002dff15
void SLOT Slot46_DrawSolidRectEx(int nX0, int nY0, int nX1, int nY1, DWORD dwColor, int nDrawMode)
{
    OGL_DrawSolidRectEx(nX0, nY0, nX1, nY1, dwColor, nDrawMode);
}

// Win 47 / Mac 50 002e00c0
void SLOT Slot47_DrawSolidRectAlpha(int nX0, int nY0, int nX1, int nY1, DWORD dwColor, BYTE nAlpha)
{
    OGL_DrawSolidRectAlpha(nX0, nY0, nX1, nY1, dwColor, nAlpha);
}

// Win 48 / Mac 51 002e0245
void SLOT Slot48_DrawLine(int nX0, int nY0, int nX1, int nY1, DWORD dwColor, BYTE nAlpha)
{
    OGL_DrawLine(nX0, nY0, nX1, nY1, dwColor, nAlpha);
}

// Win 49 / Mac 52 002e07d5
void SLOT Slot49_ClearScreen(BOOL bPartial)
{
    OGL_ClearScreen(bPartial);
}

// Win 50 / Mac 53 002dfe41
void SLOT Slot50_DrawString(int nX, int nY, char *szFormat, va_list va)
{
    OGL_DrawString(nX, nY, szFormat, va);
}

// Win 51 / Mac 54 002dfefa
void SLOT Slot51_DrawLight(DWORD *pLight, DWORD *pPlayerLight, int nX, int nY)
{
    OGL_DrawLight(pLight, pPlayerLight, nX, nY);
}

// Win 52 / Mac 55 002e480c
void SLOT Slot52_DebugFillBackBuffer(int nX, int nY)
{
    OGL_DebugFillBackBuffer(nX, nY);
}

// Win 53 / Mac 56 002df7ed
void SLOT Slot53_ClearCaches()
{
    OGL_ClearCaches();
}

template <typename Fn> void *slot_ptr(Fn fn)
{
    return reinterpret_cast<void *>(fn);
}

void *const g_aRendererTable[RENDERER_SLOT_COUNT] = {
    slot_ptr(Slot00_Initialize),
    slot_ptr(Slot01_InitPerspective),
    slot_ptr(Slot02_Release),
    slot_ptr(Slot03_CreateWindow),
    slot_ptr(Slot04_DestroyWindow),
    slot_ptr(Slot05_EndCutScene),
    slot_ptr(Slot06_BeginScene),
    slot_ptr(Slot07_EndScene1),
    slot_ptr(Slot08_EndScene2),
    slot_ptr(Slot09_ResizeWindow),
    slot_ptr(Slot10_GetBackBuffer),
    slot_ptr(Slot11_ActivateWindow),
    slot_ptr(Slot12_SetOption),
    slot_ptr(Slot13_BeginCutScene),
    slot_ptr(Slot14_PlayCutScene),
    slot_ptr(Slot15_CheckCutScene),
    slot_ptr(Slot16_DecodeSmacker),
    slot_ptr(Slot17_PlayerSmacker),
    slot_ptr(Slot18_CloseSmacker),
    slot_ptr(Slot19_GetRenderStatistics),
    slot_ptr(Slot20_GetScreenSize),
    slot_ptr(Slot21_UpdateScaleFactor),
    slot_ptr(Slot22_SetGamma),
    slot_ptr(Slot23_CheckGamma),
    slot_ptr(Slot24_SetPerspectiveScale),
    slot_ptr(Slot25_AdjustPerspectivePosition),
    slot_ptr(Slot26_PerspectiveScalePosition),
    slot_ptr(Slot27_SetDefaultPerspectiveFactor),
    slot_ptr(Slot28_SetPalette),
    slot_ptr(Slot29_SetPaletteTable),
    slot_ptr(Slot30_SetGlobalLight),
    slot_ptr(Slot31_DrawGroundTile),
    slot_ptr(Slot32_DrawPerspectiveImage),
    slot_ptr(Slot33_DrawImage),
    slot_ptr(Slot34_DrawShiftedImage),
    slot_ptr(Slot35_DrawVerticalCropImage),
    slot_ptr(Slot36_DrawShadow),
    slot_ptr(Slot37_DrawImageFast),
    slot_ptr(Slot38_DrawClippedImage),
    slot_ptr(Slot39_DrawWallTile),
    slot_ptr(Slot40_DrawTransWallTile),
    slot_ptr(Slot41_DrawShadowTile),
    slot_ptr(Slot42_DrawRect),
    slot_ptr(Slot43_DrawRectEx),
    slot_ptr(Slot44_DrawSolidRect),
    slot_ptr(Slot45_DrawSolidSquare),
    slot_ptr(Slot46_DrawSolidRectEx),
    slot_ptr(Slot47_DrawSolidRectAlpha),
    slot_ptr(Slot48_DrawLine),
    slot_ptr(Slot49_ClearScreen),
    slot_ptr(Slot50_DrawString),
    slot_ptr(Slot51_DrawLight),
    slot_ptr(Slot52_DebugFillBackBuffer),
    slot_ptr(Slot53_ClearCaches),
};

} // namespace

extern "C" void *const *renderer_table(void)
{
    return g_aRendererTable;
}
