// Screen read, palette, texture/blend/filter state, strings, rects, lines and clear
// (Mac 002df9e2-002e07ec).

#include <stdio.h>

#include "renderer.h"

// Mac 002df9e2
BOOL OGL_GetBackBuffer(BYTE *pBuffer)
{
    int nRow = g_nScreenHeight;
    if (nRow > 0) {
        int nWidth = g_nScreenWidth;
        int nPitch = nWidth * 3;
        do {
            --nRow;
            glReadPixels(0, nRow, nWidth, 1, GL_RGB, GL_UNSIGNED_BYTE, pBuffer);
            if (glGetError() != GL_NO_ERROR)
                OGL_HALT(694);
            pBuffer += nPitch;
        } while (nRow > 0);
    }
    return TRUE;
}

// Mac 002dfa7d OGL_GetScreenSize
void OGL_GetScreenSize(int *pWidth, int *pHeight)
{
    *pWidth = g_nScreenWidth;
    *pHeight = g_nScreenHeight;
}

// Mac 002dfa9e OGL_SetPalette
void OGL_SetPalette(const PALETTEENTRY *pPalette)
{
    for (int i = 0; i < 256; i++) {
        g_aPaletteRGB[i][0] = pPalette[i].peRed;
        g_aPaletteRGB[i][1] = pPalette[i].peGreen;
        g_aPaletteRGB[i][2] = pPalette[i].peBlue;
        g_aPaletteRGBX[i][0] = pPalette[i].peRed;
        g_aPaletteRGBX[i][1] = pPalette[i].peGreen;
        g_aPaletteRGBX[i][2] = pPalette[i].peBlue;
        g_aPaletteRGBA[i] = (uint32_t)pPalette[i].peRed | ((uint32_t)pPalette[i].peGreen << 8) |
                            ((uint32_t)pPalette[i].peBlue << 16) | 0xff000000u;
        g_aPalette1555[i] = (uint16_t)(((uint32_t)(pPalette[i].peRed >> 3) << 10) |
                                       (((uint32_t)pPalette[i].peGreen << 2) & 0x3e0) |
                                       ((uint32_t)pPalette[i].peBlue >> 3) | 0x8000);
        if (pPalette[i].peRed == 0 && pPalette[i].peGreen == 0 && pPalette[i].peBlue == 0) {
            g_aPaletteRGBA[i] = 0;
            g_aPalette1555[i] = 0;
            g_aPaletteRGBX[i][3] = 0;
        }
    }
}

// Mac 002dfb94
uint32_t OGL_GetPaletteColor(int nIndex)
{
    return g_aPaletteRGBA[nIndex];
}

// Mac 002dfbab OGL_SetPaletteTable
void OGL_SetPaletteTable(BYTE **pPaletteTable)
{
    memcpy(g_aPaletteTable, pPaletteTable, 0x120);
    g_pPaletteTable = g_aPaletteTable;
}

// Mac 002dfbe3
BOOL OGL_ResizeWindow(HWND hWnd, int nResolutionMode)
{
    (void)hWnd;
    BOOL bResult = TRUE;
    if (nResolutionMode == 2)
        nResolutionMode = 1;
    OGL_GammaBlack();
    OGL_StopCutscene();
    OGL_CloseOpenGLWindow();
    int16_t nError = OGL_PlatformSwapContext(nResolutionMode, 0, 0);
    if (nError != 0) {
        g_host.LogWrite("OpenGLSwapContext: *** OpenGLMacSwapContext failed. (%d)", (int)nError);
        OGL_ApplyGamma();
        bResult = FALSE;
    } else {
        OGL_OpenOpenGLWindow(nResolutionMode);
        OGL_ClearDisplay();
        OGL_ApplyGamma();
    }
    return bResult;
}

// Mac 002dfc65
void OGL_SetTextureMode(int nMode)
{
    if (g_nCurTextureMode == nMode)
        return;
    if (nMode == OGL_TEXMODE_UNTEXTURED) {
        if (g_bTextureRectangle)
            glDisable(GL_TEXTURE_RECTANGLE_ARB);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_ALPHA_TEST);
    } else if (nMode == OGL_TEXMODE_TEXTURED) {
        if (g_bTextureRectangle)
            glEnable(GL_TEXTURE_RECTANGLE_ARB);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_ALPHA_TEST);
    }
    g_nCurTextureMode = nMode;
}

