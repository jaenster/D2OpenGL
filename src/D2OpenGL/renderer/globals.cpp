#include "globals.h"

// Mac 0031c2bf; the Mac strings are full build-machine paths, only the file name is kept
const char g_szFileCD2Textures[] =
    "CD2Textures.cpp";
// Mac 0031e707
const char g_szFileOglBlocks[] = "oglBlocks.cpp";
// Mac 0031ebf9
const char g_szFileOglPerspective[] =
    "oglPerspective.cpp";
// Mac 0031ec53
const char g_szFileOglSmack[] = "oglSmack.cpp";
// Mac 0031eca7
const char g_szFileOglSprite[] = "oglSprite.cpp";
// Mac 0031ecfc
const char g_szFileOglVertex[] = "oglVertex.cpp";
// Mac 0031eda2
const char g_szFileCOGLAGPTextures[] =
    "COGLAGPTextures.cpp";
// Mac Storm operator new/delete 000095fc..000096b0
const char g_szFileStorm[] =
    "STORM.CPP";

// Mac 00334bb4
const BYTE g_abSmackWhite[4] = {0xff, 0xff, 0xff, 0xff};
// Mac 00334bc0
const uint32_t g_adwSmackFillPattern[4] = {0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff};
// Mac 00334be8
const int g_aTileGridStripIndex[2][10] = {
    {0, 1, 3, 4, 6, 7, 9, 10, 12, 13},
    {1, 2, 4, 5, 7, 8, 10, 11, 13, 14},
};
// Mac 00334c40
const int g_anAGPClassWidth[6] = {16, 32, 64, 128, 160, 256};
// Mac 00334c58
const int g_anAGPClassHeight[6] = {16, 32, 64, 128, 80, 256};
// Mac 00334c70
const int g_anAGPClassCount[6] = {512, 2048, 2048, 256, 256, 64};

// Mac 003ce7d0
CutsceneGLCapState g_aCutsceneGLCaps[3] = {
    {0x84F5 /* GL_TEXTURE_RECTANGLE_ARB */, 0, 0, {0, 0}},
    {GL_ALPHA_TEST, 0, 0, {0, 0}},
    {GL_TEXTURE_2D, 0, 1, {0, 0}},
};
// Mac 003ce7e8
int g_nResolution = -1;
// Mac 003ce7ec
int g_nColorBits = 16;
// Mac 003ce7f0
int g_nRendererId = -1;
// Mac 003ce7f4
int g_nGamma = 155;
// Mac 003ce7f8
int g_nContrast = 1;
// Mac 003ce7fc
OGLWallCorner g_aWallCornerTable[10][6] = {
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 5, -40}, {0, 3, -56}, {0, 1, -72}, {0, -1, -88}, {0, -3, -104}, {0, -5, -120}},
    {{-5, 0, -120}, {-3, 0, -104}, {-1, 0, -88}, {1, 0, -72}, {3, 0, -56}, {5, 0, -40}},
    {{0, 5, -40}, {0, 3, -56}, {0, 1, -72}, {1, 0, -72}, {3, 0, -56}, {5, 0, -40}},
    {{-5, 0, -120}, {-3, 0, -104}, {-1, 0, -88}, {0, -1, -88}, {0, -3, -104}, {0, -5, -120}},
    {{0, 5, -40}, {0, 3, -56}, {0, 1, -72}, {1, 0, -72}, {3, 0, -56}, {5, 0, -40}},
    {{5, 10, 40}, {5, 8, 24}, {5, 6, 8}, {5, 4, -8}, {5, 2, -24}, {5, 0, -40}},
    {{0, 5, -40}, {2, 5, -24}, {4, 5, -8}, {6, 5, 8}, {8, 5, 24}, {10, 5, 40}},
    {{0, 5, -40}, {2, 5, -24}, {4, 5, -8}, {5, 4, -8}, {5, 2, -24}, {5, 0, -40}},
    {{5, 10, 40}, {5, 8, 24}, {5, 6, 8}, {6, 5, 8}, {8, 5, 24}, {10, 5, 40}},
};

