#ifndef D2OPENGL_RENDERER_GLOBALS_H
#define D2OPENGL_RENDERER_GLOBALS_H

// Every renderer global the recovered functions use, in Mac address order, one provenance line each.
// All are renderer-private: the renderer shares no data global with the game. The game data it reads
// arrives by pointer (g_pGfxSettings, g_pPaletteTable) or through g_host (host/host.h).
//
// Not here, on purpose:
// - the Mac renderer table 003ce6ec (the Windows table is slots.cpp) and the vtables/RTTI (C++);
// - the __nl_symbol_ptr cells 003961e0..003963b4 (PIC indirection to the globals below);
// - __const float/double constant-pool entries: written as literals;
// - Mac-only data with no Windows use: 00334b88 HIWindow attributes, 00334b9c EventTypeSpec[3],
//   00552b44/00552b48 the Carbon event UPP static and its guard.
//
// A few Mac file-statics are here too, so that one header serves every source file.

#include "types.h"

class CD2Textures;

// The __FILE__ strings the Mac passes to the Fog/Storm allocators.
extern const char g_szFileCD2Textures[];        // Mac 0031c2bf
extern const char g_szFileOglBlocks[];          // Mac 0031e707
extern const char g_szFileOglPerspective[];     // Mac 0031ebf9
extern const char g_szFileOglSmack[];           // Mac 0031ec53
extern const char g_szFileOglSprite[];          // Mac 0031eca7
extern const char g_szFileOglVertex[];          // Mac 0031ecfc
extern const char g_szFileCOGLAGPTextures[];    // Mac 0031eda2
extern const char g_szFileStorm[];              // Mac Storm operator new/delete (000095fc..000096b0)

// Mac 00334bb4: glColor4ubv white for the smacker banner
extern const BYTE g_abSmackWhite[4];
// Mac 00334bc0: the memset_pattern16 fill for the smacker RGBA buffers
extern const uint32_t g_adwSmackFillPattern[4];
// Mac 00334be8: the two GL_TRIANGLE_STRIPs over the 5x3 wall/shadow grid (00334c10 is row 1)
extern const int g_aTileGridStripIndex[2][10];
// Mac 00334c40: COGLAGPTextures size-class widths
extern const int g_anAGPClassWidth[6];
// Mac 00334c58: COGLAGPTextures size-class heights
extern const int g_anAGPClassHeight[6];
// Mac 00334c70: COGLAGPTextures slots per class (5184 in total)
extern const int g_anAGPClassCount[6];

// Mac 003ce7d0: GL caps saved around the cutscene {cap, saved, wanted}
extern CutsceneGLCapState g_aCutsceneGLCaps[3];
// Mac 003ce7e8: D2 resolution of the open window (0 = 640x480, 1 = 800x600), -1 before the first
extern int g_nResolution;
// Mac 003ce7ec: colour depth for the pixel format and the mode switch (16 or 32)
extern int g_nColorBits;
// Mac 003ce7f0: requested AGL renderer id, -1 = any (no WGL meaning)
extern int g_nRendererId;
// Mac 003ce7f4: gamma, 55..255, 100 = identity
extern int g_nGamma;
// Mac 003ce7f8: contrast, 0..100
extern int g_nContrast;
// Mac 003ce7fc: wall corner offsets [direction][column edge]; OGL_InitTileVertices shifts nX/nY to 16.16 once
extern OGLWallCorner g_aWallCornerTable[10][6];

