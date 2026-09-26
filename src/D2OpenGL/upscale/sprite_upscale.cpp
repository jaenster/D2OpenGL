// Sprite textures made larger in palette-index space (sprite_upscale.h).

#include "sprite_upscale.h"

#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <d2util.h>

#include "hdpack.h"
#include "../present/present.h"
#include "../renderer/renderer.h"

#include "../../common/log.h"

namespace {

// ToggleKey: the original sprites, pixels repeated to the same texture size, or the upscaled ones.
bool s_bOriginal;

void Repeat(const uint8_t *pSrc, int nWidth, int nHeight, int nFactor, uint8_t *pDst)
{
    int nOutWidth = nWidth * nFactor;
    for (int y = 0; y < nHeight * nFactor; y++) {
        const uint8_t *pRow = pSrc + (size_t)(y / nFactor) * nWidth;
        uint8_t *pOut = pDst + (size_t)y * nOutWidth;
        for (int x = 0; x < nOutWidth; x++)
            pOut[x] = pRow[x / nFactor];
    }
}

int s_nFactor = 1;
bool s_bPackUsed;   // the HD pack's scale is the factor
bool s_bPackNoted;  // a scale mismatch was logged

struct Scratch {
    uint8_t *p;
    size_t n;
    uint8_t *Get(size_t nSize)
    {
        if (nSize > n) {
            uint8_t *pNew = static_cast<uint8_t *>(realloc(p, nSize));
            if (!pNew)
                return NULL;
            p = pNew;
            n = nSize;
        }
        return p;
    }
};
Scratch s_out;    // the texture image
Scratch s_raw;    // the cel without its colour map, for the pack lookup
Scratch s_merged; // the cel with every transparent index made one, for MMPX
Scratch s_tile;   // a floor tile or wall block, tightly packed and filled
Scratch s_tileOut;

// Timing, for the log.
unsigned s_nCount;
unsigned s_nNextLog = 64;
double s_fTotalUs;
double s_fMaxUs;
uint64_t s_nPixels;

double Microseconds(const LARGE_INTEGER &start)
{
    LARGE_INTEGER now, freq;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&freq);
    return (double)(now.QuadPart - start.QuadPart) * 1e6 / (double)freq.QuadPart;
}

// MMPX by the factor. The renderer's palette conversion makes every black entry transparent, so to
// the scaler they are one colour: all become the first of them. When that is index 0 it is libd2's
// transparent index; otherwise index 0 is an ordinary colour.
bool Magnify(const uint8_t *pSrc, int nWidth, int nHeight, uint8_t *pDst)
{
    const uint8_t *pPalette = &g_aPaletteRGB[0][0];
    uint8_t aMerge[256];
    int nFirst = -1;
    bool bMerge = false;
    for (int i = 0; i < 256; ++i) {
        bool bBlack = (pPalette[i * 3] | pPalette[i * 3 + 1] | pPalette[i * 3 + 2]) == 0;
        if (bBlack && nFirst < 0)
            nFirst = i;
        aMerge[i] = bBlack ? (uint8_t)nFirst : (uint8_t)i;
        bMerge |= bBlack && i != nFirst;
    }
    size_t nSize = (size_t)nWidth * nHeight;
    if (bMerge) {
        uint8_t *pMerged = s_merged.Get(nSize);
        if (!pMerged)
            return false;
        for (size_t i = 0; i < nSize; ++i)
            pMerged[i] = aMerge[pSrc[i]];
        pSrc = pMerged;
    }
    int nFlags = nFirst == 0 ? 0 : D2UTIL_INDEX0_OPAQUE;
    return d2_upscale_indices_ex(pSrc, nWidth, nHeight, nWidth, s_nFactor, pPalette, pDst, nWidth * s_nFactor,
                                 nFlags) == 0;
}