// Mac 002dfcf5
void OGL_SetBlendMode(int nBlendMode)
{
    if (g_nCurBlendMode != OGL_BLEND_UNSET && g_nCurBlendMode == nBlendMode)
        return;
    switch (nBlendMode) {
    case OGL_BLEND_OFF:
        glDisable(GL_BLEND);
        break;
    case OGL_BLEND_ADD:
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        break;
    case OGL_BLEND_MUL:
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        break;
    case OGL_BLEND_ALPHA:
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    default:
        break;
    }
    g_nCurBlendMode = nBlendMode;
}

// Mac 002dfdb0
void OGL_SetTextureFilter(int nFilter)
{
    if (nFilter == OGL_FILTER_LINEAR) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 9729.0f);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 9729.0f);
    } else if (nFilter == OGL_FILTER_NEAREST) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 9728.0f);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 9728.0f);
    }
}

// Mac 002dfe26 OGL_GetRenderStatistics
D2RenderStatisticsStrc *OGL_GetRenderStatistics(void)
{
    return NULL;
}

// Mac 002dfe2d
void OGL_OpenWindowHook2(void)
{
}

// Mac 002dfe32
void OGL_CloseWindowHook1(void)
{
}

// Mac 002dfe37
void OGL_OpenWindowHook1(void)
{
}

// Mac 002dfe3c
void OGL_CloseWindowHook2(void)
{
}

// Mac 002dfe41 OGL_DrawString
void OGL_DrawString(int nX, int nY, const char *szFormat, va_list va)
{
    char szText[0x200];
    wchar_t wszText[0x200];

    vsprintf(szText, szFormat, va);
    memset(wszText, 0, sizeof(wszText));
    g_host.Utf8ToWide(wszText, szText, 0x200);
    int nOldFont = g_host.SetFont(13);
    g_host.DrawGameText(wszText, nX, nY + 12, 0, FALSE);
    g_host.SetFont(nOldFont);
}

// Mac 002dfefa OGL_DrawLight
void OGL_DrawLight(DWORD *, DWORD *, int, int)
{
}

// Mac 002dfeff
BOOL OGL_GetBlendedShadows(void)
{
    return g_pGfxSettings->bBlendedShadows;
}

