// Sprites (Mac 002e1ddf-002e480c): oglPerspective (the perspective divisor table and the
// position slots), oglSmack (the 468x60 banner) and oglSprite (the sprite staging buffers, the
// per-sprite texture cache and the image draw slots).

#include "renderer.h"

#include "../upscale/sprite_upscale.h"

// ================================================================================================
// oglPerspective.cpp

// Mac 002e1ddf
void OGL_AllocPerspectiveTable(void)
{
    g_pPerspectiveTable = static_cast<int *>(g_host.AllocClientMemory(0x8000, g_szFileOglPerspective, 57, 0));
}

// Mac 002e1e1e
void OGL_FreePerspectiveTable(void)
{
    if (g_pPerspectiveTable) {
        g_host.FreeClientMemory(g_pPerspectiveTable, g_szFileOglPerspective, 64, 0);
        g_pPerspectiveTable = NULL;
    }
}

// Mac 002e1e67
void OGL_BuildPerspectiveTable(void)
{
    int *pTable = g_pPerspectiveTable;
    for (int i = -0x1000; i != 0x1000; ++i) {
        int nDivisor;
        if (i >= 2999)
            nDivisor = -1;
        else if (i <= -2999)
            nDivisor = -5999;
        else
            nDivisor = i - 3000;
        pTable[i + 0x1000] = 0x2ee000 / nDivisor;
    }
    g_nPerspectiveFactor = 0x100;
}

// Mac 002e1ede
BOOL OGL_IsPerspectiveEnabled(void)
{
    if (!g_pGfxSettings->bPerspectiveCapable)
        return 0;
    return g_pGfxSettings->bPerspectiveEnabled;
}

// Mac 002e1efd OGL_SetPerspectiveScale
void OGL_SetPerspectiveScale(int nScaleX, int nScaleY)
{
    g_nPerspectiveCenterX = nScaleX;
    g_nPerspectiveCenterY = nScaleY;
}

// Mac 002e1f1a OGL_AdjustPerspectivePosition
void OGL_AdjustPerspectivePosition(int nX, int nY, int nBias, int *pXAdjust, int *pYAdjust)
{
    // The products overflow 32 bits for large inputs: they are computed in uint32_t so they wrap as
    // the Mac's IMULs do; the right shifts are arithmetic.
    int nA = (int32_t)(((uint32_t)nX - (uint32_t)g_nPerspectiveCenterX) * 0x18u) >> 16;
    int nB = (int32_t)(((uint32_t)nY - (uint32_t)g_nPerspectiveCenterY) * 0x18u) >> 16;
    uint32_t uDiff = (uint32_t)nB - (uint32_t)nA;
    uint32_t uSum = (uint32_t)nB + (uint32_t)nA;
    int nIndex = (int32_t)((uint32_t)nBias * 0x99800u + uSum * 0x9c9b8u) >> 20;
    int nK = g_pPerspectiveTable[((uint32_t)nIndex + 0x1000u) & 0x1fff];

    *pXAdjust = g_nScreenWidth / 2 + ((int32_t)(uDiff * (uint32_t)nK * 0x2d4u) >> 20);

    int nT = (int32_t)((uint32_t)nBias * 0x109a08u + uSum * (uint32_t)-0x5a800) >> 10;
    *pYAdjust = g_nScreenHeight / 2 + ((int32_t)((uint32_t)nT * (uint32_t)nK) >> 20) - 0x20;
}

// Mac 002e1fcc OGL_PerspectiveScalePosition
void OGL_PerspectiveScalePosition(int nX, int nY, int nAngle, int *pXAdjust, int *pYAdjust, BOOL bOrder)
{
    (void)nAngle;

    int nA = (int32_t)(((uint32_t)nX - (uint32_t)g_nPerspectiveCenterX) * 0x18u) >> 16;
    int nB = (int32_t)(((uint32_t)nY - (uint32_t)g_nPerspectiveCenterY) * 0x18u) >> 16;
    uint32_t uDiff = (uint32_t)nB - (uint32_t)nA;
    uint32_t uSum = (uint32_t)nB + (uint32_t)nA;
    int nIndex = (int32_t)(uSum * 0x9c9b8u) >> 20;
    int *pTable = g_pPerspectiveTable;
    int nK = pTable[((uint32_t)nIndex + 0x1000u) & 0x1fff];

    *pXAdjust = g_nScreenWidth / 2 + ((int32_t)(uDiff * (uint32_t)nK * 0x2d4u) >> 20);

    int nE = (int32_t)(uSum * (uint32_t)-0x2d4) >> 1;
    int nV = (int32_t)((uint32_t)nK * (uint32_t)nE) >> 20;
    *pYAdjust = g_nScreenHeight / 2 + nV - 0x10;

    int nW = (int32_t)(((uint32_t)nE + 0x37400u) * (uint32_t)pTable[((uint32_t)nIndex + 0x1080u) & 0x1fff]) >> 20;
    uint32_t uFactor = (uint32_t)nV - (uint32_t)nW;
    g_nPerspectiveFactor = (int32_t)uFactor;
    if (bOrder)
        uFactor *= 0x4ccu;
    else
        uFactor *= 0x466u;
    g_nPerspectiveFactor = (int32_t)uFactor >> 10;
}

// Mac 002e20bf OGL_SetDefaultPerspectiveFactor
void OGL_SetDefaultPerspectiveFactor(void)
{
    g_nPerspectiveFactor = 0x100;
}

// ================================================================================================
// oglSmack.cpp

// Darwin libc memset_pattern16: fills nLen bytes at pDst with the 16-byte pattern, repeated.
static void OGL_MemsetPattern16(void *pDst, const void *pPattern, size_t nLen)
{
    BYTE *pOut = static_cast<BYTE *>(pDst);
    const BYTE *pIn = static_cast<const BYTE *>(pPattern);
    for (size_t i = 0; i < nLen; ++i)
        pOut[i] = pIn[i & 15];
}