// The pack frame for the cel, looked up by its indices before any colour map or remap.
bool FindPacked(const OGLSprite *pSprite, const uint8_t *pIndices, int nSlot, uint8_t *pColorMap, int nWidth,
                int nHeight, HDPackFrame *pFrame)
{
    const uint8_t *pRaw = pIndices;
    if (pColorMap || nSlot == 2) {
        uint8_t *pBuffer = s_raw.Get((size_t)nWidth * nHeight);
        if (!pBuffer)
            return false;
        memset(pBuffer, 0, (size_t)nWidth * nHeight);
        g_host.DrawCelFrame(pSprite->pData, -pSprite->nOffsetX, nHeight - 1 - pSprite->nOffsetY, 0, 0, pBuffer,
                            nHeight, nWidth, 0, 0, NULL);
        pRaw = pBuffer;
    }
    return HDPack_Find(pRaw, nWidth, nHeight, nWidth, pFrame);
}

// The pack frame at its box origin times the factor, every index through the colour map and the
// remap as the decode put the cel's through them; 0 elsewhere.
void PlacePacked(const HDPackFrame &frame, uint8_t *pDst, int nDstWidth, int nDstHeight, const uint8_t *pColorMap,
                 const uint8_t *pRemap)
{
    memset(pDst, 0, (size_t)nDstWidth * nDstHeight);
    int nBoxWidth = frame.w * s_nFactor;
    int nBoxHeight = frame.h * s_nFactor;
    const uint8_t *pSrc = frame.pPixels;
    for (int y = 0; y < nBoxHeight; ++y) {
        uint8_t *pRow = pDst + (size_t)(frame.y0 * s_nFactor + y) * nDstWidth + frame.x0 * s_nFactor;
        for (int x = 0; x < nBoxWidth; ++x) {
            uint8_t v = *pSrc++;
            if (v && pColorMap)
                v = pColorMap[v];
            if (v && pRemap)
                v = pRemap[v];
            pRow[x] = v;
        }
    }
}

// The tile's image from the pack, MMPX, or repeated pixels, masked to the source's own outline.
const uint8_t *MagnifyTile(const uint8_t *pFilled, const uint8_t *pMask, int nWidth, int nHeight)
{
    int nOutWidth = nWidth * s_nFactor;
    int nOutHeight = nHeight * s_nFactor;
    uint8_t *pOut = s_tileOut.Get((size_t)nOutWidth * nOutHeight);
    if (!pOut)
        return NULL;
    HDPackFrame frame;
    if (s_bOriginal) {
        Repeat(pMask, nWidth, nHeight, s_nFactor, pOut);
        return pOut;
    }
    if (s_bPackUsed && HDPack_Find(pMask, nWidth, nHeight, nWidth, &frame)) {
        PlacePacked(frame, pOut, nOutWidth, nOutHeight, NULL, NULL);
        return pOut;
    }
    if (!Magnify(pFilled, nWidth, nHeight, pOut))
        return NULL;
    for (int y = 0; y < nOutHeight; y++) {
        const uint8_t *pRow = pMask + (size_t)(y / s_nFactor) * nWidth;
        uint8_t *pDst = pOut + (size_t)y * nOutWidth;
        for (int x = 0; x < nOutWidth; x++)
            if (!pRow[x / s_nFactor])
                pDst[x] = 0;
    }
    return pOut;
}

}  // namespace

