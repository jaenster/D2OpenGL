#ifndef D2OPENGL_RENDERER_TYPES_H
#define D2OPENGL_RENDERER_TYPES_H

// The renderer's types. Two kinds live here:
//
// - Boundary structs, which the game owns and hands to the renderer (slot arguments, D2CMP results).
//   They follow the Windows 1.14d layout, checked against Game.exe and its four renderers, with a
//   static_assert on the size and on every field offset. Where the recovered struct is short or
//   mistyped, the layout follows the code that reads it.
// - Renderer-private structs recovered from the Mac code (vertices, sprite records, texture nodes).
//   They never cross into the game, but the recovered code indexes them by offset, so they keep the
//   Mac layout and are asserted too.
//
// The texture classes are in textures.h.

#include <windows.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <GL/gl.h>

#define OGL_FASTCALL __attribute__((fastcall))

// ------------------------------------------------------------------------------------------------
// Enums

enum DRAWMODE_values {
    DRAW_VERYTRANSPARENT = 0x0,
    DRAW_TRANSPARENT = 0x1,
    DRAW_LESSTRANSPARENT = 0x2,
    DRAW_LIGHTTRANSPARENT = 0x3,
    DRAW_DARKTRANSPARENT = 0x4,
    DRAW_SOLID = 0x5,
    DRAW_DARKERSOLID = 0x6,
    DRAW_LIGHTERSOLID = 0x7,
};
typedef uint32_t DRAWMODE;

enum GLOBALPALLETE_values {
    PALETTE_RED = 0x1,
    PALETTE_GREEN = 0x2,
    PALETTE_BLUE = 0x3,
    PALETTE_GOLD = 0x4,
    PALETTE_DARKGREY = 0x5,
    PALETTE_LIGHTBLACK = 0x6,
    PALETTE_LIGHTGOLD = 0x7,
    PALETTE_ORANGE = 0x8,
    PALETTE_LIGHTYELLOW = 0x9,
    PALETTE_DARKGREEN = 0xa,
    PALETTE_PURPLE = 0xb,
    PALETTE_GREEN2 = 0xc,
    PALETTE_LIGHTGOLD2 = 0xd,
    PALETTE_BLACK = 0xe,
};
typedef uint32_t GLOBALPALLETE;

enum eD2UnitType_values {
    UNIT_PLAYER = 0x0,
    UNIT_MONSTER = 0x1,
    UNIT_OBJECT = 0x2,
    UNIT_MISSILE = 0x3,
    UNIT_ITEM = 0x4,
    UNIT_WARP = 0x5,
    UNIT_COUNT = 0x6,
};
typedef uint32_t eD2UnitType;

enum eD2RendererMode_values {
    RENDERER_Unused0 = 0x0,
    RENDERER_Windowed = 0x1,
    RENDERER_Unused2 = 0x2,
    RENDERED_DirectDraw = 0x3,
    RENDERER_Glide = 0x4,
    RENDERED_Unused5 = 0x5,
    RENDERER_Direct3D = 0x6,
    RENDERER_COUNT = 0x7,
};
typedef uint32_t eD2RendererMode;

// Renderer state caches (OGL_SetTextureMode 002dfc65, OGL_SetBlendMode 002dfcf5, OGL_SetTextureFilter
// 002dfdb0). 0 means "unknown": OGL_BeginScene resets both caches to it so the next call applies.
enum OGLTextureMode {
    OGL_TEXMODE_UNSET = 0,
    OGL_TEXMODE_TEXTURED = 1,       // enable GL_TEXTURE_2D, GL_ALPHA_TEST (and the rectangle target)
    OGL_TEXMODE_UNTEXTURED = 2,     // disable them
};

enum OGLBlendMode {
    OGL_BLEND_UNSET = 0,
    OGL_BLEND_OFF = 1,              // glDisable(GL_BLEND)
    OGL_BLEND_ADD = 2,              // GL_ONE, GL_ONE
    OGL_BLEND_MUL = 3,              // GL_ZERO, GL_SRC_COLOR
    OGL_BLEND_ALPHA = 4,            // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
};

enum OGLTextureFilter {
    OGL_FILTER_NEAREST = 1,
    OGL_FILTER_LINEAR = 2,
};

// Who owns a texture node's back-reference (CD2Textures::SetNodeOwner 002b13b8).
enum OGLTextureOwner {
    TEXOWNER_NONE = 0,
    TEXOWNER_TILE = 1,              // short at D2TileLibraryEntryStrc+0x42
    TEXOWNER_BLOCK = 2,             // short at D2TileLibraryBlockStrc+0x04
    TEXOWNER_SPRITE = 3,            // int at OGLSpriteTexSlot[nOwnerIndex].nTextureId (owner+4+idx*8)
};

#pragma pack(push, 1)

// ================================================================================================
// Boundary structs: Windows 1.14d layout.

// Game.exe D2GfxSettingsStrc, instance D2GFXRenderedData 0072da48 (defaults 0,1,0,0x9b,1,1). Slot 1
// arg 1. The renderer keeps the pointer and reads it live: D2gfx rewrites it at runtime.
struct D2GfxSettingsStrc {
    BOOL bPerspectiveEnabled;   // +0x00
    BOOL bPerspectiveCapable;   // +0x04  D2gfx zeroes it for mode < 4
    BOOL bLowQuality;           // +0x08
    int nGamma;                 // +0x0C  not read by the renderer
    BOOL bVSync;                // +0x10  not read by the renderer
    BOOL bBlendedShadows;       // +0x14
};
static_assert(sizeof(D2GfxSettingsStrc) == 0x18, "D2GfxSettingsStrc");
static_assert(offsetof(D2GfxSettingsStrc, bPerspectiveEnabled) == 0x00, "D2GfxSettingsStrc.bPerspectiveEnabled");
static_assert(offsetof(D2GfxSettingsStrc, bPerspectiveCapable) == 0x04, "D2GfxSettingsStrc.bPerspectiveCapable");
static_assert(offsetof(D2GfxSettingsStrc, bLowQuality) == 0x08, "D2GfxSettingsStrc.bLowQuality");
static_assert(offsetof(D2GfxSettingsStrc, nGamma) == 0x0C, "D2GfxSettingsStrc.nGamma");
static_assert(offsetof(D2GfxSettingsStrc, bVSync) == 0x10, "D2GfxSettingsStrc.bVSync");
static_assert(offsetof(D2GfxSettingsStrc, bBlendedShadows) == 0x14, "D2GfxSettingsStrc.bBlendedShadows");