// Mac 002e20d4
void OGL_InitSmackerTextures(void)
{
    g_pSmackIndexBuf = static_cast<BYTE *>(g_host.AllocClientMemory(0x8000, g_szFileOglSmack, 60, 0));
    g_apSmackPixels[0] = static_cast<uint32_t *>(g_host.AllocClientMemory(0x10000, g_szFileOglSmack, 61, 0));
    g_apSmackPixels[1] = static_cast<uint32_t *>(g_host.AllocClientMemory(0x10000, g_szFileOglSmack, 62, 0));
    glGenTextures(1, &g_aSmackTextures[0]);
    glGenTextures(1, &g_aSmackTextures[1]);
    GLuint *pTexture = g_aSmackTextures;
    for (int i = 2; i != 0; --i, ++pTexture) {
        glBindTexture(GL_TEXTURE_2D, *pTexture);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 9728.0f);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 9728.0f);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 10496.0f);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 10496.0f);
    }
}

// Mac 002e221e
void OGL_FreeSmackerTextures(void)
{
    glDeleteTextures(2, g_aSmackTextures);
    memset(g_aSmackTextures, 0, sizeof(g_aSmackTextures));
    if (g_pSmackIndexBuf) {
        g_host.FreeClientMemory(g_pSmackIndexBuf, g_szFileOglSmack, 117, 0);
        g_pSmackIndexBuf = NULL;
    }
    if (g_apSmackPixels[0]) {
        g_host.FreeClientMemory(g_apSmackPixels[0], g_szFileOglSmack, 122, 0);
        g_apSmackPixels[0] = NULL;
    }
    if (g_apSmackPixels[1]) {
        g_host.FreeClientMemory(g_apSmackPixels[1], g_szFileOglSmack, 127, 0);
        g_apSmackPixels[1] = NULL;
    }
}

// Mac 002e22fb OGL_DecodeSmacker
void OGL_DecodeSmacker(char *szSmacker, D2SmackerContextStrc *pContext, int nVersion)
{
    memset(g_pSmackIndexBuf, 0, 0x8000);
    for (int i = 0; i != 0x10000; i += 0x400)
        OGL_MemsetPattern16(reinterpret_cast<BYTE *>(g_apSmackPixels[0]) + i, g_adwSmackFillPattern, 0x400);
    for (int i = 0; i != 0x10000; i += 0x400)
        OGL_MemsetPattern16(reinterpret_cast<BYTE *>(g_apSmackPixels[1]) + i, g_adwSmackFillPattern, 0x400);
    GLuint *pTexture = g_aSmackTextures;
    uint32_t **ppPixels = g_apSmackPixels;
    for (int i = 2; i != 0; --i, ++pTexture, ++ppPixels) {
        glBindTexture(GL_TEXTURE_2D, *pTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, 256, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, *ppPixels);
    }

    char cMode = (char)nVersion;
    pContext->eMode = cMode;
    if (cMode == 'p') {
        void *pFile = NULL;
        DWORD nSize = 0;
        BOOL bLoaded = g_host.SFileLoadFileEx(NULL, szSmacker, &pFile, &nSize, 0, 3, NULL);
        if (!bLoaded)
            OGL_HALT(216);
        if (!pFile)
            OGL_HALT(217);
        if (bLoaded && pFile) {
            PALETTEENTRY aPalette[256];
            unsigned nWidth, nHeight, nBitDepth;
            uint32_t bDecoded = g_host.SBmpDecodeImage(2, pFile, nSize, aPalette, g_pSmackIndexBuf, 0x6db0, &nWidth,
                                                       &nHeight, &nBitDepth);
            g_host.SMemFree(pFile, g_szFileOglSmack, 236, 0);
            if (bDecoded && nWidth == 468 && nHeight == 60 && nBitDepth == 8) {
                // Re-stride the rows from pitch 468 to pitch 512, bottom up (row 0 stays).
                int nRow = 59;
                int nDstOffset = 0x7600;
                size_t nRowBytes = 468;
                while (true) {
                    int nSrcOffset = (int)nRowBytes * nRow;
                    --nRow;
                    memmove(g_pSmackIndexBuf + nDstOffset, g_pSmackIndexBuf + nSrcOffset, nRowBytes);
                    if (nRow < 1)
                        break;
                    nDstOffset -= 0x200;
                    nRowBytes = nWidth;
                }

                BYTE aRGB[256 * 3];
                for (int i = 0; i != 256; ++i) {
                    aRGB[i * 3 + 0] = aPalette[i].peRed;
                    aRGB[i * 3 + 1] = aPalette[i].peGreen;
                    aRGB[i * 3 + 2] = aPalette[i].peBlue;
                }
                OGL_SmackExpandToRGBA(g_pSmackIndexBuf, g_apSmackPixels[0], aRGB);
                OGL_SmackExpandToRGBA(g_pSmackIndexBuf + 0x100, g_apSmackPixels[1], aRGB);

                ppPixels = g_apSmackPixels;
                pTexture = g_aSmackTextures;
                for (int i = 2; i != 0; --i, ++pTexture, ++ppPixels) {
                    glBindTexture(GL_TEXTURE_2D, *pTexture);
                    OGL_SetTextureFilter(OGL_FILTER_LINEAR);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 64, GL_RGBA, GL_UNSIGNED_BYTE, *ppPixels);
                }
                return;
            }
        }
    } else {
        if (cMode != 's' || pContext->hSmack)
            return;
        // The Mac resolves szSmacker to an FSSpec here and, on success, clears hSmack; Windows has
        // no FSSpec step and the handle is already clear.
        pContext->hSmack = NULL;
    }

    if (pContext->eMode == 's' && pContext->hSmack)
        pContext->hSmack = NULL;
    pContext->eMode = 0;
}

// Mac 002e2730 OGL_CloseSmacker
void OGL_CloseSmacker(D2SmackerContextStrc *pContext)
{
    if (pContext->eMode == 's' && pContext->hSmack)
        pContext->hSmack = NULL;
    pContext->eMode = 0;
}