// Mac 0055256c: cutscene quad width (screen pixels)
extern int g_nCutsceneScreenWidth;
// Mac 00552570: cutscene quad height
extern int g_nCutsceneScreenHeight;
// Mac 00552574: cutscene texture width
extern int g_nCutsceneTexWidth;
// Mac 00552578: cutscene texture height
extern int g_nCutsceneTexHeight;
// Mac 0055257c: cutscene texture name
extern GLuint g_nCutsceneTexture;
// Mac 00552580: the movie was skipped or reached its last frame
extern bool g_bCutsceneDone;
// Mac 00552584: the open Bink movie
extern HBINK g_pBink;
// Mac 00552588: BinkCopyToBuffer destination x
extern uint32_t g_nBinkDestX;
// Mac 0055258c: BinkCopyToBuffer destination y
extern uint32_t g_nBinkDestY;
// Mac 00552590: 32x32 8-bit decode buffer for wall/shadow blocks (0x400)
extern BYTE *g_pWallBlockBuffer;
// Mac 00552594: 0x1000 buffer, allocated and freed, never read
extern BYTE *g_pBlockScratch;
// Mac 00552598: floor tile decode buffer (0x20000)
extern BYTE *g_pFloorTileBuffer;
// Mac 0055259c: a cutscene is playing (slot 15 CheckCutScene)
extern bool g_bInCutscene;
// Mac 005525a0: OGL_SetTextureMode cache (OGLTextureMode)
extern int g_nCurTextureMode;
// Mac 005525a4: OGL_SetBlendMode cache (OGLBlendMode)
extern int g_nCurBlendMode;
// Mac 005525a8: the renderer's copy of the game's 72 palette table pointers
extern D2PaletteTable g_aPaletteTable;
// Mac 005526c8: OGL_AtExitRelease ran
extern bool g_bAtExitReleased;
// Mac 005526cc: OGL_DrawGroundTile must rebuild its UVs and strip table
extern bool g_bGroundMeshDirty;
// Mac 005526d0: the 6x6 ground tile vertex grid
extern OGLVertex g_aGroundVerts[6][6];
// Mac 00552a30: the 5 triangle strips over g_aGroundVerts, 12 vertices each
extern OGLVertex *g_apGroundStrip[5][12];
// Mac 00552b20: OGL_PlatformSetRenderer found an accelerated renderer
extern bool g_bAcceleratedRenderer;
// Mac 00552b24: fullscreen (SetOption 8 stores !bWindowed); tested == 1
extern int g_bFullscreen;
// Mac 00552b28: the display mode was switched and must be restored (Mac: the saved CGDisplayModeRef)
extern BOOL g_bDisplayModeChanged;
// Mac 00552b2c: the display (Mac CGDirectDisplayID; SetOption 0/1), NULL = primary
extern HMONITOR g_hDisplay;
// Mac 00552b30: the game's window (Mac WindowRef); stored by the slot 3/5/9 adapters, never destroyed
extern HWND g_hWnd;
// Mac 00552b34: the GL context (Mac AGLContext)
extern HGLRC g_hGLRC;
// Windows: the window DC the context is made current on (no Mac counterpart)
extern HDC g_hDC;
// Mac 00552b38: window content origin in screen coordinates (read only by the Mac-only slots 11-13)
extern POINT g_ptWindowOrigin;
// Mac 00552b3c: OGL_StartCutscene ran
extern bool g_bCutsceneActive;
// Mac 00552b40: 640x480x4 cutscene frame buffer (Storm new[] 0x12c000), kept between cutscenes
extern BYTE *g_pCutsceneBuffer;
// Mac 00552b50: fullscreen display mode per D2 resolution (SetOption 2 fills [0], 3 fills [1] and [2])
extern OGLDisplayModeRec g_aDisplayModes[3];
// Mac 00552b70: perspective divisor table, int[0x2000], indexed (n + 0x1000) & 0x1fff
extern int *g_pPerspectiveTable;
// Mac 00552b74: perspective centre x (slot 24 nScaleX)
extern int g_nPerspectiveCenterX;
// Mac 00552b78: perspective centre y (slot 24 nScaleY)
extern int g_nPerspectiveCenterY;
// Mac 00552b7c: 512x64 8-bit smacker banner index buffer (0x8000)
extern BYTE *g_pSmackIndexBuf;
// Mac 00552b80: the two 256x64 RGBA halves of the banner (0x10000 each)
extern uint32_t *g_apSmackPixels[2];
// Mac 00552b88: the two banner textures
extern GLuint g_aSmackTextures[2];
// Mac 00552b90: the banner quad
extern OGLSmackVertex g_aSmackQuad[4];
// Mac 00552bc0: 8-bit sprite staging buffer (0x10000)
extern BYTE *g_pSpriteBuffer;
// Mac 00552bc4: sprite outline staging buffer (0x10000)
extern BYTE *g_pSpriteOutlineBuffer;
// Mac 00552bc8: 0x40000 buffer, allocated and freed, never read
extern BYTE *g_pSpriteSpareBuffer;
// Mac 00552bcc: the sprite quad (only [0]'s colour is used)
extern OGLVertex g_aSpriteQuad[4];
// Mac 00552c2c: the global light colour (slot 30 SetGlobalLight)
extern OGLColorRGB g_rgbGlobalLight;
// Mac 00552c30: g_aTileQuad/g_aTileGrid hold normalised UVs, not 32x32 texel UVs
extern bool g_bTileVertsNormalizedUV;
// Mac 00552c34: the flat-path tile quad
extern OGLVertex g_aTileQuad[4];
// Mac 00552c94: the perspective 5-row x 3-column wall/shadow grid
extern OGLVertex g_aTileGrid[5][3];
// Mac 00552dfc: g_aWallCornerTable was converted to 16.16
extern bool g_bWallCornerTableFixed;
// Mac 00552e00: COGLAGPTextures::AcquireSlot round-robin fallback (a function static on the Mac)
extern int g_nAGPRoundRobin;

