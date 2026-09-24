#ifndef D2OPENGL_RENDERER_CORE_H
#define D2OPENGL_RENDERER_CORE_H

// The cutscene GL state and Bink player, the oglBlocks decoders, and the init/window/scene/cutscene
// slot functions (Mac 002de719-002df9b9).
// Each prototype carries its Mac provenance; "Mac fastcc" marks clang internal register
// arguments (ECX, EDX) that this build takes as ordinary parameters.

#include "types.h"

// Mac 002de719: saves g_aCutsceneGLCaps and sets the cutscene GL state
void OGL_CutsceneBeginGL(int nScreenWidth, int nScreenHeight, int nTexWidth, int nTexHeight);

// Mac 002de7e4: restores g_aCutsceneGLCaps
void OGL_CutsceneRestoreGL(void);

// Mac 002de824: the Bink player loop (the body of OpenGLPlayCutscene)
void OGL_PlayBinkMovie(const char *szFile, int nResolutionMode, BYTE *pBuffer, int nPitch, OGLFrameCallback fpFrame);

// Mac 002dee38: allocates the three oglBlocks buffers (lines 49-51)
void OGL_BlocksInit(void);

// Mac 002deec5: frees them (lines 58, 64, 70); NULL-safe
void OGL_BlocksShutdown(void);

// Mac 002def7a: uploads the floor tile on first use, then binds it
BOOL OGL_BindFloorTileTexture(D2TileLibraryEntryStrc *pTile);

// Mac 002df0d1: Mac fastcc: ECX pTile, EDX pDest, stack nPitch
void OGL_DecodeFloorTileBlocks(D2TileLibraryEntryStrc *pTile, BYTE *pDest, int nPitch);

// Mac 002df201: decodes a 32x32 block on first use, then binds it
void OGL_BindWallBlockTexture(D2TileLibraryBlockStrc *pBlock);

// Mac 002df347: the tile free callback (g_host.SetTileFreeCallback): Windows D2CMP passes the tile in ECX
void OGL_FASTCALL OGL_ReleaseTileTextures(D2TileLibraryEntryStrc *pTile);

// Mac 002df3c4 OGL_Initialize: slot 0: tail call to OGL_PlatformInitialize
BOOL OGL_Initialize(HINSTANCE hInstance);

// Mac 002df3cd OGL_InitPerspective: slot 1
BOOL OGL_InitPerspective(D2GfxSettingsStrc *pSettings, D2GfxHelperStrc *pHelpers);

// Mac 002df42b: the atexit handler OGL_InitPerspective registers
void OGL_AtExitRelease(void);

// Mac 002df45c OGL_Release: slot 2
BOOL OGL_Release(void);

// Mac 002df47b OGL_CreateWindow: slot 3; hWnd unused (the adapter stores it in g_hWnd)
BOOL OGL_CreateWindow(HWND hWnd, int nResolutionMode);

// Mac 002df4db: Blizzard's sOpenOpenGLWindow; Mac fastcc: ECX nResolutionMode
BOOL OGL_OpenOpenGLWindow(int nResolutionMode);

// Mac 002df760 OGL_DestroyWindow: slot 4
BOOL OGL_DestroyWindow(void);

// Mac 002df784: deletes g_pTextures and closes the GL window
void OGL_CloseOpenGLWindow(void);

// Mac 002df7ed OGL_ClearCaches: slot 53
void OGL_ClearCaches(void);

// Mac 002df81a OGL_EndCutScene: slot 5 (1999 D2OpenGL.dll OpenGLPauseSurface); hWnd unused
void OGL_EndCutScene(HWND hWnd, int nResolutionMode, int nWindowState);

// Mac 002df864 OGL_BeginCutScene: slot 13
BOOL OGL_BeginCutScene(void);

// Mac 002df86e OGL_PlayCutScene: slot 14
void OGL_PlayCutScene(const char *szFile, int nResolutionMode, OGLFrameCallback fpFrame);

// Mac 002df8f1 OGL_CheckCutScene: slot 15
BOOL OGL_CheckCutScene(void);

// Mac 002df903 OGL_BeginScene: slot 6; the clear colour is ignored
BOOL OGL_BeginScene(BOOL bClear, BYTE nRed, BYTE nGreen, BYTE nBlue);

// Mac 002df978 OGL_EndScene1: slot 7
BOOL OGL_EndScene1(void);

// Mac 002df992 OGL_UpdateScaleFactor: slot 21
void OGL_UpdateScaleFactor(int nScaleFactor);

// Mac 002df9a6 OGL_SetGamma: slot 22: tail call to OGL_SetGammaLevel
void OGL_SetGamma(int nGamma);

// Mac 002df9af OGL_CheckGamma: slot 23
int OGL_CheckGamma(void);

// Mac 002df9b9 OGL_EndScene2: slot 8: ends the texture frame and swaps
BOOL OGL_EndScene2(void);

#endif