// Mac 002e274d OGL_PlayerSmacker
void OGL_PlayerSmacker(D2SmackerContextStrc *pContext)
{
    if (pContext->eMode != 'p' || g_bSurfacePaused)
        return;

    int nY = pContext->nY;
    OGL_SetTextureMode(OGL_TEXMODE_TEXTURED);
    OGL_SetBlendMode(OGL_BLEND_OFF);
    glColor4ubv(g_abSmackWhite);

    int nX = pContext->nX;
    float fLeft = (float)nX;
    g_aSmackQuad[0].fX = fLeft;
    float fTop = (float)(g_nScreenHeight - (nY - 64));
    g_aSmackQuad[0].fY = fTop;
    g_aSmackQuad[1].fX = fLeft;
    float fBottom = (float)(g_nScreenHeight - nY);
    g_aSmackQuad[1].fY = fBottom;
    g_aSmackQuad[1].fT = 0.9375f;
    float fMiddle = (float)(nX + 256);
    g_aSmackQuad[2].fX = fMiddle;
    g_aSmackQuad[2].fY = fBottom;
    g_aSmackQuad[2].fS = 1.0f;
    g_aSmackQuad[2].fT = 0.9375f;
    g_aSmackQuad[3].fX = fMiddle;
    g_aSmackQuad[3].fY = fTop;
    g_aSmackQuad[3].fS = 1.0f;
    glBindTexture(GL_TEXTURE_2D, g_aSmackTextures[0]);
    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(g_aSmackQuad[0].fX, g_aSmackQuad[0].fY);
    glTexCoord2f(0.0f, g_aSmackQuad[1].fT);
    glVertex2f(g_aSmackQuad[1].fX, g_aSmackQuad[1].fY);
    glTexCoord2f(g_aSmackQuad[2].fS, g_aSmackQuad[2].fT);
    glVertex2f(g_aSmackQuad[2].fX, g_aSmackQuad[2].fY);
    glTexCoord2f(g_aSmackQuad[3].fS, 0.0f);
    glVertex2f(g_aSmackQuad[3].fX, g_aSmackQuad[3].fY);
    glEnd();

    g_aSmackQuad[0].fX = fMiddle;
    fTop = (float)(g_nScreenHeight - (nY - 64));
    g_aSmackQuad[0].fY = fTop;
    g_aSmackQuad[1].fX = fMiddle;
    fBottom = (float)(g_nScreenHeight - nY);
    g_aSmackQuad[1].fY = fBottom;
    g_aSmackQuad[1].fT = 0.9375f;
    float fRight = (float)(nX + 468);
    g_aSmackQuad[2].fX = fRight;
    g_aSmackQuad[2].fY = fBottom;
    g_aSmackQuad[2].fS = 0.828125f;
    g_aSmackQuad[2].fT = 0.9375f;
    g_aSmackQuad[3].fX = fRight;
    g_aSmackQuad[3].fY = fTop;
    g_aSmackQuad[3].fS = 0.828125f;
    glBindTexture(GL_TEXTURE_2D, g_aSmackTextures[1]);
    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(g_aSmackQuad[0].fX, g_aSmackQuad[0].fY);
    glTexCoord2f(0.0f, g_aSmackQuad[1].fT);
    glVertex2f(g_aSmackQuad[1].fX, g_aSmackQuad[1].fY);
    glTexCoord2f(g_aSmackQuad[2].fS, g_aSmackQuad[2].fT);
    glVertex2f(g_aSmackQuad[2].fX, g_aSmackQuad[2].fY);
    glTexCoord2f(g_aSmackQuad[3].fS, 0.0f);
    glVertex2f(g_aSmackQuad[3].fX, g_aSmackQuad[3].fY);
    glEnd();
}

// Mac 002e2af8
void OGL_SmackExpandToRGBA(const BYTE *pIndices, uint32_t *pDst, const BYTE *pRGB)
{
    for (int nRow = 0; nRow != 64; ++nRow) {
        for (int x = 0; x != 256; ++x) {
            uint32_t dwPixel = 0xff;
            if (pIndices[x] && pRGB) {
                int nEntry = pIndices[x] * 3;
                dwPixel = (uint32_t)pRGB[nEntry + 2] << 8 | (uint32_t)pRGB[nEntry] << 24 |
                          (uint32_t)pRGB[nEntry + 1] << 16 | 0xff;
            }
            pDst[x] = dwPixel;
        }
        pDst += 256;
        pIndices += 0x200;
    }
}

// ================================================================================================
// oglSprite.cpp

// Mac 002e2b70
void OGL_InitSpriteBuffers(void)
{
    if (g_pSpriteBuffer)
        OGL_HALT(94);
    if (g_pSpriteOutlineBuffer)
        OGL_HALT(95);
    if (g_pSpriteSpareBuffer)
        OGL_HALT(96);
    g_pSpriteBuffer = static_cast<BYTE *>(g_host.AllocClientMemory(0x10000, g_szFileOglSprite, 98, 0));
    g_pSpriteOutlineBuffer = static_cast<BYTE *>(g_host.AllocClientMemory(0x10000, g_szFileOglSprite, 99, 0));
    g_pSpriteSpareBuffer = static_cast<BYTE *>(g_host.AllocClientMemory(0x40000, g_szFileOglSprite, 100, 0));
    g_host.SetPixelDataFreeCallback(OGL_FreeSpriteTextures);
}

// Mac 002e2c8c
void OGL_FASTCALL OGL_FreeSpriteTextures(OGLSpriteTexSlot *pSlots)
{
    for (int i = 0; i != OGL_SPRITE_TEX_SLOTS; ++i) {
        if (pSlots[i].nSizeClass > 0 && pSlots[i].nTextureId > 0)
            g_pTextures->FreeTextureById(pSlots[i].nTextureId);
    }
}

// Mac 002e2cd0
void OGL_FreeSpriteBuffers(void)
{
    if (g_pSpriteBuffer) {
        g_host.FreeClientMemory(g_pSpriteBuffer, g_szFileOglSprite, 115, 0);
        g_pSpriteBuffer = NULL;
    }
    if (g_pSpriteOutlineBuffer) {
        g_host.FreeClientMemory(g_pSpriteOutlineBuffer, g_szFileOglSprite, 121, 0);
        g_pSpriteOutlineBuffer = NULL;
    }
    if (g_pSpriteSpareBuffer) {
        g_host.FreeClientMemory(g_pSpriteSpareBuffer, g_szFileOglSprite, 127, 0);
        g_pSpriteSpareBuffer = NULL;
    }
    g_host.SetPixelDataFreeCallback(NULL);
}