// Mac 005c8a64: COGLAGPTextures is the texture manager
extern BOOL g_bAGPTextures;
// Mac 005c8a68: textures are GL_TEXTURE_RECTANGLE: texture coordinates are in texels
extern BOOL g_bTextureRectangle;
// Mac 005e9e14: the game's settings (slot 1 arg 1), read live
extern D2GfxSettingsStrc *g_pGfxSettings;
// Mac 005e9e18: the game's software rasterisers (slot 1 arg 2), stored and never read
extern D2GfxHelperStrc *g_pGfxHelpers;
// Mac 005e9e1c: slot 21 UpdateScaleFactor, stored and never read
extern int g_nScaleFactor;
// Mac 005e9e20: &g_aPaletteTable once slot 29 ran, else NULL
extern BYTE **g_pPaletteTable;
// Mac 005e9e24: 640 or 800
extern int g_nScreenWidth;
// Mac 005e9e28: 480 or 600
extern int g_nScreenHeight;
// Mac 005e9e2c: between BeginScene and EndScene1 (SetOption 10 reads it)
extern BOOL g_bInScene;
// Mac 005e9e30: slot 5 paused the surface (nWindowState != 0)
extern BOOL g_bSurfacePaused;
// Mac 005e9e34: zeroed by OGL_OpenOpenGLWindow, never read
extern int g_nRenderStat0;
// Mac 005e9e40: zeroed by OGL_OpenOpenGLWindow, never read
extern int g_nRenderStat3;
// Mac 005e9e4c: zeroed by OGL_OpenOpenGLWindow, never read
extern int g_nRenderStat6;
// Mac 005e9e50: zeroed by OGL_OpenOpenGLWindow, never read
extern int g_nRenderStat7;
// Mac 005e9e54: zeroed by OGL_OpenOpenGLWindow, never read
extern int g_nRenderStat8;
// Mac 005e9edc: the texture manager (COGLTextures or COGLAGPTextures)
extern CD2Textures *g_pTextures;
// Mac 005e9ee0: palette as R,G,B,A bytes (A = 0xff; pure black is 0 = transparent)
extern uint32_t g_aPaletteRGBA[256];
// Mac 005ea2e0: palette as ARGB1555 (pure black is 0)
extern uint16_t g_aPalette1555[256];
// Mac 005ea4e0: palette as R,G,B (written, never read)
extern uint8_t g_aPaletteRGB[256][3];
// Mac 005ea7e0: palette as R,G,B,0 (written, never read)
extern uint8_t g_aPaletteRGBX[256][4];
// Mac 005eabe0: BinkCopyToBuffer surface type chosen by OGL_StartCutscene
extern int g_nBinkSurfaceType;
// Mac 005eabe4: perspective scale, 8.8 fixed (0x100 = 1.0)
extern int g_nPerspectiveFactor;
// Mac 005eabe8: 64 KB [intensity][channel] = intensity * channel / 255
extern BYTE *g_pModulateTable;
// Mac 005eabec: COGLAGPTextures bound a 2D page (written, never read)
extern BOOL g_bBoundPage2D;

#endif