const uint8_t *Upscale_FloorTile(const uint8_t *pSrc, int nWidth, int nHeight, int nPitch, int nCellWidth,
                                 int nCellHeight)
{
    if (s_nFactor == 1)
        return NULL;
    size_t nSize = (size_t)nWidth * nHeight;
    uint8_t *pBuf = s_tile.Get(nSize * 2);
    if (!pBuf)
        return NULL;
    uint8_t *pMask = pBuf;          // the tile as it is
    uint8_t *pFilled = pBuf + nSize; // its empty pixels taken from the tile one lattice step away
    for (int y = 0; y < nHeight; y++)
        memcpy(pMask + (size_t)y * nWidth, pSrc + (size_t)y * nPitch, nWidth);
    memcpy(pFilled, pMask, nSize);
    const int nStepX = nCellWidth / 2;
    const int nStepY = nCellHeight / 2;
    static const int aSign[4][2] = {{1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
    for (int y = 0; y < nHeight; y++) {
        for (int x = 0; x < nWidth; x++) {
            if (pMask[(size_t)y * nWidth + x])
                continue;
            for (int k = 0; k < 4; k++) {
                int sx = x + aSign[k][0] * nStepX;
                int sy = y + aSign[k][1] * nStepY;
                if (sx < 0 || sy < 0 || sx >= nCellWidth || sy >= nCellHeight || sx >= nWidth || sy >= nHeight)
                    continue;
                uint8_t v = pMask[(size_t)sy * nWidth + sx];
                if (v) {
                    pFilled[(size_t)y * nWidth + x] = v;
                    break;
                }
            }
        }
    }
    return MagnifyTile(pFilled, pMask, nWidth, nHeight);
}

const uint8_t *Upscale_WallBlock(const uint8_t *pSrc, int nWidth, int nHeight)
{
    if (s_nFactor == 1)
        return NULL;
    return MagnifyTile(pSrc, pSrc, nWidth, nHeight);
}

int Upscale_TileFactor(void)
{
    return s_nFactor;
}

int Upscale_OpenTextures(void)
{
    int nScale = Present_RenderScale();
    if (nScale <= 1 || !Present_UpscaleSprites())
        s_nFactor = 1;
    else
        s_nFactor = nScale >= 4 ? 4 : 2;

    int nPackScale = HDPack_Open(Present_HDPackPath());
    s_bPackUsed = s_nFactor > 1 && nPackScale == s_nFactor;
    if (nPackScale && !s_bPackUsed && !s_bPackNoted) {
        s_bPackNoted = true;
        d2log("hdpack: not used: its scale is %d, sprite textures are %dx", nPackScale, s_nFactor);
    }
    return s_nFactor;
}

bool Upscale_ToggleOriginal(void)
{
    s_bOriginal = !s_bOriginal;
    d2log("upscale: %s sprites", s_bOriginal ? "original" : "upscaled");
    return s_bOriginal;
}

void Upscale_CloseTextures(void)
{
    s_nFactor = 1;
    s_bPackUsed = false;
}

const uint8_t *Upscale_Sprite(const OGLSprite *pSprite, const uint8_t *pIndices, int nSlot, uint8_t *pColorMap,
                              int *pnWidth, int *pnHeight)
{
    if (s_nFactor == 1)
        return pIndices;
    LARGE_INTEGER start;
    QueryPerformanceCounter(&start);

    int nWidth = *pnWidth;
    int nHeight = *pnHeight;
    int nOutWidth = nWidth * s_nFactor;
    int nOutHeight = nHeight * s_nFactor;
    uint8_t *pOut = s_out.Get((size_t)nOutWidth * nOutHeight);
    if (!pOut)
        return pIndices;

    HDPackFrame frame;
    if (s_bOriginal)
        Repeat(pIndices, nWidth, nHeight, s_nFactor, pOut);
    else if (s_bPackUsed && nSlot != 3 && FindPacked(pSprite, pIndices, nSlot, pColorMap, nWidth, nHeight, &frame))
        PlacePacked(frame, pOut, nOutWidth, nOutHeight, pColorMap,
                    nSlot == 2 ? g_pPaletteTable[D2PALETTE_INDEX_REMAP] : NULL);
    else if (!Magnify(pIndices, nWidth, nHeight, pOut))
        return pIndices;
    *pnWidth = nOutWidth;
    *pnHeight = nOutHeight;

    double fUs = Microseconds(start);
    s_fTotalUs += fUs;
    if (fUs > s_fMaxUs)
        s_fMaxUs = fUs;
    s_nPixels += (uint64_t)nWidth * nHeight;
    if (++s_nCount == s_nNextLog) {
        s_nNextLog *= 4;
        d2log("upscale: %u sprite textures at %dx, %.0f us average, %.0f us longest, %.0f source pixels average",
              s_nCount, s_nFactor, s_fTotalUs / s_nCount, s_fMaxUs, (double)s_nPixels / s_nCount);
    }
    return pOut;
}