struct D2TileLibraryEntryStrc;
struct D2GfxLightExStrc;

// Game.exe D2GfxHelperStrc, instance 0072da60 = {004f8de0, 004f7ea0, 004f8050, 004f8b80, 004f8120,
// 004f82d0, 004f84f0}: D2gfx's software rasterisers, all __fastcall (arity from RET n). Slot 1 arg 2.
// The renderer stores the pointer and never calls through it.
typedef void(OGL_FASTCALL *D2GfxHelper_FillYBufferTable)(void *ppvBits, int nWidth, int nHeight, int a4);
typedef void(OGL_FASTCALL *D2GfxHelper_DrawVisTile)(BYTE *pbTileData, int nScreenX, int nScreenY);
typedef void(OGL_FASTCALL *D2GfxHelper_DrawVisTileLit)(BYTE *pbTileData, int nScreenX, int nScreenY,
                                                      int nLightLevel);
typedef BOOL(OGL_FASTCALL *D2GfxHelper_DrawGroundTile)(D2TileLibraryEntryStrc *pTile, int nScreenX,
                                                      int nScreenY, D2GfxLightExStrc *pLightGrid);
typedef void(OGL_FASTCALL *D2GfxHelper_DrawWallTile)(BYTE *pbTileData, int nScreenX, int nScreenY,
                                                    DWORD *pLightCorners);
typedef void(OGL_FASTCALL *D2GfxHelper_DrawBlendedVisTile)(BYTE *pbTileData, int nScreenX, int nScreenY,
                                                          int nAlpha);
typedef void(OGL_FASTCALL *D2GfxHelper_DrawRoofTile)(BYTE *pbTileData, int nScreenX, int nScreenY,
                                                    DWORD *pLightCorners, int nAlpha);

struct D2GfxHelperStrc {
    D2GfxHelper_FillYBufferTable nfpFillYBufferTable;       // +0x00  004f8de0
    D2GfxHelper_DrawVisTile nfpDrawVisTile;                 // +0x04  004f7ea0
    D2GfxHelper_DrawVisTileLit nfpDrawVisTileLit;           // +0x08  004f8050
    D2GfxHelper_DrawGroundTile nfpDrawGroundTile;           // +0x0C  004f8b80
    D2GfxHelper_DrawWallTile nfpDrawWallTile;               // +0x10  004f8120
    D2GfxHelper_DrawBlendedVisTile nfpDrawBlendedVisTile;   // +0x14  004f82d0
    D2GfxHelper_DrawRoofTile nfpDrawRoofTile;               // +0x18  004f84f0
};
static_assert(sizeof(D2GfxHelperStrc) == 0x1c, "D2GfxHelperStrc");
static_assert(offsetof(D2GfxHelperStrc, nfpFillYBufferTable) == 0x00, "D2GfxHelperStrc.nfpFillYBufferTable");
static_assert(offsetof(D2GfxHelperStrc, nfpDrawVisTile) == 0x04, "D2GfxHelperStrc.nfpDrawVisTile");
static_assert(offsetof(D2GfxHelperStrc, nfpDrawVisTileLit) == 0x08, "D2GfxHelperStrc.nfpDrawVisTileLit");
static_assert(offsetof(D2GfxHelperStrc, nfpDrawGroundTile) == 0x0C, "D2GfxHelperStrc.nfpDrawGroundTile");
static_assert(offsetof(D2GfxHelperStrc, nfpDrawWallTile) == 0x10, "D2GfxHelperStrc.nfpDrawWallTile");
static_assert(offsetof(D2GfxHelperStrc, nfpDrawBlendedVisTile) == 0x14, "D2GfxHelperStrc.nfpDrawBlendedVisTile");
static_assert(offsetof(D2GfxHelperStrc, nfpDrawRoofTile) == 0x18, "D2GfxHelperStrc.nfpDrawRoofTile");

// Game.exe D2GfxLightStrc. The wall slots pass an array, indexed by block.x >> 5 and that + 1 (at
// least 6 entries). A DWORD "dwGamma" in the image slots carries the same four bytes packed.
struct D2GfxLightStrc {
    BYTE nIntensity;    // +0x00
    BYTE nRed;          // +0x01
    BYTE nGreen;        // +0x02
    BYTE nBlue;         // +0x03
};
static_assert(sizeof(D2GfxLightStrc) == 0x4, "D2GfxLightStrc");
static_assert(offsetof(D2GfxLightStrc, nIntensity) == 0x0, "D2GfxLightStrc.nIntensity");
static_assert(offsetof(D2GfxLightStrc, nRed) == 0x1, "D2GfxLightStrc.nRed");
static_assert(offsetof(D2GfxLightStrc, nGreen) == 0x2, "D2GfxLightStrc.nGreen");
static_assert(offsetof(D2GfxLightStrc, nBlue) == 0x3, "D2GfxLightStrc.nBlue");

// Game.exe D2GfxLightExStrc. DrawGroundTile's pLight is an array of 36 (6x6 vertex grid, row-major),
// built by D2Client DrawFloorTile 004de410 when the engine mode is >= 4; nX/nY are screen positions.
struct D2GfxLightExStrc {
    D2GfxLightStrc sLight;  // +0x00
    int nX;                 // +0x04
    int nY;                 // +0x08
};
static_assert(sizeof(D2GfxLightExStrc) == 0xc, "D2GfxLightExStrc");
static_assert(offsetof(D2GfxLightExStrc, sLight) == 0x0, "D2GfxLightExStrc.sLight");
static_assert(offsetof(D2GfxLightExStrc, nX) == 0x4, "D2GfxLightExStrc.nX");
static_assert(offsetof(D2GfxLightExStrc, nY) == 0x8, "D2GfxLightExStrc.nY");