// Mac 002dff15 OGL_DrawSolidRectEx
void OGL_DrawSolidRectEx(int nX0, int nY0, int nX1, int nY1, DWORD dwColor, int nDrawMode)
{
    int nMaxX = g_nScreenWidth - 1;
    if (nX1 < 0)
        nX1 = 0;
    if (nX1 > nMaxX)
        nX1 = nMaxX;
    if (nX0 < 0)
        nX0 = 0;
    if (nX0 > nMaxX)
        nX0 = nMaxX;
    if (nX0 == nX1)
        return;

    int nMaxY = g_nScreenHeight - 1;
    if (nY1 < 0)
        nY1 = 0;
    if (nY1 > nMaxY)
        nY1 = nMaxY;
    if (nY0 < 0)
        nY0 = 0;
    if (nY0 > nMaxY)
        nY0 = nMaxY;
    if (nY0 == nY1)
        return;

    if (nY1 <= nY0)
        OGL_HALT(41);

    OGL_SetTextureMode(OGL_TEXMODE_UNTEXTURED);
    OGL_SetBlendMode(OGL_BLEND_ALPHA);
    int nScreenHeight = g_nScreenHeight;
    uint32_t dwRGBA = OGL_GetPaletteColor((BYTE)dwColor);

    BYTE nAlpha;
    if (nDrawMode == 0)
        nAlpha = 0x40;
    else if (nDrawMode == 2)
        nAlpha = 0xc0;
    else if (nDrawMode == 1)
        nAlpha = 0x80;
    else
        nAlpha = 0xff;

    float fBottom = (float)(nScreenHeight - nY1);
    float fTop = (float)(nScreenHeight - nY0);
    float fX1 = (float)nX1;
    float fX0 = (float)nX0;
    glColor4ub((GLubyte)dwRGBA, (GLubyte)(dwRGBA >> 8), (GLubyte)(dwRGBA >> 16), nAlpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(fX0, fTop);
    glVertex2f(fX0, fBottom);
    glVertex2f(fX1, fBottom);
    glVertex2f(fX1, fTop);
    glEnd();
}

// Mac 002e00c0 OGL_DrawSolidRectAlpha
void OGL_DrawSolidRectAlpha(int nX0, int nY0, int nX1, int nY1, DWORD dwColor, BYTE nAlpha)
{
    int nMaxX = g_nScreenWidth - 1;
    if (nX1 < 0)
        nX1 = 0;
    if (nX1 > nMaxX)
        nX1 = nMaxX;
    if (nX0 < 0)
        nX0 = 0;
    if (nX0 > nMaxX)
        nX0 = nMaxX;
    if (nX0 == nX1)
        return;

    int nMaxY = g_nScreenHeight - 1;
    if (nY1 < 0)
        nY1 = 0;
    if (nY1 > nMaxY)
        nY1 = nMaxY;
    if (nY0 < 0)
        nY0 = 0;
    if (nY0 > nMaxY)
        nY0 = nMaxY;
    if (nY0 == nY1)
        return;

    if (nY1 <= nY0)
        OGL_HALT(99);

    OGL_SetTextureMode(OGL_TEXMODE_UNTEXTURED);
    OGL_SetBlendMode(OGL_BLEND_ALPHA);
    int nScreenHeight = g_nScreenHeight;
    uint32_t dwRGBA = OGL_GetPaletteColor((BYTE)dwColor);
    glColor4ub((GLubyte)dwRGBA, (GLubyte)(dwRGBA >> 8), (GLubyte)(dwRGBA >> 16), nAlpha);

    float fTop = (float)(nScreenHeight - nY0);
    glBegin(GL_TRIANGLE_FAN);
    float fX0 = (float)nX0;
    float fBottom = (float)(nScreenHeight - nY1);
    glVertex2f(fX0, fTop);
    glVertex2f(fX0, fBottom);
    float fX1 = (float)nX1;
    glVertex2f(fX1, fBottom);
    glVertex2f(fX1, fTop);
    glEnd();
}

// Mac 002e0245 OGL_DrawLine
void OGL_DrawLine(int nX0, int nY0, int nX1, int nY1, DWORD dwColor, BYTE nAlpha)
{
    if ((nX0 & nX1) < 0)
        return;
    if (g_nScreenWidth <= nX1 && g_nScreenWidth <= nX0)
        return;
    if ((nY0 & nY1) < 0)
        return;
    if (g_nScreenHeight <= nY1 && g_nScreenHeight <= nY0)
        return;

    uint32_t dwRGBA = OGL_GetPaletteColor((BYTE)dwColor);
    OGL_SetTextureMode(OGL_TEXMODE_UNTEXTURED);
    OGL_SetBlendMode(nAlpha == 0xff ? OGL_BLEND_OFF : OGL_BLEND_ALPHA);
    float fY0 = (float)nY0;
    float fX0 = (float)nX0;
    glColor4ub((GLubyte)dwRGBA, (GLubyte)(dwRGBA >> 8), (GLubyte)(dwRGBA >> 16), nAlpha);

    if (nX0 == nX1 && nY0 == nY1) {
        float fY = (float)g_nScreenHeight - fY0;
        glBegin(GL_POINTS);
        glVertex2f(fX0, fY);
    } else {
        float fScreenHeight = (float)g_nScreenHeight;
        float fY = fScreenHeight - fY0;
        glBegin(GL_LINES);
        glVertex2f(fX0, fY);
        glVertex2f((float)nX1, fScreenHeight - (float)nY1);
    }
    glEnd();
}

// Mac 002e03c0 OGL_DrawRect
void OGL_DrawRect(RECT *pRect, BYTE nPaletteIndex)
{
    if (pRect->left > pRect->right)
        OGL_HALT(182);
    if (pRect->top > pRect->bottom)
        OGL_HALT(183);

    int nLeft = pRect->left;
    int nRight = pRect->right;
    int nTop = pRect->top;
    int nBottom = pRect->bottom;
    int nWidth = nRight - nLeft;
    if (nWidth == 0)
        return;
    int nHeight = nBottom - nTop;
    if (nHeight == 0)
        return;
    if (nRight < 0 || nLeft > g_nScreenWidth)
        return;
    if (nBottom < 0 || nTop > g_nScreenHeight)
        return;

    int nMidY = (int)((unsigned)nHeight >> 1) + nTop;
    int nMidX = (int)((unsigned)nWidth >> 1) + nLeft;
    OGL_DrawLine(nMidX, nTop, nRight, nMidY, nPaletteIndex, 0xff);
    OGL_DrawLine(nRight, nMidY, nMidX, nBottom, nPaletteIndex, 0xff);
    OGL_DrawLine(nMidX, nBottom, nLeft, nMidY, nPaletteIndex, 0xff);
    OGL_DrawLine(nLeft, nMidY, nMidX, nTop, nPaletteIndex, 0xff);
}

// Mac 002e0534 OGL_DrawRectEx
void OGL_DrawRectEx(RECT *pRect, BYTE nPaletteIndex)
{
    if (pRect->left > pRect->right)
        OGL_HALT(220);
    if (pRect->top > pRect->bottom)
        OGL_HALT(221);

    int nMaxX = g_nScreenWidth - 1;
    int nRight = pRect->right;
    if (nRight < 0)
        nRight = 0;
    if (nRight > nMaxX)
        nRight = nMaxX;
    int nLeft = pRect->left;
    if (nLeft < 0)
        nLeft = 0;
    if (nLeft > nMaxX)
        nLeft = nMaxX;
    if (nLeft == nRight)
        return;

    int nMaxY = g_nScreenHeight - 1;
    int nBottom = pRect->bottom;
    if (nBottom < 0)
        nBottom = 0;
    if (nBottom > nMaxY)
        nBottom = nMaxY;
    int nTop = pRect->top;
    if (nTop < 0)
        nTop = 0;
    if (nTop > nMaxY)
        nTop = nMaxY;
    if (nTop == nBottom)
        return;

    if (nBottom <= nTop)
        OGL_HALT(237);

    OGL_DrawLine(nLeft, nTop, nRight, nTop, nPaletteIndex, 0xff);
    OGL_DrawLine(nRight, nTop, nRight, nBottom, nPaletteIndex, 0xff);
    OGL_DrawLine(nRight, nBottom, nLeft, nBottom, nPaletteIndex, 0xff);
    OGL_DrawLine(nLeft, nBottom, nLeft, nTop, nPaletteIndex, 0xff);
}

// Mac 002e06bd OGL_DrawSolidRect
void OGL_DrawSolidRect(RECT *pRect, BYTE nPaletteIndex)
{
    if (pRect->left > pRect->right)
        OGL_HALT(253);
    if (pRect->top > pRect->bottom)
        OGL_HALT(254);
    OGL_DrawSolidRectEx(pRect->left, pRect->top, pRect->right, pRect->bottom, nPaletteIndex, 5);
}

// Mac 002e0757 OGL_DrawSolidSquare
void OGL_DrawSolidSquare(POINT *pPoint, BYTE nSize, BYTE nPaletteIndex)
{
    if (nSize == 0)
        return;
    RECT rc;
    int nHalf = (BYTE)(nSize - 1);
    SetRect(&rc, pPoint->x - nHalf, pPoint->y - nHalf, pPoint->x + nHalf + 1, pPoint->y + nHalf + 1);
    OGL_DrawSolidRectEx(rc.left, rc.top, rc.right, rc.bottom, nPaletteIndex, 5);
}

// Mac 002e07d5 OGL_ClearScreen
void OGL_ClearScreen(BOOL)
{
    glClear(GL_COLOR_BUFFER_BIT);
}

// Mac 002e07ec
void OGL_InvalidateGroundMesh(void)
{
    g_bGroundMeshDirty = true;
}