// Mac 002e2d91 OGL_DrawPerspectiveImage
void OGL_DrawPerspectiveImage(D2GfxDataStrc *pData, int nX, int nY, DWORD dwGamma, int nDrawMode, int nScreenMode,
                              BYTE *pPalette)
{
    OGLSprite spr;
    if (!OGL_SpriteLoad(pData, &spr))
        return;
    OGL_SpriteCalcTextureSize(&spr);
    float fS0 = spr.fS0;
    float fS1 = spr.fS1;
    float fT0 = spr.fT0;
    float fT1 = spr.fT1;
    OGL_IsPerspectiveEnabled();

    // World corners in fixed point.
    int nTexWidth = spr.nTexWidth;
    int nHeightFixed = spr.nTexHeight << 12;
    int nWorldY = (spr.nOffsetY << 12) - (spr.nOffsetX << 11) + nY;
    int nWorldX = (spr.nOffsetX << 11) + nX + (spr.nOffsetY << 12);
    int nPX, nPY;

    OGL_AdjustPerspectivePosition(nWorldX - nHeightFixed, nWorldY - nHeightFixed, 0, &nPX, &nPY);
    g_aSpriteQuad[0].fX = (float)nPX;
    g_aSpriteQuad[0].fY = (float)g_nScreenHeight - (float)nPY;
    g_aSpriteQuad[0].fU = fS0;
    g_aSpriteQuad[0].fV = fT0;

    OGL_AdjustPerspectivePosition(nWorldX, nWorldY, 0, &nPX, &nPY);
    g_aSpriteQuad[1].fX = (float)nPX;
    g_aSpriteQuad[1].fY = (float)g_nScreenHeight - (float)nPY;
    g_aSpriteQuad[1].fU = fS0;
    g_aSpriteQuad[1].fV = fT1;

    nWorldY -= nTexWidth << 11;
    nWorldX += nTexWidth << 11;
    OGL_AdjustPerspectivePosition(nWorldX, nWorldY, 0, &nPX, &nPY);
    g_aSpriteQuad[2].fX = (float)nPX;
    g_aSpriteQuad[2].fY = (float)g_nScreenHeight - (float)nPY;
    g_aSpriteQuad[2].fU = fS1;
    g_aSpriteQuad[2].fV = fT1;

    OGL_AdjustPerspectivePosition(nWorldX - nHeightFixed, nWorldY - nHeightFixed, 0, &nPX, &nPY);
    g_aSpriteQuad[3].fX = (float)nPX;
    int nScreenHeight = g_nScreenHeight;
    float fY3 = (float)nScreenHeight - (float)nPY;
    g_aSpriteQuad[3].fY = fY3;
    g_aSpriteQuad[3].fU = fS1;
    g_aSpriteQuad[3].fV = fT0;

    OGLVertex *pVertex = g_aSpriteQuad;
    if (nScreenMode == 2) {
        for (int i = 4; i != 0; --i, ++pVertex)
            pVertex->fX = pVertex->fX + 160.0f;
    } else if (nScreenMode == 1) {
        for (int i = 4; i != 0; --i, ++pVertex)
            pVertex->fX = pVertex->fX + -160.0f;
    }

    int nMaxY = nScreenHeight - 47;
    int nScreenWidth = g_nScreenWidth;
    if (!((0.0f <= g_aSpriteQuad[0].fX && g_aSpriteQuad[0].fX < (float)nScreenWidth && 0.0f <= g_aSpriteQuad[0].fY &&
           g_aSpriteQuad[0].fY < (float)nMaxY) ||
          (0.0f <= g_aSpriteQuad[1].fX && g_aSpriteQuad[1].fX < (float)nScreenWidth && 0.0f <= g_aSpriteQuad[1].fY &&
           g_aSpriteQuad[1].fY < (float)nMaxY) ||
          (0.0f <= g_aSpriteQuad[2].fX && g_aSpriteQuad[2].fX < (float)nScreenWidth && 0.0f <= g_aSpriteQuad[2].fY &&
           g_aSpriteQuad[2].fY < (float)nMaxY) ||
          (0.0f <= g_aSpriteQuad[3].fX && g_aSpriteQuad[3].fX < (float)nScreenWidth && 0.0f <= fY3 &&
           fY3 < (float)nMaxY)))
        return;

    if (!OGL_SpriteBindTexture(&spr, pPalette, 0, 1))
        return;
    BYTE nAlpha = OGL_SetDrawModeBlend(nDrawMode);
    OGL_SetTextureMode(OGL_TEXMODE_TEXTURED);
    OGL_SetVertexColorFromLight(&g_aSpriteQuad[0], dwGamma);
    g_aSpriteQuad[0].a = nAlpha;
    glColor4ubv(&g_aSpriteQuad[0].r);
    if (g_bTextureRectangle) {
        float fTexHeight = (float)spr.nTexHeight;
        float fTexWidth = (float)spr.nTexWidth;
        glBegin(GL_TRIANGLE_FAN);
        glTexCoord2f(g_aSpriteQuad[0].fU * fTexWidth, g_aSpriteQuad[0].fV * fTexHeight);
        glVertex2f(g_aSpriteQuad[0].fX, g_aSpriteQuad[0].fY);
        glTexCoord2f(g_aSpriteQuad[1].fU * fTexWidth, g_aSpriteQuad[1].fV * fTexHeight);
        glVertex2f(g_aSpriteQuad[1].fX, g_aSpriteQuad[1].fY);
        glTexCoord2f(g_aSpriteQuad[2].fU * fTexWidth, g_aSpriteQuad[2].fV * fTexHeight);
        glVertex2f(g_aSpriteQuad[2].fX, g_aSpriteQuad[2].fY);
        glTexCoord2f(fTexWidth * g_aSpriteQuad[3].fU, fTexHeight * g_aSpriteQuad[3].fV);
    } else {
        glBegin(GL_TRIANGLE_FAN);
        glTexCoord2f(g_aSpriteQuad[0].fU, g_aSpriteQuad[0].fV);
        glVertex2f(g_aSpriteQuad[0].fX, g_aSpriteQuad[0].fY);
        glTexCoord2f(g_aSpriteQuad[1].fU, g_aSpriteQuad[1].fV);
        glVertex2f(g_aSpriteQuad[1].fX, g_aSpriteQuad[1].fY);
        glTexCoord2f(g_aSpriteQuad[2].fU, g_aSpriteQuad[2].fV);
        glVertex2f(g_aSpriteQuad[2].fX, g_aSpriteQuad[2].fY);
        glTexCoord2f(g_aSpriteQuad[3].fU, g_aSpriteQuad[3].fV);
    }
    glVertex2f(g_aSpriteQuad[3].fX, g_aSpriteQuad[3].fY);
    glEnd();
}