// Game.exe DC6Block (D2CMP). Read only through the D2CMP accessors (g_host.GetDC6Width & co).
struct DC6Block {
    int nFlip;          // +0x00
    int nWidth;         // +0x04
    int nHeight;        // +0x08
    int nOffsetX;       // +0x0C
    int nOffsetY;       // +0x10
    int nAllocSize;     // +0x14
    int nNextBlock;     // +0x18  really the renderer cache pointer: D2CMP points it at a zeroed 0x2c-byte
                        //        slot; g_host.GetDC6BlockPixelData returns it (OGLSpriteTexSlot[5])
    int nLength;        // +0x1C
    // BYTE data[];     // +0x20
};
static_assert(sizeof(DC6Block) == 0x20, "DC6Block");
static_assert(offsetof(DC6Block, nFlip) == 0x00, "DC6Block.nFlip");
static_assert(offsetof(DC6Block, nWidth) == 0x04, "DC6Block.nWidth");
static_assert(offsetof(DC6Block, nHeight) == 0x08, "DC6Block.nHeight");
static_assert(offsetof(DC6Block, nOffsetX) == 0x0C, "DC6Block.nOffsetX");
static_assert(offsetof(DC6Block, nOffsetY) == 0x10, "DC6Block.nOffsetY");
static_assert(offsetof(DC6Block, nAllocSize) == 0x14, "DC6Block.nAllocSize");
static_assert(offsetof(DC6Block, nNextBlock) == 0x18, "DC6Block.nNextBlock");
static_assert(offsetof(DC6Block, nLength) == 0x1C, "DC6Block.nLength");

// Game.exe DC6 (D2CMP cel file, 1.14d form). A DC6Block* array of nDirections * nFrames follows.
struct DC6 {
    int nVersion;       // +0x00  6
    int nFlags;         // +0x04
    int nFormat;        // +0x08
    char nTerm[4];      // +0x0C
    int nDirections;    // +0x10
    int nFrames;        // +0x14
    // DC6Block *pBlocks[];  +0x18
};
static_assert(sizeof(DC6) == 0x18, "DC6");
static_assert(offsetof(DC6, nVersion) == 0x00, "DC6.nVersion");
static_assert(offsetof(DC6, nFlags) == 0x04, "DC6.nFlags");
static_assert(offsetof(DC6, nFormat) == 0x08, "DC6.nFormat");
static_assert(offsetof(DC6, nTerm) == 0x0C, "DC6.nTerm");
static_assert(offsetof(DC6, nDirections) == 0x10, "DC6.nDirections");
static_assert(offsetof(DC6, nFrames) == 0x14, "DC6.nFrames");

// Game.exe D2GfxDataStrc. The renderer reads only +0x3c, after g_host.GetOrLoadSprite filled it.
struct D2GfxDataStrc {
    int nFrameNumber;           // +0x00
    uint8_t dwFlags;            // +0x04
    uint8_t nComponentType;     // +0x05
    uint8_t pad_0x6[2];         // +0x06
    int eUnitType;              // +0x08  eD2UnitType
    int nClassId;               // +0x0C
    int nMode;                  // +0x10
    int nOverlayId;             // +0x14
    int nUnitToken;             // +0x18
    int nCompositToken;         // +0x1C
    int nArmorToken;            // +0x20
    int nModeToken;             // +0x24
    int nWeaponClass;           // +0x28
    char *szFilename;           // +0x2C
    uint8_t _pad_0x30[4];       // +0x30
    DC6 *pDC6;                  // +0x34
    int bLoaded;                // +0x38
    DC6Block *pDC6Block;        // +0x3C
    int nPaletteShift;          // +0x40  used as the direction index by D2CMP 00601840
    uint32_t dwRenderFlags;     // +0x44
};
static_assert(sizeof(D2GfxDataStrc) == 0x48, "D2GfxDataStrc");
static_assert(offsetof(D2GfxDataStrc, nFrameNumber) == 0x00, "D2GfxDataStrc.nFrameNumber");
static_assert(offsetof(D2GfxDataStrc, dwFlags) == 0x04, "D2GfxDataStrc.dwFlags");
static_assert(offsetof(D2GfxDataStrc, nComponentType) == 0x05, "D2GfxDataStrc.nComponentType");
static_assert(offsetof(D2GfxDataStrc, pad_0x6) == 0x06, "D2GfxDataStrc.pad_0x6");
static_assert(offsetof(D2GfxDataStrc, eUnitType) == 0x08, "D2GfxDataStrc.eUnitType");
static_assert(offsetof(D2GfxDataStrc, nClassId) == 0x0C, "D2GfxDataStrc.nClassId");
static_assert(offsetof(D2GfxDataStrc, nMode) == 0x10, "D2GfxDataStrc.nMode");
static_assert(offsetof(D2GfxDataStrc, nOverlayId) == 0x14, "D2GfxDataStrc.nOverlayId");
static_assert(offsetof(D2GfxDataStrc, nUnitToken) == 0x18, "D2GfxDataStrc.nUnitToken");
static_assert(offsetof(D2GfxDataStrc, nCompositToken) == 0x1C, "D2GfxDataStrc.nCompositToken");
static_assert(offsetof(D2GfxDataStrc, nArmorToken) == 0x20, "D2GfxDataStrc.nArmorToken");
static_assert(offsetof(D2GfxDataStrc, nModeToken) == 0x24, "D2GfxDataStrc.nModeToken");
static_assert(offsetof(D2GfxDataStrc, nWeaponClass) == 0x28, "D2GfxDataStrc.nWeaponClass");
static_assert(offsetof(D2GfxDataStrc, szFilename) == 0x2C, "D2GfxDataStrc.szFilename");
static_assert(offsetof(D2GfxDataStrc, _pad_0x30) == 0x30, "D2GfxDataStrc._pad_0x30");
static_assert(offsetof(D2GfxDataStrc, pDC6) == 0x34, "D2GfxDataStrc.pDC6");
static_assert(offsetof(D2GfxDataStrc, bLoaded) == 0x38, "D2GfxDataStrc.bLoaded");
static_assert(offsetof(D2GfxDataStrc, pDC6Block) == 0x3C, "D2GfxDataStrc.pDC6Block");
static_assert(offsetof(D2GfxDataStrc, nPaletteShift) == 0x40, "D2GfxDataStrc.nPaletteShift");
static_assert(offsetof(D2GfxDataStrc, dwRenderFlags) == 0x44, "D2GfxDataStrc.dwRenderFlags");

// Game.exe D2GfxTileRectStrc: what DrawClippedImage's `void* pCropRect` (slot 38) points at. Not a
// RECT: the order is left, right, top, bottom (DDraw 00511e80, Glide 0050b390, Mac 002e40fc agree).
struct D2GfxTileRectStrc {
    int nLeft;      // +0x00
    int nRight;     // +0x04  clamped to the screen width
    int nTop;       // +0x08
    int nBottom;    // +0x0C  clamped to the screen height
};
static_assert(sizeof(D2GfxTileRectStrc) == 0x10, "D2GfxTileRectStrc");
static_assert(offsetof(D2GfxTileRectStrc, nLeft) == 0x0, "D2GfxTileRectStrc.nLeft");
static_assert(offsetof(D2GfxTileRectStrc, nRight) == 0x4, "D2GfxTileRectStrc.nRight");
static_assert(offsetof(D2GfxTileRectStrc, nTop) == 0x8, "D2GfxTileRectStrc.nTop");
static_assert(offsetof(D2GfxTileRectStrc, nBottom) == 0xC, "D2GfxTileRectStrc.nBottom");