// Mac 0055256c
int g_nCutsceneScreenWidth;
// Mac 00552570
int g_nCutsceneScreenHeight;
// Mac 00552574
int g_nCutsceneTexWidth;
// Mac 00552578
int g_nCutsceneTexHeight;
// Mac 0055257c
GLuint g_nCutsceneTexture;
// Mac 00552580
bool g_bCutsceneDone;
// Mac 00552584
HBINK g_pBink;
// Mac 00552588
uint32_t g_nBinkDestX;
// Mac 0055258c
uint32_t g_nBinkDestY;
// Mac 00552590
BYTE *g_pWallBlockBuffer;
// Mac 00552594
BYTE *g_pBlockScratch;
// Mac 00552598
BYTE *g_pFloorTileBuffer;
// Mac 0055259c
bool g_bInCutscene;
// Mac 005525a0
int g_nCurTextureMode;
// Mac 005525a4
int g_nCurBlendMode;
// Mac 005525a8
D2PaletteTable g_aPaletteTable;
// Mac 005526c8
bool g_bAtExitReleased;
// Mac 005526cc
bool g_bGroundMeshDirty;
// Mac 005526d0
OGLVertex g_aGroundVerts[6][6];
// Mac 00552a30
OGLVertex *g_apGroundStrip[5][12];
// Mac 00552b20
bool g_bAcceleratedRenderer;
// Mac 00552b24
int g_bFullscreen;
// Mac 00552b28
BOOL g_bDisplayModeChanged;
// Mac 00552b2c
HMONITOR g_hDisplay;
// Mac 00552b30
HWND g_hWnd;
// Mac 00552b34
HGLRC g_hGLRC;
// Windows only: no Mac counterpart
HDC g_hDC;
// Mac 00552b38
POINT g_ptWindowOrigin;
// Mac 00552b3c
bool g_bCutsceneActive;
// Mac 00552b40
BYTE *g_pCutsceneBuffer;
// Mac 00552b50
OGLDisplayModeRec g_aDisplayModes[3];
// Mac 00552b70
int *g_pPerspectiveTable;
// Mac 00552b74
int g_nPerspectiveCenterX;
// Mac 00552b78
int g_nPerspectiveCenterY;
// Mac 00552b7c
BYTE *g_pSmackIndexBuf;
// Mac 00552b80
uint32_t *g_apSmackPixels[2];
// Mac 00552b88
GLuint g_aSmackTextures[2];
// Mac 00552b90
OGLSmackVertex g_aSmackQuad[4];
// Mac 00552bc0
BYTE *g_pSpriteBuffer;
// Mac 00552bc4
BYTE *g_pSpriteOutlineBuffer;
// Mac 00552bc8
BYTE *g_pSpriteSpareBuffer;
// Mac 00552bcc
OGLVertex g_aSpriteQuad[4];
// Mac 00552c2c
OGLColorRGB g_rgbGlobalLight;
// Mac 00552c30
bool g_bTileVertsNormalizedUV;
// Mac 00552c34
OGLVertex g_aTileQuad[4];
// Mac 00552c94
OGLVertex g_aTileGrid[5][3];
// Mac 00552dfc
bool g_bWallCornerTableFixed;
// Mac 00552e00
int g_nAGPRoundRobin;

// Mac 005c8a64
BOOL g_bAGPTextures;
// Mac 005c8a68
BOOL g_bTextureRectangle;
// Mac 005e9e14
D2GfxSettingsStrc *g_pGfxSettings;
// Mac 005e9e18
D2GfxHelperStrc *g_pGfxHelpers;
// Mac 005e9e1c
int g_nScaleFactor;
// Mac 005e9e20
BYTE **g_pPaletteTable;
// Mac 005e9e24
int g_nScreenWidth;
// Mac 005e9e28
int g_nScreenHeight;
// Mac 005e9e2c
BOOL g_bInScene;
// Mac 005e9e30
BOOL g_bSurfacePaused;
// Mac 005e9e34
int g_nRenderStat0;
// Mac 005e9e40
int g_nRenderStat3;
// Mac 005e9e4c
int g_nRenderStat6;
// Mac 005e9e50
int g_nRenderStat7;
// Mac 005e9e54
int g_nRenderStat8;
// Mac 005e9edc
CD2Textures *g_pTextures;
// Mac 005e9ee0
uint32_t g_aPaletteRGBA[256];
// Mac 005ea2e0
uint16_t g_aPalette1555[256];
// Mac 005ea4e0
uint8_t g_aPaletteRGB[256][3];
// Mac 005ea7e0
uint8_t g_aPaletteRGBX[256][4];
// Mac 005eabe0
int g_nBinkSurfaceType;
// Mac 005eabe4
int g_nPerspectiveFactor;
// Mac 005eabe8
BYTE *g_pModulateTable;
// Mac 005eabec
BOOL g_bBoundPage2D;