// Mac 002e33a9
BOOL OGL_SpriteLoad(D2GfxDataStrc *pData, OGLSprite *pSprite)
{
    if (!g_host.GetOrLoadSprite(pData, 0, 1))
        return FALSE;
    if (!OGL_GfxDataBlock(pData))
        OGL_HALT(179);
    pSprite->pData = pData;
    pSprite->nWidth = g_host.GetDC6Width(OGL_GfxDataBlock(pData));
    pSprite->nOffsetX = g_host.GetDC6OffsetX(OGL_GfxDataBlock(pData));
    pSprite->nHeight = g_host.GetDC6Height(OGL_GfxDataBlock(pData));
    pSprite->nOffsetY = g_host.GetDC6OffsetY(OGL_GfxDataBlock(pData));
    return TRUE;
}

// Mac 002e344e
void OGL_SpriteCalcTextureSize(OGLSprite *pSprite)
{
    int nTexWidth, nTexHeight;
    if (g_bAGPTextures) {
        int nSize = pSprite->nHeight;
        if (pSprite->nWidth >= nSize)
            nSize = pSprite->nWidth;
        int nNext = 16;
        do {
            nTexWidth = nNext;
            nNext = nTexWidth * 2;
        } while (nTexWidth < nSize);
        nTexHeight = nTexWidth;
    } else {
        int nNext = 1;
        do {
            nTexWidth = nNext;
            nNext = nTexWidth * 2;
        } while (nTexWidth < pSprite->nWidth);
        nNext = 1;
        do {
            nTexHeight = nNext;
            nNext = nTexHeight * 2;
        } while (nTexHeight < pSprite->nHeight);
    }
    pSprite->nTexWidth = nTexWidth;
    pSprite->nTexHeight = nTexHeight;
    OGL_IsPerspectiveEnabled();
    pSprite->fS0 = 0.0f;
    pSprite->fS1 = 1.0f;
    pSprite->fT0 = 0.0f;
    pSprite->fT1 = 1.0f;
    pSprite->nTexSize = pSprite->nTexHeight * pSprite->nTexWidth;
}

// Mac 002e34d7
BOOL OGL_SpriteBindTexture(const OGLSprite *pSprite, BYTE *pPalette, BYTE nOutlineColor, int nTexMode)
{
    DC6Block *pBlock = OGL_GfxDataBlock(pSprite->pData);
    if (!pBlock)
        OGL_HALT(269);

    int nSlot = 1;
    if (nTexMode != 4)
        nSlot = nTexMode;
    OGLSpriteTexSlot *pSlots = static_cast<OGLSpriteTexSlot *>(g_host.GetDC6BlockPixelData(pBlock));

    if (pSlots[nSlot].nSizeClass == 0) {
        int nWidth = pSprite->nWidth;
        if (nWidth <= 32 && pSprite->nHeight <= 32) {
            pSlots[nSlot].nSizeClass = 3;
        } else if (nWidth <= 128 && pSprite->nHeight <= 128) {
            pSlots[nSlot].nSizeClass = 2;
        } else {
            if (nWidth > 256)
                return FALSE;
            if (pSprite->nHeight > 256)
                return FALSE;
            pSlots[nSlot].nSizeClass = 1;
        }
    }

    int nTextureId = pSlots[nSlot].nTextureId;
    if (nTextureId) {
        if (g_pTextures->m_pNodes[nTextureId - 1].nOwnerKey == (int)(intptr_t)pPalette || nTexMode == 4) {
            g_pTextures->BindTextureById(nTextureId);
            return TRUE;
        }
        g_pTextures->FreeTextureById(nTextureId);
        pSlots[nSlot].nTextureId = 0;
    }

    if (!g_pSpriteBuffer)
        OGL_HALT(369);
    if (pSprite->nTexSize > 0x10000)
        OGL_HALT(370);
    memset(g_pSpriteBuffer, 0, pSprite->nTexSize);
    int nPitch = pSprite->nTexWidth;
    int nTexHeight = pSprite->nTexHeight;
    OGL_IsPerspectiveEnabled();
    if (nPitch & 3)
        nPitch = (nPitch | 3) + 1;

    BYTE *pUpload = g_pSpriteBuffer;
    switch (nSlot) {
    case 1:
    case 4:
        g_host.DrawCelFrame(pSprite->pData, -pSprite->nOffsetX, nTexHeight - 1 - pSprite->nOffsetY, 0, 0,
                            g_pSpriteBuffer, nTexHeight, nPitch, 0, 0, pPalette);
        break;
    case 2: {
        g_host.DrawCelFrame(pSprite->pData, -pSprite->nOffsetX, nTexHeight - 1 - pSprite->nOffsetY, 0, 0,
                            g_pSpriteBuffer, nTexHeight, nPitch, 0, 0, pPalette);
        if (pSprite->nTexSize > 0) {
            const BYTE *pRemap = g_pPaletteTable[D2PALETTE_INDEX_REMAP];
            int i = 0;
            do {
                if (pUpload[i])
                    pUpload[i] = pRemap[pUpload[i]];
                ++i;
            } while (i < pSprite->nTexSize);
        }
        break;
    }
    case 3: {
        g_host.DrawCelFrame(pSprite->pData, 1 - pSprite->nOffsetX, nTexHeight - 2 - pSprite->nOffsetY, 0, 0,
                            g_pSpriteBuffer, nTexHeight, nPitch, 0, 0, NULL);
        if (!g_pSpriteOutlineBuffer)
            OGL_HALT(443);
        if (pSprite->nTexSize > 0x10000)
            OGL_HALT(444);
        memset(g_pSpriteOutlineBuffer, 0, pSprite->nTexSize);
        if (nPitch * nTexHeight - nPitch * 2 > 0) {
            int nCount = (nTexHeight - 2) * nPitch;
            const BYTE *pSrc = g_pSpriteBuffer + nPitch;
            BYTE *pDst = g_pSpriteOutlineBuffer + nPitch;
            do {
                if (*pSrc++) {
                    pDst[-nPitch] = nOutlineColor;
                    pDst[-1] = nOutlineColor;
                    pDst[1] = nOutlineColor;
                    pDst[nPitch] = nOutlineColor;
                }
                ++pDst;
            } while (--nCount);
        }
        pUpload = g_pSpriteOutlineBuffer;
        break;
    }
    default:
        break;
    }

    // Upscale: the texture is made from the index image magnified in index space, or from the HD
    // pack (upscale/sprite_upscale.h); its texture coordinates stay the same fractions.
    int nUploadWidth = nPitch;
    int nUploadHeight = nTexHeight;
    const BYTE *pScaled = Upscale_Sprite(pSprite, pUpload, nSlot, pPalette, &nUploadWidth, &nUploadHeight);
    g_pTextures->CreateTexture(pScaled, nUploadWidth, nUploadHeight, TEXOWNER_SPRITE, pSlots, nSlot,
                               (int)(intptr_t)pPalette);
    OGL_SetTextureFilter(OGL_FILTER_NEAREST);
    g_pTextures->BindTextureById(pSlots[nSlot].nTextureId);
    return TRUE;
}