// One 0x14-byte sub-tile block in the array D2TileLibraryEntryStrc+0x54 points at (also known
// as D2GroundSubTileStrc). +4 is a renderer-owned texture slot inside the game's struct.
struct D2TileLibraryBlockStrc {
    int16_t nPosX;              // +0x00
    int16_t nPosY;              // +0x02
    int16_t nTextureIndex;      // +0x04  renderer texture id (0 = none): Glide 0050fbd3, Mac 002df213
    uint8_t nGridX;             // +0x06
    uint8_t nGridY;             // +0x07
    uint16_t nFormat;           // +0x08  bit0 = draw it; bit2 = RLE, else a 15-row isometric diamond
    uint8_t field_0A[6];        // +0x0A  length (unaligned int) and an unknown short
    BYTE *pPixelData;           // +0x10
};
static_assert(sizeof(D2TileLibraryBlockStrc) == 0x14, "D2TileLibraryBlockStrc");
static_assert(offsetof(D2TileLibraryBlockStrc, nPosX) == 0x00, "D2TileLibraryBlockStrc.nPosX");
static_assert(offsetof(D2TileLibraryBlockStrc, nPosY) == 0x02, "D2TileLibraryBlockStrc.nPosY");
static_assert(offsetof(D2TileLibraryBlockStrc, nTextureIndex) == 0x04, "D2TileLibraryBlockStrc.nTextureIndex");
static_assert(offsetof(D2TileLibraryBlockStrc, nGridX) == 0x06, "D2TileLibraryBlockStrc.nGridX");
static_assert(offsetof(D2TileLibraryBlockStrc, nGridY) == 0x07, "D2TileLibraryBlockStrc.nGridY");
static_assert(offsetof(D2TileLibraryBlockStrc, nFormat) == 0x08, "D2TileLibraryBlockStrc.nFormat");
static_assert(offsetof(D2TileLibraryBlockStrc, field_0A) == 0x0A, "D2TileLibraryBlockStrc.field_0A");
static_assert(offsetof(D2TileLibraryBlockStrc, pPixelData) == 0x10, "D2TileLibraryBlockStrc.pPixelData");

struct D2TileRecordStrc;

// Game.exe D2TileLibraryEntryStrc. Ghidra types it 0x50; the object is 0x60 (D2CMP allocates
// nTiles * 0x60, every renderer reads +0x50/+0x54). +0x50..+0x5c are valid only after
// g_host.AllocTileEntry(pTile, 1, 0) returned non-zero.
struct D2TileLibraryEntryStrc {
    int nDirection;                     // +0x00  wall direction 0..9 (perspective wall path)
    short nRoofHeight;                  // +0x04
    short nFlags;                       // +0x06
    int nHeight;                        // +0x08
    int nWidth;                         // +0x0C
    int nHeightToBottom;                // +0x10
    int nOrientation;                   // +0x14
    int nIndex;                         // +0x18
    int nSubIndex;                      // +0x1C
    int nFrame_Rarity;                  // +0x20
    int transparentColorRGB24;          // +0x24
    BYTE dwTileFlags[4];                // +0x28
    int dwBlockOffset_pBlock;           // +0x2C
    int nBlockSize;                     // +0x30
    int nBlocks;                        // +0x34  file header count; the renderer uses nSubtileCount
    D2TileRecordStrc *pParent;          // +0x38
    short field_0x3c;                   // +0x3C
    short nCacheIndex;                  // +0x3E
    short field_0x40;                   // +0x40
    short nTextureCacheIndex;           // +0x42  renderer floor-texture id (0 = none)
    int field_0x44;                     // +0x44
    int dwBlockDataOffset;              // +0x48
    int nBlockDataSize;                 // +0x4C
    int nSubtileCount;                  // +0x50
    D2TileLibraryBlockStrc *pSubtiles;  // +0x54
    char *szFilename;                   // +0x58
    void *pCacheNode;                   // +0x5C
};
static_assert(sizeof(D2TileLibraryEntryStrc) == 0x60, "D2TileLibraryEntryStrc");
static_assert(offsetof(D2TileLibraryEntryStrc, nDirection) == 0x00, "D2TileLibraryEntryStrc.nDirection");
static_assert(offsetof(D2TileLibraryEntryStrc, nRoofHeight) == 0x04, "D2TileLibraryEntryStrc.nRoofHeight");
static_assert(offsetof(D2TileLibraryEntryStrc, nFlags) == 0x06, "D2TileLibraryEntryStrc.nFlags");
static_assert(offsetof(D2TileLibraryEntryStrc, nHeight) == 0x08, "D2TileLibraryEntryStrc.nHeight");
static_assert(offsetof(D2TileLibraryEntryStrc, nWidth) == 0x0C, "D2TileLibraryEntryStrc.nWidth");
static_assert(offsetof(D2TileLibraryEntryStrc, nHeightToBottom) == 0x10, "D2TileLibraryEntryStrc.nHeightToBottom");
static_assert(offsetof(D2TileLibraryEntryStrc, nOrientation) == 0x14, "D2TileLibraryEntryStrc.nOrientation");
static_assert(offsetof(D2TileLibraryEntryStrc, nIndex) == 0x18, "D2TileLibraryEntryStrc.nIndex");
static_assert(offsetof(D2TileLibraryEntryStrc, nSubIndex) == 0x1C, "D2TileLibraryEntryStrc.nSubIndex");
static_assert(offsetof(D2TileLibraryEntryStrc, nFrame_Rarity) == 0x20, "D2TileLibraryEntryStrc.nFrame_Rarity");
static_assert(offsetof(D2TileLibraryEntryStrc, transparentColorRGB24) == 0x24, "D2TileLibraryEntryStrc.transparentColorRGB24");
static_assert(offsetof(D2TileLibraryEntryStrc, dwTileFlags) == 0x28, "D2TileLibraryEntryStrc.dwTileFlags");
static_assert(offsetof(D2TileLibraryEntryStrc, dwBlockOffset_pBlock) == 0x2C, "D2TileLibraryEntryStrc.dwBlockOffset_pBlock");
static_assert(offsetof(D2TileLibraryEntryStrc, nBlockSize) == 0x30, "D2TileLibraryEntryStrc.nBlockSize");
static_assert(offsetof(D2TileLibraryEntryStrc, nBlocks) == 0x34, "D2TileLibraryEntryStrc.nBlocks");
static_assert(offsetof(D2TileLibraryEntryStrc, pParent) == 0x38, "D2TileLibraryEntryStrc.pParent");
static_assert(offsetof(D2TileLibraryEntryStrc, field_0x3c) == 0x3C, "D2TileLibraryEntryStrc.field_0x3c");
static_assert(offsetof(D2TileLibraryEntryStrc, nCacheIndex) == 0x3E, "D2TileLibraryEntryStrc.nCacheIndex");
static_assert(offsetof(D2TileLibraryEntryStrc, field_0x40) == 0x40, "D2TileLibraryEntryStrc.field_0x40");
static_assert(offsetof(D2TileLibraryEntryStrc, nTextureCacheIndex) == 0x42, "D2TileLibraryEntryStrc.nTextureCacheIndex");
static_assert(offsetof(D2TileLibraryEntryStrc, field_0x44) == 0x44, "D2TileLibraryEntryStrc.field_0x44");
static_assert(offsetof(D2TileLibraryEntryStrc, dwBlockDataOffset) == 0x48, "D2TileLibraryEntryStrc.dwBlockDataOffset");
static_assert(offsetof(D2TileLibraryEntryStrc, nBlockDataSize) == 0x4C, "D2TileLibraryEntryStrc.nBlockDataSize");
static_assert(offsetof(D2TileLibraryEntryStrc, nSubtileCount) == 0x50, "D2TileLibraryEntryStrc.nSubtileCount");
static_assert(offsetof(D2TileLibraryEntryStrc, pSubtiles) == 0x54, "D2TileLibraryEntryStrc.pSubtiles");
static_assert(offsetof(D2TileLibraryEntryStrc, szFilename) == 0x58, "D2TileLibraryEntryStrc.szFilename");
static_assert(offsetof(D2TileLibraryEntryStrc, pCacheNode) == 0x5C, "D2TileLibraryEntryStrc.pCacheNode");

// Game.exe D2TileLibraryHashRefStrc / HashNodeStrc / HashStrc / D2TileRecordStrc. Not read by the
// renderer; kept because D2TileLibraryEntryStrc points at them.
struct D2TileLibraryHashRefStrc {
    D2TileLibraryEntryStrc *pTile;      // +0x00
    D2TileLibraryHashRefStrc *pPrev;    // +0x04
};
static_assert(sizeof(D2TileLibraryHashRefStrc) == 0x8, "D2TileLibraryHashRefStrc");
static_assert(offsetof(D2TileLibraryHashRefStrc, pTile) == 0x0, "D2TileLibraryHashRefStrc.pTile");
static_assert(offsetof(D2TileLibraryHashRefStrc, pPrev) == 0x4, "D2TileLibraryHashRefStrc.pPrev");

struct D2TileLibraryHashNodeStrc {
    int nIndex;                         // +0x00
    int nSubIndex;                      // +0x04
    int nOrientation;                   // +0x08
    D2TileLibraryHashRefStrc *pRef;     // +0x0C
    D2TileLibraryHashNodeStrc *pPrev;   // +0x10
};
static_assert(sizeof(D2TileLibraryHashNodeStrc) == 0x14, "D2TileLibraryHashNodeStrc");
static_assert(offsetof(D2TileLibraryHashNodeStrc, nIndex) == 0x00, "D2TileLibraryHashNodeStrc.nIndex");
static_assert(offsetof(D2TileLibraryHashNodeStrc, nSubIndex) == 0x04, "D2TileLibraryHashNodeStrc.nSubIndex");
static_assert(offsetof(D2TileLibraryHashNodeStrc, nOrientation) == 0x08, "D2TileLibraryHashNodeStrc.nOrientation");
static_assert(offsetof(D2TileLibraryHashNodeStrc, pRef) == 0x0C, "D2TileLibraryHashNodeStrc.pRef");
static_assert(offsetof(D2TileLibraryHashNodeStrc, pPrev) == 0x10, "D2TileLibraryHashNodeStrc.pPrev");

struct D2TileLibraryHashStrc {
    D2TileLibraryHashNodeStrc *pNodes[128];     // +0x000
};
static_assert(sizeof(D2TileLibraryHashStrc) == 0x200, "D2TileLibraryHashStrc");
static_assert(offsetof(D2TileLibraryHashStrc, pNodes) == 0x0, "D2TileLibraryHashStrc.pNodes");

struct D2TileRecordStrc {
    char szLibraryName[260];                // +0x000
    void *pLibrary;                         // +0x104
    D2TileLibraryHashStrc *pHashBlock;      // +0x108
    D2TileRecordStrc *pPrev;                // +0x10C
};
static_assert(sizeof(D2TileRecordStrc) == 0x110, "D2TileRecordStrc");
static_assert(offsetof(D2TileRecordStrc, szLibraryName) == 0x000, "D2TileRecordStrc.szLibraryName");
static_assert(offsetof(D2TileRecordStrc, pLibrary) == 0x104, "D2TileRecordStrc.pLibrary");
static_assert(offsetof(D2TileRecordStrc, pHashBlock) == 0x108, "D2TileRecordStrc.pHashBlock");
static_assert(offsetof(D2TileRecordStrc, pPrev) == 0x10C, "D2TileRecordStrc.pPrev");

// Game.exe D2SmackerContextStrc (D2Win D2WinSmack+0x140). The Mac renderer never opens hSmack: its
// 's' path is a stub and only the 'p' (PCX banner) path draws.
struct D2SmackerContextStrc {
    void *hSmack;       // +0x00  RAD Smack*, only tested and cleared by the Mac code
    char eMode;         // +0x04  's' smk video, 'p' pcx still, 0 none
    uint8_t nPad[3];    // +0x05
    int nX;             // +0x08
    int nY;             // +0x0C  the banner's bottom edge in D2 screen space
};
static_assert(sizeof(D2SmackerContextStrc) == 0x10, "D2SmackerContextStrc");
static_assert(offsetof(D2SmackerContextStrc, hSmack) == 0x0, "D2SmackerContextStrc.hSmack");
static_assert(offsetof(D2SmackerContextStrc, eMode) == 0x4, "D2SmackerContextStrc.eMode");
static_assert(offsetof(D2SmackerContextStrc, nPad) == 0x5, "D2SmackerContextStrc.nPad");
static_assert(offsetof(D2SmackerContextStrc, nX) == 0x8, "D2SmackerContextStrc.nX");
static_assert(offsetof(D2SmackerContextStrc, nY) == 0xC, "D2SmackerContextStrc.nY");