// Mac 002e3934
BYTE OGL_SetDrawModeBlend(int nDrawMode)
{
    switch (nDrawMode) {
    case 3:
        OGL_SetBlendMode(OGL_BLEND_ADD);
        return 0xff;
    case 4:
        OGL_SetBlendMode(OGL_BLEND_MUL);
        return 0xff;
    case 5:
    case 7:
        OGL_SetBlendMode(OGL_BLEND_OFF);
        return 0xff;
    default: {
        BYTE nAlpha = 0x80;
        if (nDrawMode != 1)
            nAlpha = (nDrawMode == 0) ? 0x40 : 0xff;
        BYTE nResult = 0xc0;
        if (nDrawMode != 2)
            nResult = nAlpha;
        OGL_SetBlendMode(OGL_BLEND_ALPHA);
        return nResult;
    }
    }
}

// Mac 002e39bc OGL_DrawImage
void OGL_DrawImage(D2GfxDataStrc *pData, int nX, int nY, DWORD dwGamma, int nDrawMode, BYTE *pPalette)
{
    OGLSprite spr;
    if (!OGL_SpriteLoad(pData, &spr))
        return;
    if (spr.nOffsetX + nX > g_nScreenWidth)
        return;
    if (spr.nOffsetX + nX + spr.nWidth < 0)
        return;
    if (spr.nOffsetY + nY < 0)
        return;
    if (spr.nOffsetY + nY - spr.nHeight >= g_nScreenHeight)
        return;
    OGL_SpriteCalcTextureSize(&spr);
    BYTE nAlpha = OGL_SetDrawModeBlend(nDrawMode);
    if (OGL_SpriteBindTexture(&spr, pPalette, 0, nDrawMode == 7 ? 2 : 1))
        OGL_DrawSpriteQuad(&spr, nX, nY, dwGamma, 0, nAlpha, FALSE, 0, 0);
}

// Mac 002e3ab2
void OGL_DrawSpriteQuad(const OGLSprite *pSprite, int nX, int nY, DWORD dwGamma, int nColorIndex, BYTE nAlpha,
                        BOOL bUseColorIndex, int nTexWidth, int nTexHeight)
{
    OGL_SetTextureMode(OGL_TEXMODE_TEXTURED);
    if (nColorIndex | bUseColorIndex) {
        const BYTE *pRGB = g_pPaletteTable[D2PALETTE_STANDARD_COLORS] + nColorIndex * 3;
        BYTE nBlue = pRGB[2];
        BYTE nRed = pRGB[0];
        BYTE nGreen = pRGB[1];
        g_aSpriteQuad[0].r = nRed;
        g_aSpriteQuad[0].g = nGreen;
        g_aSpriteQuad[0].b = nBlue;
    } else {
        OGL_SetVertexColorFromLight(&g_aSpriteQuad[0], dwGamma);
    }
    g_aSpriteQuad[0].a = nAlpha;

    int nOffsetX = pSprite->nOffsetX;
    float fT1 = pSprite->fT1;
    float fT0 = pSprite->fT0;
    float fS1 = pSprite->fS1;
    float fS0 = pSprite->fS0;
    OGL_IsPerspectiveEnabled();
    int nHeight = pSprite->nTexHeight;
    int nWidth = pSprite->nTexWidth;
    int nOffsetY = pSprite->nOffsetY - nHeight;
    if (OGL_IsPerspectiveEnabled()) {
        int nFactor = g_nPerspectiveFactor;
        if (nFactor != 0x100) {
            nHeight = nHeight * nFactor / 256;
            nWidth = nWidth * nFactor / 256;
            nOffsetY = nOffsetY * nFactor / 256;
            nOffsetX = nOffsetX * nFactor / 256;
        }
    }

    float fTop = (float)(nOffsetY + nY);
    g_aSpriteQuad[0].fX = (float)(nOffsetX + nX);
    g_aSpriteQuad[2].fX = (float)(nOffsetX + nX + nWidth);
    if (!g_bTextureRectangle) {
        g_aSpriteQuad[1].fU = fS0;
    } else {
        if (!nTexWidth)
            nTexHeight = pSprite->nTexHeight;
        if (!nTexWidth)
            nTexWidth = pSprite->nTexWidth;
        fT0 = fT0 * (float)nTexHeight;
        fT1 = fT1 * (float)nTexHeight;
        fS1 = fS1 * (float)nTexWidth;
        g_aSpriteQuad[1].fU = fS0 * (float)nTexWidth;
    }
    int nScreenHeight = g_nScreenHeight;
    g_aSpriteQuad[2].fY = (float)nScreenHeight - (float)(nOffsetY + nY + nHeight);
    g_aSpriteQuad[0].fY = (float)nScreenHeight - fTop;
    g_aSpriteQuad[3].fY = (float)nScreenHeight - fTop;
    g_aSpriteQuad[0].fU = g_aSpriteQuad[1].fU;
    g_aSpriteQuad[0].fV = fT0;
    g_aSpriteQuad[1].fX = g_aSpriteQuad[0].fX;
    g_aSpriteQuad[1].fY = g_aSpriteQuad[2].fY;
    g_aSpriteQuad[1].fV = fT1;
    g_aSpriteQuad[2].fU = fS1;
    g_aSpriteQuad[2].fV = fT1;
    g_aSpriteQuad[3].fX = g_aSpriteQuad[2].fX;
    g_aSpriteQuad[3].fU = fS1;
    g_aSpriteQuad[3].fV = fT0;

    glColor4ubv(&g_aSpriteQuad[0].r);
    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(g_aSpriteQuad[0].fU, g_aSpriteQuad[0].fV);
    glVertex2f(g_aSpriteQuad[0].fX, g_aSpriteQuad[0].fY);
    glTexCoord2f(g_aSpriteQuad[1].fU, g_aSpriteQuad[1].fV);
    glVertex2f(g_aSpriteQuad[1].fX, g_aSpriteQuad[1].fY);
    glTexCoord2f(g_aSpriteQuad[2].fU, g_aSpriteQuad[2].fV);
    glVertex2f(g_aSpriteQuad[2].fX, g_aSpriteQuad[2].fY);
    glTexCoord2f(g_aSpriteQuad[3].fU, g_aSpriteQuad[3].fV);
    glVertex2f(g_aSpriteQuad[3].fX, g_aSpriteQuad[3].fY);
    glEnd();
}