// What slot 19 GetRenderStatistics may return (Glide/D3D &0x7c90c8, 27 dwords). DDraw, Gdi and the
// Mac return NULL, which D2Client handles.
struct D2RenderStatisticsStrc {
    DWORD dwFloorCacheBytes;                    // +0x00
    DWORD dwFloorCacheStat1;                    // +0x04
    DWORD dwFloorCacheMisses;                   // +0x08
    DWORD dwSubtileCacheBytes;                  // +0x0C
    DWORD dwSubtileCacheStat1;                  // +0x10
    DWORD dwSubtileCacheMisses;                 // +0x14
    DWORD dwSpriteCacheBytes[3];                // +0x18
    DWORD dwSpriteCacheStat1[3];                // +0x24
    DWORD dwSpriteCacheMisses[3];               // +0x30
    DWORD dwSpriteCacheStat2[3];                // +0x3C
    DWORD dwSpriteCacheAccessesPerFrame[3];     // +0x48
    DWORD dwSpriteCacheStat3[3];                // +0x54
    DWORD dwSpriteCacheStat4[3];                // +0x60
};
static_assert(sizeof(D2RenderStatisticsStrc) == 0x6c, "D2RenderStatisticsStrc");
static_assert(offsetof(D2RenderStatisticsStrc, dwFloorCacheBytes) == 0x00, "D2RenderStatisticsStrc.dwFloorCacheBytes");
static_assert(offsetof(D2RenderStatisticsStrc, dwFloorCacheStat1) == 0x04, "D2RenderStatisticsStrc.dwFloorCacheStat1");
static_assert(offsetof(D2RenderStatisticsStrc, dwFloorCacheMisses) == 0x08, "D2RenderStatisticsStrc.dwFloorCacheMisses");
static_assert(offsetof(D2RenderStatisticsStrc, dwSubtileCacheBytes) == 0x0C, "D2RenderStatisticsStrc.dwSubtileCacheBytes");
static_assert(offsetof(D2RenderStatisticsStrc, dwSubtileCacheStat1) == 0x10, "D2RenderStatisticsStrc.dwSubtileCacheStat1");
static_assert(offsetof(D2RenderStatisticsStrc, dwSubtileCacheMisses) == 0x14, "D2RenderStatisticsStrc.dwSubtileCacheMisses");
static_assert(offsetof(D2RenderStatisticsStrc, dwSpriteCacheBytes) == 0x18, "D2RenderStatisticsStrc.dwSpriteCacheBytes");
static_assert(offsetof(D2RenderStatisticsStrc, dwSpriteCacheStat1) == 0x24, "D2RenderStatisticsStrc.dwSpriteCacheStat1");
static_assert(offsetof(D2RenderStatisticsStrc, dwSpriteCacheMisses) == 0x30, "D2RenderStatisticsStrc.dwSpriteCacheMisses");
static_assert(offsetof(D2RenderStatisticsStrc, dwSpriteCacheStat2) == 0x3C, "D2RenderStatisticsStrc.dwSpriteCacheStat2");
static_assert(offsetof(D2RenderStatisticsStrc, dwSpriteCacheAccessesPerFrame) == 0x48, "D2RenderStatisticsStrc.dwSpriteCacheAccessesPerFrame");
static_assert(offsetof(D2RenderStatisticsStrc, dwSpriteCacheStat3) == 0x54, "D2RenderStatisticsStrc.dwSpriteCacheStat3");
static_assert(offsetof(D2RenderStatisticsStrc, dwSpriteCacheStat4) == 0x60, "D2RenderStatisticsStrc.dwSpriteCacheStat4");

// Slot 29 SetPaletteTable's argument: 72 BYTE* tables built by D2Win 004fb010 (every renderer copies
// 0x48 dwords). The Mac reads [69] (+0x114, RGB triplets) and [70] (+0x118, a 256-byte remap).
typedef BYTE *D2PaletteTable[72];
static_assert(sizeof(D2PaletteTable) == 0x120, "D2PaletteTable");
enum { D2PALETTE_STANDARD_COLORS = 69, D2PALETTE_INDEX_REMAP = 70 };

// Slot 28 SetPalette's argument: 256 PALETTEENTRY.
typedef PALETTEENTRY D2Palette256[256];
static_assert(sizeof(D2Palette256) == 0x400, "D2Palette256");

// RAD Bink handle: the four leading fields the renderer reads (binkw32's BINK struct).
struct BINK {
    uint32_t Width;     // +0x00
    uint32_t Height;    // +0x04
    uint32_t Frames;    // +0x08
    uint32_t FrameNum;  // +0x0C
    // the rest is RAD's
};
static_assert(offsetof(BINK, Width) == 0x0, "BINK.Width");
static_assert(offsetof(BINK, Height) == 0x4, "BINK.Height");
static_assert(offsetof(BINK, Frames) == 0x8, "BINK.Frames");
static_assert(offsetof(BINK, FrameNum) == 0xC, "BINK.FrameNum");
typedef BINK *HBINK;

// ================================================================================================
// Renderer-private structs (Mac layout).

// The 0x18-byte vertex shared by the ground grid, the tile quad, the wall/shadow grid and the sprite
// quad. Only +0x00..+0x13 are ever touched; glColor4ubv takes &r.
struct OGLVertex {
    float fX;           // +0x00  screen x (GL y-up space)
    float fY;           // +0x04  g_nScreenHeight - screen y
    BYTE r;             // +0x08
    BYTE g;             // +0x09
    BYTE b;             // +0x0A
    BYTE a;             // +0x0B
    float fU;           // +0x0C
    float fV;           // +0x10
    float fUnused;      // +0x14
};
static_assert(sizeof(OGLVertex) == 0x18, "OGLVertex");
static_assert(offsetof(OGLVertex, fX) == 0x00, "OGLVertex.fX");
static_assert(offsetof(OGLVertex, fY) == 0x04, "OGLVertex.fY");
static_assert(offsetof(OGLVertex, r) == 0x08, "OGLVertex.r");
static_assert(offsetof(OGLVertex, g) == 0x09, "OGLVertex.g");
static_assert(offsetof(OGLVertex, b) == 0x0A, "OGLVertex.b");
static_assert(offsetof(OGLVertex, a) == 0x0B, "OGLVertex.a");
static_assert(offsetof(OGLVertex, fU) == 0x0C, "OGLVertex.fU");
static_assert(offsetof(OGLVertex, fV) == 0x10, "OGLVertex.fV");
static_assert(offsetof(OGLVertex, fUnused) == 0x14, "OGLVertex.fUnused");