// Mac 002e3e98 OGL_DrawShiftedImage
void OGL_DrawShiftedImage(D2GfxDataStrc *pData, int nX, int nY, DWORD dwGamma, int nDrawMode, int nGlobalPaletteShift)
{
    OGLSprite spr;
    if (!OGL_SpriteLoad(pData, &spr))
        return;
    if (spr.nOffsetX + nX > g_nScreenWidth)
        return;
    if (spr.nOffsetX + nX + spr.nWidth < 0)
        return;
    if (spr.nOffsetY + nY < 0)
        return;
    if (spr.nOffsetY + nY - spr.nHeight >= g_nScreenHeight)
        return;
    OGL_SpriteCalcTextureSize(&spr);
    BYTE nAlpha = OGL_SetDrawModeBlend(nDrawMode);
    if (OGL_SpriteBindTexture(&spr, NULL, 0, 1))
        OGL_DrawSpriteQuad(&spr, nX, nY, dwGamma, nGlobalPaletteShift, nAlpha, FALSE, 0, 0);
}

// Mac 002e3f6c OGL_DrawVerticalCropImage
void OGL_DrawVerticalCropImage(D2GfxDataStrc *pData, int nX, int nY, int nSkipLines, int nDrawLines, int nDrawMode)
{
    OGLSprite spr;
    if (!OGL_SpriteLoad(pData, &spr))
        return;
    OGL_SpriteCalcTextureSize(&spr);
    if (!OGL_SpriteBindTexture(&spr, NULL, 0, 1))
        return;
    BYTE nAlpha = OGL_SetDrawModeBlend(nDrawMode);
    OGL_IsPerspectiveEnabled();
    int nTexHeight = spr.nTexHeight;
    if (nSkipLines) {
        spr.fT0 = (float)(uint32_t)(nTexHeight - (nDrawLines + nSkipLines)) / (float)nTexHeight;
        spr.fT1 = (float)(uint32_t)(nTexHeight - nSkipLines) / (float)nTexHeight;
        nY -= nSkipLines;
    } else {
        spr.fT0 = (float)(uint32_t)(nTexHeight - nDrawLines) / (float)nTexHeight;
    }
    int nTexWidth = spr.nTexWidth;
    spr.nTexHeight = nDrawLines;
    OGL_DrawSpriteQuad(&spr, nX, nY, 0xffffffff, 0, nAlpha, FALSE, nTexWidth, nTexHeight);
}

// Mac 002e40a2 OGL_DrawClippedImage
void OGL_DrawClippedImage(D2GfxDataStrc *pData, int nX, int nY, const D2GfxTileRectStrc *pCropRect, int nDrawMode)
{
    BYTE nAlpha = OGL_SetDrawModeBlend(nDrawMode);
    OGLSprite spr;
    if (!OGL_SpriteLoad(pData, &spr))
        return;
    OGL_SpriteCalcTextureSize(&spr);
    if (!OGL_SpriteBindTexture(&spr, NULL, 0, 1))
        return;

    int nLeft = 0;
    if (pCropRect->nLeft >= 0)
        nLeft = pCropRect->nLeft;
    int nRight = g_nScreenWidth;
    if (pCropRect->nRight <= nRight)
        nRight = pCropRect->nRight;
    if (nLeft > nRight)
        return;
    int nX0 = spr.nOffsetX + nX;
    if (nX0 >= nRight)
        return;
    int nWidth = spr.nWidth;
    int nX1 = nX0 + nWidth;
    if (nX1 < nLeft)
        return;

    int nTop = pCropRect->nTop + 2;
    if (nTop < 0)
        nTop = 0;
    int nBottom = g_nScreenHeight;
    if (pCropRect->nBottom <= nBottom)
        nBottom = pCropRect->nBottom;
    if (nTop > nBottom)
        return;
    int nYBottom = spr.nOffsetY + nY;
    if (nYBottom < nTop)
        return;
    int nHeight = spr.nHeight;
    int nYTop = nYBottom - nHeight;
    if (nYTop > nBottom)
        return;

    int nTexHeight = spr.nTexHeight;
    int nTexWidth = spr.nTexWidth;
    int nClipLeft = nLeft - nX0;
    if (nClipLeft <= 0)
        nClipLeft = 0;
    int nClipBottom = 0;
    int nNewWidth = nWidth - nClipLeft;
    if (nX1 >= nRight)
        nNewWidth += nRight - nX1;
    if (nYBottom > nBottom)
        nClipBottom = nYBottom - 1 - nBottom;
    if (nYTop < nTop)
        nHeight = nYBottom + 1 - nTop;
    int nVisibleHeight = nHeight - nClipBottom;
    if (nVisibleHeight <= 0)
        OGL_HALT(1306);

    float fInvWidth = 1.0f / (float)nTexWidth;
    float fInvHeight = 1.0f / (float)nTexHeight;
    OGL_IsPerspectiveEnabled();
    if (nClipBottom) {
        spr.fT0 = (float)(nTexHeight - nHeight) * fInvHeight;
        spr.fT1 = (float)(nTexHeight - nClipBottom) * fInvHeight;
        nY -= nClipBottom;
    } else {
        spr.fT0 = fInvHeight * (float)(nTexHeight - nVisibleHeight);
    }
    spr.nTexHeight = nVisibleHeight;
    if (nClipLeft) {
        spr.fS0 = (float)nClipLeft * fInvWidth;
        spr.fS1 = (float)(nNewWidth + nClipLeft) * fInvWidth;
        nX += nClipLeft;
    } else {
        spr.fS1 = (float)nNewWidth * fInvWidth;
    }
    spr.nTexWidth = nNewWidth;
    OGL_DrawSpriteQuad(&spr, nX, nY, 0xffffffff, 0, nAlpha, FALSE, nTexWidth, nTexHeight);
}

// Mac 002e4332 OGL_DrawShadow
void OGL_DrawShadow(D2GfxDataStrc *pData, int nX, int nY)
{
    OGLSprite spr;
    if (!OGL_SpriteLoad(pData, &spr))
        return;
    OGL_SpriteCalcTextureSize(&spr);
    if (!OGL_SpriteBindTexture(&spr, NULL, 0, 4))
        return;
    OGL_SetTextureMode(OGL_TEXMODE_TEXTURED);
    BYTE nAlpha;
    if (OGL_GetBlendedShadows()) {
        OGL_SetBlendMode(OGL_BLEND_ALPHA);
        nAlpha = 0xc0;
    } else {
        OGL_SetBlendMode(OGL_BLEND_OFF);
        nAlpha = 0xff;
    }

    float fT1 = spr.fT1;
    float fT0 = spr.fT0;
    float fS0 = spr.fS0;
    float fS1 = spr.fS1;
    OGL_IsPerspectiveEnabled();
    int nTexHeight = spr.nTexHeight;
    int nHalfHeight = nTexHeight >> 1;
    int nTexWidth = spr.nTexWidth;
    spr.nOffsetY >>= 1;
    int nOffsetY = spr.nOffsetY - nHalfHeight;
    int nOffsetX = spr.nOffsetY + spr.nOffsetX;
    int nWidth = nTexWidth;
    if (OGL_IsPerspectiveEnabled()) {
        int nFactor = g_nPerspectiveFactor;
        if (nFactor != 0x100) {
            nHalfHeight = nHalfHeight * nFactor / 256;
            nWidth = nFactor * nTexWidth / 256;
            nOffsetY = nOffsetY * nFactor / 256;
            nOffsetX = nOffsetX * nFactor / 256;
        }
    }

    int nLeft = nOffsetX + nX;
    float fTop = (float)(nOffsetY + nY);
    float fBottom = (float)(nOffsetY + nY + nHalfHeight);
    int nScreenHeight = g_nScreenHeight;
    if (g_bTextureRectangle) {
        fS0 = fS0 * (float)nTexWidth;
        fT0 = fT0 * (float)nTexHeight;
        fT1 = fT1 * (float)nTexHeight;
        fS1 = fS1 * (float)nTexWidth;
    }
    g_aSpriteQuad[0].fX = (float)(nLeft - nHalfHeight);
    g_aSpriteQuad[0].fY = (float)nScreenHeight - fTop;
    g_aSpriteQuad[0].fU = fS0;
    g_aSpriteQuad[0].fV = fT0;
    g_aSpriteQuad[1].fX = (float)nLeft;
    g_aSpriteQuad[1].fY = (float)nScreenHeight - fBottom;
    g_aSpriteQuad[1].fU = fS0;
    g_aSpriteQuad[1].fV = fT1;
    g_aSpriteQuad[2].fX = (float)(nWidth + nLeft);
    g_aSpriteQuad[2].fY = g_aSpriteQuad[1].fY;
    g_aSpriteQuad[2].fU = fS1;
    g_aSpriteQuad[2].fV = fT1;
    g_aSpriteQuad[3].fX = (float)(nWidth + nLeft - nHalfHeight);
    g_aSpriteQuad[3].fY = (float)nScreenHeight - fTop;
    g_aSpriteQuad[3].fU = fS1;
    g_aSpriteQuad[3].fV = fT0;
    g_aSpriteQuad[0].r = 0;
    g_aSpriteQuad[0].g = 0;
    g_aSpriteQuad[0].b = 0;
    g_aSpriteQuad[0].a = nAlpha;

    glColor4ubv(&g_aSpriteQuad[0].r);
    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(g_aSpriteQuad[0].fU, g_aSpriteQuad[0].fV);
    glVertex2f(g_aSpriteQuad[0].fX, g_aSpriteQuad[0].fY);
    glTexCoord2f(g_aSpriteQuad[1].fU, g_aSpriteQuad[1].fV);
    glVertex2f(g_aSpriteQuad[1].fX, g_aSpriteQuad[1].fY);
    glTexCoord2f(g_aSpriteQuad[2].fU, g_aSpriteQuad[2].fV);
    glVertex2f(g_aSpriteQuad[2].fX, g_aSpriteQuad[2].fY);
    glTexCoord2f(g_aSpriteQuad[3].fU, g_aSpriteQuad[3].fV);
    glVertex2f(g_aSpriteQuad[3].fX, g_aSpriteQuad[3].fY);
    glEnd();
}

// Mac 002e476f OGL_DrawImageFast
void OGL_DrawImageFast(D2GfxDataStrc *pData, int nX, int nY, BYTE nPaletteIndex)
{
    OGL_SetBlendMode(OGL_BLEND_ALPHA);
    OGLSprite spr;
    if (!OGL_SpriteLoad(pData, &spr))
        return;
    spr.nWidth += 2;
    spr.nHeight += 2;
    OGL_SpriteCalcTextureSize(&spr);
    if (OGL_SpriteBindTexture(&spr, NULL, nPaletteIndex, 3))
        OGL_DrawSpriteQuad(&spr, nX - 1, nY + 1, 0xffffffff, 0, 0x40, TRUE, 0, 0);
}

// Mac 002e480c OGL_DebugFillBackBuffer
void OGL_DebugFillBackBuffer(int nX, int nY)
{
    (void)nX;
    (void)nY;
}