// One wall-corner record of g_aWallCornerTable[10][6]; nX/nY become 16.16 at first init.
struct OGLWallCorner {
    int nX;             // +0x0
    int nY;             // +0x4
    int nZ;             // +0x8  pixel height
};
static_assert(sizeof(OGLWallCorner) == 0xc, "OGLWallCorner");
static_assert(offsetof(OGLWallCorner, nX) == 0x0, "OGLWallCorner.nX");
static_assert(offsetof(OGLWallCorner, nY) == 0x4, "OGLWallCorner.nY");
static_assert(offsetof(OGLWallCorner, nZ) == 0x8, "OGLWallCorner.nZ");

// g_rgbGlobalLight (Mac 00552c2c), set by OGL_SetGlobalLight.
struct OGLColorRGB {
    BYTE r;
    BYTE g;
    BYTE b;
};
static_assert(sizeof(OGLColorRGB) == 3, "OGLColorRGB");

// The per-draw sprite record, a stack local in every image slot (Mac [EBP-0x40]). The Windows Glide
// renderer's GlideTexInfoStrc starts with the same 0x30 bytes.
struct OGLSprite {
    D2GfxDataStrc *pData;   // +0x00  set by OGL_SpriteLoad
    int nWidth;             // +0x04  GetDC6Width (DrawImageFast adds 2)
    int nHeight;            // +0x08  GetDC6Height (DrawImageFast adds 2)
    int nOffsetX;           // +0x0C  GetDC6OffsetX
    int nOffsetY;           // +0x10  GetDC6OffsetY (DrawShadow halves it in place)
    int nTexWidth;          // +0x14  pow2 >= nWidth (square pow2 >= 16 with AGP textures)
    int nTexHeight;         // +0x18
    int nTexSize;           // +0x1C  nTexWidth * nTexHeight
    float fS0;              // +0x20
    float fS1;              // +0x24
    float fT0;              // +0x28
    float fT1;              // +0x2C
};
static_assert(sizeof(OGLSprite) == 0x30, "OGLSprite");
static_assert(offsetof(OGLSprite, pData) == 0x00, "OGLSprite.pData");
static_assert(offsetof(OGLSprite, nWidth) == 0x04, "OGLSprite.nWidth");
static_assert(offsetof(OGLSprite, nHeight) == 0x08, "OGLSprite.nHeight");
static_assert(offsetof(OGLSprite, nOffsetX) == 0x0C, "OGLSprite.nOffsetX");
static_assert(offsetof(OGLSprite, nOffsetY) == 0x10, "OGLSprite.nOffsetY");
static_assert(offsetof(OGLSprite, nTexWidth) == 0x14, "OGLSprite.nTexWidth");
static_assert(offsetof(OGLSprite, nTexHeight) == 0x18, "OGLSprite.nTexHeight");
static_assert(offsetof(OGLSprite, nTexSize) == 0x1C, "OGLSprite.nTexSize");
static_assert(offsetof(OGLSprite, fS0) == 0x20, "OGLSprite.fS0");
static_assert(offsetof(OGLSprite, fS1) == 0x24, "OGLSprite.fS1");
static_assert(offsetof(OGLSprite, fT0) == 0x28, "OGLSprite.fT0");
static_assert(offsetof(OGLSprite, fT1) == 0x2C, "OGLSprite.fT1");

// The renderer's cache area in each DC6 block (DC6Block+0x18, returned by GetDC6BlockPixelData and
// passed to the pixel-data free callback): OGLSpriteTexSlot[5], indexed by texture mode 1..3.
struct OGLSpriteTexSlot {
    int nSizeClass;     // +0x0  0 unclassified, 3 <=32x32, 2 <=128x128, 1 <=256x256
    int nTextureId;     // +0x4  1-based g_pTextures node id, 0 = none
};
static_assert(sizeof(OGLSpriteTexSlot) == 0x8, "OGLSpriteTexSlot");
static_assert(offsetof(OGLSpriteTexSlot, nSizeClass) == 0x0, "OGLSpriteTexSlot.nSizeClass");
static_assert(offsetof(OGLSpriteTexSlot, nTextureId) == 0x4, "OGLSpriteTexSlot.nTextureId");
enum { OGL_SPRITE_TEX_SLOTS = 5 };

// The smacker banner quad (Mac 00552b90..bbc: clang dropped the members only ever stored as 0).
struct OGLSmackVertex {
    float fX;
    float fY;
    float fS;
    float fT;
};
static_assert(sizeof(OGLSmackVertex) == 0x10, "OGLSmackVertex");

// One entry of g_aCutsceneGLCaps (Mac 003ce7d0): a GL capability saved around the cutscene.
struct CutsceneGLCapState {
    GLenum eCap;        // +0x0
    GLboolean bSaved;   // +0x4  glGetBooleanv target, restored afterwards
    GLboolean bWanted;  // +0x5  state during the cutscene
    BYTE pad[2];        // +0x6
};
static_assert(sizeof(CutsceneGLCapState) == 0x8, "CutsceneGLCapState");
static_assert(offsetof(CutsceneGLCapState, eCap) == 0x0, "CutsceneGLCapState.eCap");
static_assert(offsetof(CutsceneGLCapState, bSaved) == 0x4, "CutsceneGLCapState.bSaved");
static_assert(offsetof(CutsceneGLCapState, bWanted) == 0x5, "CutsceneGLCapState.bWanted");

// A cutscene quad vertex (stack local CutsceneVertex[4] in OGL_PlayBinkMovie).
struct CutsceneVertex {
    float fX;
    float fY;
    float fU;
    float fV;
};
static_assert(sizeof(CutsceneVertex) == 0x10, "CutsceneVertex");

// One entry of g_aDisplayModes[3] (Mac 00552b50), filled by SetOption 2/3. Packed bit fields:
// bit 0 stretched, bits 1-15 width, bits 16-31 height; then the refresh rate as Fixed 16.16.
struct OGLDisplayModeRec {
    uint32_t dwSizeFlags;   // +0x0
    uint32_t fxRefresh;     // +0x4
};
static_assert(sizeof(OGLDisplayModeRec) == 0x8, "OGLDisplayModeRec");
static_assert(offsetof(OGLDisplayModeRec, dwSizeFlags) == 0x0, "OGLDisplayModeRec.dwSizeFlags");
static_assert(offsetof(OGLDisplayModeRec, fxRefresh) == 0x4, "OGLDisplayModeRec.fxRefresh");

inline bool OGLDisplayMode_IsStretched(const OGLDisplayModeRec &r) { return (r.dwSizeFlags & 1) != 0; }
inline int OGLDisplayMode_Width(const OGLDisplayModeRec &r) { return (int)((r.dwSizeFlags >> 1) & 0x7fff); }
inline int OGLDisplayMode_Height(const OGLDisplayModeRec &r) { return (int)(r.dwSizeFlags >> 16); }
inline int OGLDisplayMode_RefreshHz(const OGLDisplayModeRec &r) { return (int)(r.fxRefresh >> 16); }

// A texture handle record of CD2Textures (0x30 each, m_pNodes[id - 1]).
struct AGPTextureRecord;
struct D2TextureNode {
    int nId;                        // +0x00  1-based
    union {
        GLuint hTexture;            // +0x04  COGLTextures
        AGPTextureRecord *pAGP;     // +0x04  COGLAGPTextures
    };
    BOOL bUsedThisFrame;            // +0x08
    BOOL bUsedLastFrame;            // +0x0C
    int nWidth;                     // +0x10
    int nHeight;                    // +0x14
    int nOwnerType;                 // +0x18  OGLTextureOwner
    void *pOwner;                   // +0x1C
    int nOwnerIndex;                // +0x20  TEXOWNER_SPRITE only
    int nOwnerKey;                  // +0x24  TEXOWNER_SPRITE only: the palette the texture was built with
    D2TextureNode *pNext;           // +0x28
    D2TextureNode *pPrev;           // +0x2C
};
static_assert(sizeof(D2TextureNode) == 0x30, "D2TextureNode");
static_assert(offsetof(D2TextureNode, nId) == 0x00, "D2TextureNode.nId");
static_assert(offsetof(D2TextureNode, hTexture) == 0x04, "D2TextureNode.hTexture");
static_assert(offsetof(D2TextureNode, pAGP) == 0x04, "D2TextureNode.pAGP");
static_assert(offsetof(D2TextureNode, bUsedThisFrame) == 0x08, "D2TextureNode.bUsedThisFrame");
static_assert(offsetof(D2TextureNode, bUsedLastFrame) == 0x0C, "D2TextureNode.bUsedLastFrame");
static_assert(offsetof(D2TextureNode, nWidth) == 0x10, "D2TextureNode.nWidth");
static_assert(offsetof(D2TextureNode, nHeight) == 0x14, "D2TextureNode.nHeight");
static_assert(offsetof(D2TextureNode, nOwnerType) == 0x18, "D2TextureNode.nOwnerType");
static_assert(offsetof(D2TextureNode, pOwner) == 0x1C, "D2TextureNode.pOwner");
static_assert(offsetof(D2TextureNode, nOwnerIndex) == 0x20, "D2TextureNode.nOwnerIndex");
static_assert(offsetof(D2TextureNode, nOwnerKey) == 0x24, "D2TextureNode.nOwnerKey");
static_assert(offsetof(D2TextureNode, pNext) == 0x28, "D2TextureNode.pNext");
static_assert(offsetof(D2TextureNode, pPrev) == 0x2C, "D2TextureNode.pPrev");

// COGLAGPTextures pool records.
struct AGPSlot {
    GLuint name;        // +0x0
    int nOwner;         // +0x4  node id, -1 when free
};
static_assert(sizeof(AGPSlot) == 0x8, "AGPSlot");
static_assert(offsetof(AGPSlot, name) == 0x0, "AGPSlot.name");
static_assert(offsetof(AGPSlot, nOwner) == 0x4, "AGPSlot.nOwner");

struct AGPPage {
    int nOwner;         // +0x0  -1 when free
    GLuint name;        // +0x4
    int nUnknown08;     // +0x8
};
static_assert(sizeof(AGPPage) == 0xc, "AGPPage");
static_assert(offsetof(AGPPage, nOwner) == 0x0, "AGPPage.nOwner");
static_assert(offsetof(AGPPage, name) == 0x4, "AGPPage.name");
static_assert(offsetof(AGPPage, nUnknown08) == 0x8, "AGPPage.nUnknown08");

struct AGPTextureRecord {
    GLuint name;            // +0x0
    GLuint hPage2D;         // +0x4  never set non-zero in the recovered code
    BYTE *pPixels;          // +0x8  slot address in the arena
    uint32_t nUsageBits;    // +0xC  bit0 set on bind, shifted left each frame
};
static_assert(sizeof(AGPTextureRecord) == 0x10, "AGPTextureRecord");
static_assert(offsetof(AGPTextureRecord, name) == 0x0, "AGPTextureRecord.name");
static_assert(offsetof(AGPTextureRecord, hPage2D) == 0x4, "AGPTextureRecord.hPage2D");
static_assert(offsetof(AGPTextureRecord, pPixels) == 0x8, "AGPTextureRecord.pPixels");
static_assert(offsetof(AGPTextureRecord, nUsageBits) == 0xC, "AGPTextureRecord.nUsageBits");

#pragma pack(pop)

// ================================================================================================
// Function types that cross the boundary.

// The cutscene per-frame callback (slot 14 PlayCutScene arg 3), called with no arguments.
typedef void (*OGLFrameCallback)(void);

// D2CMP calls both of these with the argument in ECX (Windows 005fde87, 0060aa60).
typedef void(OGL_FASTCALL *OGLTileFreeCallback)(D2TileLibraryEntryStrc *pTile);
typedef void(OGL_FASTCALL *OGLPixelDataFreeCallback)(OGLSpriteTexSlot *pSlots);

// The image slots' "dwGamma" is a D2GfxLightStrc packed into a DWORD (byte 0 intensity, then r, g, b).
inline DWORD OGL_LightToDword(const D2GfxLightStrc &light)
{
    return (DWORD)light.nIntensity | ((DWORD)light.nRed << 8) | ((DWORD)light.nGreen << 16) |
           ((DWORD)light.nBlue << 24);
}

#endif
