// The ground tile, oglVertex (the modulate table and the vertex colours), the global light, the shadow
// tile and the wall tiles (Mac 002e07fe, 002e4811-002e5ea1).

#include "renderer.h"

// Left shift of a possibly negative value into 16.16 / 19.13 fixed point, with the Mac's wrapping.
static inline int ShiftLeft(int nValue, int nBits)
{
    return (int)((uint32_t)nValue << nBits);
}

// The perspective visibility window the shadow and wall tiles test each corner against.
static inline bool IsInPerspectiveWindow(const OGLVertex &vertex, float fClipLeft, float fClipRight,
                                         float fScreenHeight)
{
    return vertex.fX >= fClipLeft && fClipRight > vertex.fX && vertex.fY >= 47.0f && fScreenHeight > vertex.fY;
}

// Mac 002e07fe OGL_DrawGroundTile
BOOL OGL_DrawGroundTile(D2TileLibraryEntryStrc *pTile, D2GfxLightExStrc *pLight, int /*nX*/, int /*nY*/,
                        int /*nWorldX*/, int /*nWorldY*/, BYTE nAlpha, int /*nScreenPanels*/,
                        BOOL /*bFilterFloor*/)
{
    if (g_bGroundMeshDirty) {
        g_bGroundMeshDirty = false;

        bool bTexelUV;
        float fTexHeight;
        float fTexWidth;
        if (g_bAGPTextures) {
            bTexelUV = g_bTextureRectangle != 0;
            fTexHeight = g_bTextureRectangle ? 80.0f : 128.0f;
            fTexWidth = g_bTextureRectangle ? 160.0f : 256.0f;
        } else {
            bTexelUV = false;
            fTexHeight = 128.0f;
            fTexWidth = 256.0f;
        }
        const float fStepV = 80.0f / fTexHeight / 10.0f;
        const float fStepU = 160.0f / fTexWidth / 10.0f;

        for (int nRow = 0; nRow < 6; nRow++) {
            for (int nCol = 0; nCol < 6; nCol++) {
                int nInsetV = 1;
                if (nCol != 0)
                    nInsetV = (nCol == 5) ? -1 : 0;
                int nInsetU;
                if (nRow == 5) {
                    nInsetU = nInsetV + 1;
                    nInsetV = nInsetV - 1;
                } else if (nRow == 0) {
                    nInsetU = nInsetV - 1;
                    nInsetV = nInsetV + 1;
                } else {
                    nInsetU = nInsetV;
                }
                float fV = (float)(nRow + nCol) * fStepV + (float)nInsetV / fTexHeight;
                float fU = (float)(nInsetU + 80) / fTexWidth + (float)(nCol - nRow) * fStepU;
                OGLVertex &vertex = g_aGroundVerts[nRow][nCol];
                if (bTexelUV) {
                    vertex.fU = fU * fTexWidth;
                    fV = fV * fTexHeight;
                } else {
                    vertex.fU = fU;
                }
                vertex.fV = fV;
            }
        }

        for (int nStrip = 0; nStrip < 5; nStrip++) {
            for (int nCol = 0; nCol < 6; nCol++) {
                g_apGroundStrip[nStrip][nCol * 2] = &g_aGroundVerts[nStrip][nCol];
                g_apGroundStrip[nStrip][nCol * 2 + 1] = &g_aGroundVerts[nStrip + 1][nCol];
            }
        }
    }

    if (!OGL_BindFloorTileTexture(pTile))
        return FALSE;

    OGL_SetTextureMode(OGL_TEXMODE_TEXTURED);
    OGL_SetBlendMode(nAlpha == 0xff ? OGL_BLEND_OFF : OGL_BLEND_ALPHA);

    for (int i = 0; i < 36; i++) {
        OGLVertex &vertex = g_aGroundVerts[i / 6][i % 6];
        const BYTE *pRow = g_pModulateTable + pLight[i].sLight.nIntensity * 256;
        vertex.r = pRow[pLight[i].sLight.nRed];
        vertex.g = pRow[pLight[i].sLight.nGreen];
        vertex.b = pRow[pLight[i].sLight.nBlue];
        vertex.a = nAlpha;
        vertex.fX = (float)pLight[i].nX;
        vertex.fY = (float)(g_nScreenHeight - pLight[i].nY);
    }

    if (g_pGfxSettings->bLowQuality) {
        static const int anCorner[4][2] = {{0, 0}, {0, 5}, {5, 5}, {5, 0}};
        glBegin(GL_TRIANGLE_FAN);
        for (int i = 0; i < 4; i++) {
            const OGLVertex &vertex = g_aGroundVerts[anCorner[i][0]][anCorner[i][1]];
            glColor4ubv(&vertex.r);
            glTexCoord2f(vertex.fU, vertex.fV);
            glVertex2f(vertex.fX, vertex.fY);
        }
        glEnd();
        return TRUE;
    }

    for (int nStrip = 0; nStrip < 5; nStrip++) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int i = 0; i < 12; i++) {
            const OGLVertex *pVertex = g_apGroundStrip[nStrip][i];
            glColor4ubv(&pVertex->r);
            glTexCoord2f(pVertex->fU, pVertex->fV);
            glVertex2f(pVertex->fX, pVertex->fY);
        }
        glEnd();
    }
    return TRUE;
}

// Mac 002e4811
void OGL_AllocModulateTable(void)
{
    g_pModulateTable = (BYTE *)g_host.AllocClientMemory(0x10000, g_szFileOglVertex, 24, 0);
}

// Mac 002e4850
void OGL_FreeModulateTable(void)
{
    if (g_pModulateTable) {
        g_host.FreeClientMemory(g_pModulateTable, g_szFileOglVertex, 31, 0);
        g_pModulateTable = NULL;
    }
}

// Mac 002e4899
void OGL_BuildModulateTable(void)
{
    for (int nIntensity = 0; nIntensity < 256; nIntensity++) {
        for (int nChannel = 0; nChannel < 256; nChannel++)
            g_pModulateTable[nIntensity * 256 + nChannel] = (BYTE)((nIntensity * nChannel) / 255);
    }
}

// Mac 002e48fc
void OGL_SetVertexColorFromGlobalLight(OGLVertex *pVertex, BYTE nIntensity)
{
    const BYTE *pRow = g_pModulateTable + nIntensity * 256;
    pVertex->r = pRow[g_rgbGlobalLight.r];
    pVertex->g = pRow[g_rgbGlobalLight.g];
    pVertex->b = pRow[g_rgbGlobalLight.b];
}

// Mac 002e4940
void OGL_SetVertexColorFromLight(OGLVertex *pVertex, DWORD dwLight)
{
    const BYTE *pRow = g_pModulateTable + (dwLight & 0xff) * 256;
    pVertex->r = pRow[(dwLight >> 8) & 0xff];
    pVertex->g = pRow[(dwLight >> 16) & 0xff];
    pVertex->b = pRow[dwLight >> 24];
}

// Mac 002e4981 OGL_SetGlobalLight
void OGL_SetGlobalLight(BYTE nRed, BYTE nGreen, BYTE nBlue)
{
    g_rgbGlobalLight.r = nRed;
    g_rgbGlobalLight.g = nGreen;
    g_rgbGlobalLight.b = nBlue;
}

// Mac 002e49a7
void OGL_InitTileVertices(void)
{
    g_bTileVertsNormalizedUV = true;

    const float fScreenHeight = (float)g_nScreenHeight;
    g_aTileQuad[0].fX = 0.0f;
    g_aTileQuad[0].fY = fScreenHeight;
    g_aTileQuad[0].fU = 0.0f;
    g_aTileQuad[0].fV = 0.0f;
    g_aTileQuad[1].fX = 0.0f;
    g_aTileQuad[1].fY = fScreenHeight;
    g_aTileQuad[1].fU = 0.0f;
    g_aTileQuad[1].fV = 1.0f;
    g_aTileQuad[2].fX = 0.0f;
    g_aTileQuad[2].fY = fScreenHeight;
    g_aTileQuad[2].fU = 1.0f;
    g_aTileQuad[2].fV = 1.0f;
    g_aTileQuad[3].fX = 0.0f;
    g_aTileQuad[3].fY = fScreenHeight;
    g_aTileQuad[3].fU = 1.0f;
    g_aTileQuad[3].fV = 0.0f;

    for (int nRow = 0; nRow < 5; nRow++) {
        const float fV = (float)(nRow << 6) * 0.00390625f;
        for (int nCol = 0; nCol < 3; nCol++) {
            g_aTileGrid[nRow][nCol].fU = (float)(nCol * 0x80) * 0.00390625f;
            g_aTileGrid[nRow][nCol].fV = fV;
        }
    }

    if (!g_bWallCornerTableFixed) {
        for (int nDirection = 0; nDirection < 10; nDirection++) {
            for (int nEdge = 0; nEdge < 6; nEdge++) {
                OGLWallCorner &corner = g_aWallCornerTable[nDirection][nEdge];
                corner.nX = ShiftLeft(corner.nX, 16);
                corner.nY = ShiftLeft(corner.nY, 16);
            }
        }
        g_bWallCornerTableFixed = true;
    }
}

// Mac 002e4ae1 OGL_DrawShadowTile
BOOL OGL_DrawShadowTile(D2TileLibraryEntryStrc *pTile, int nX, int nY, int nDrawMode, int nScreenPanels)
{
    if (g_bTextureRectangle && g_bTileVertsNormalizedUV) {
        OGL_SetTileVerticesTexelUV();
        g_bTileVertsNormalizedUV = false;
    }

    BOOL bDrawn = FALSE;
    if (!g_host.AllocTileEntry(pTile, 1, 0))
        return bDrawn;

    OGL_SetTextureMode(OGL_TEXMODE_TEXTURED);
    BYTE nAlpha;
    if (OGL_GetBlendedShadows()) {
        OGL_SetBlendMode(OGL_BLEND_ALPHA);
        nAlpha = 0xc0;
    } else {
        OGL_SetBlendMode(OGL_BLEND_OFF);
        nAlpha = 0xff;
    }
    const BYTE nIntensity = (BYTE)nDrawMode;

    if (OGL_IsPerspectiveEnabled()) {
        const int nScreenHeight = g_nScreenHeight;
        const int nScreenWidth = g_nScreenWidth;
        float fOffsetX;
        if (nScreenPanels == 1)
            fOffsetX = -160.0f;
        else
            fOffsetX = (nScreenPanels == 2) ? 160.0f : 0.0f;

        OGL_SetVertexColorFromGlobalLight(&g_aTileGrid[0][0], nIntensity);
        g_aTileGrid[0][0].a = nAlpha;
        glColor4ubv(&g_aTileGrid[0][0].r);

        const bool bPanelOpen = (unsigned)(nScreenPanels - 1) < 2;
        if (pTile->nSubtileCount <= 0)
            return bDrawn;

        const int nFixedY = ShiftLeft(nY, 13);
        const int nFixedX = ShiftLeft(nX, 13);
        const float fClipLeft = bPanelOpen ? 160.0f : 0.0f;
        const float fClipRight = (float)(bPanelOpen ? nScreenWidth - 160 : nScreenWidth);
        const float fScreenHeight = (float)nScreenHeight;

        for (int i = 0; i < pTile->nSubtileCount; i++) {
            const D2TileLibraryBlockStrc *pBlock = &pTile->pSubtiles[i];
            const int nCellX = pBlock->nPosX >> 5;
            const int nCellY = (pBlock->nPosY + 0x60) >> 5;
            const int nOffsetX = ShiftLeft(nCellX * 2 + nCellY * 4, 15);
            const int nOffsetY = ShiftLeft(nCellY * 4 - nCellX * 2, 15);
            const int nStartX = nFixedX - 0x38000 + nOffsetX;
            const int nStartY = nFixedY + 0x18000 + nOffsetY;
            int nScreenX;
            int nScreenY;

            OGL_AdjustPerspectivePosition(nStartX, nStartY, 0, &nScreenX, &nScreenY);
            g_aTileGrid[0][0].fX = (float)nScreenX;
            g_aTileGrid[0][0].fY = (float)(g_nScreenHeight - nScreenY);
            OGL_AdjustPerspectivePosition(nFixedX - 0x18000 + nOffsetX, nFixedY + 0x38000 + nOffsetY, 0, &nScreenX,
                                          &nScreenY);
            g_aTileGrid[4][0].fX = (float)nScreenX;
            g_aTileGrid[4][0].fY = (float)(g_nScreenHeight - nScreenY);
            OGL_AdjustPerspectivePosition(nFixedX - 0x8000 + nOffsetX, nFixedY + 0x28000 + nOffsetY, 0, &nScreenX,
                                          &nScreenY);
            g_aTileGrid[4][2].fX = (float)nScreenX;
            g_aTileGrid[4][2].fY = (float)(g_nScreenHeight - nScreenY);
            OGL_AdjustPerspectivePosition(nFixedX - 0x28000 + nOffsetX, (nOffsetY | 0x8000) + nFixedY, 0, &nScreenX,
                                          &nScreenY);
            g_aTileGrid[0][2].fX = (float)nScreenX;
            g_aTileGrid[0][2].fY = (float)(g_nScreenHeight - nScreenY);

            if (!IsInPerspectiveWindow(g_aTileGrid[0][0], fClipLeft, fClipRight, fScreenHeight) &&
                !IsInPerspectiveWindow(g_aTileGrid[4][0], fClipLeft, fClipRight, fScreenHeight) &&
                !IsInPerspectiveWindow(g_aTileGrid[4][2], fClipLeft, fClipRight, fScreenHeight) &&
                !IsInPerspectiveWindow(g_aTileGrid[0][2], fClipLeft, fClipRight, fScreenHeight))
                continue;

            OGL_BindWallBlockTexture(&pTile->pSubtiles[i]);

            int nRowX = nStartX;
            int nRowY = nStartY;
            for (int nRow = 0; nRow < 5; nRow++) {
                int nPointX = nRowX;
                int nPointY = nRowY;
                for (int nCol = 0; nCol < 3; nCol++) {
                    OGL_AdjustPerspectivePosition(nPointX, nPointY, 0, &nScreenX, &nScreenY);
                    g_aTileGrid[nRow][nCol].fY = (float)(g_nScreenHeight - nScreenY);
                    g_aTileGrid[nRow][nCol].fX = (float)nScreenX + fOffsetX;
                    nPointX += 0x8000;
                    nPointY -= 0x8000;
                }
                nRowX += 0x8000;
                nRowY += 0x8000;
            }

            for (int nStrip = 0; nStrip < 2; nStrip++) {
                glBegin(GL_TRIANGLE_STRIP);
                for (int k = 0; k < 10; k++) {
                    const int nIndex = g_aTileGridStripIndex[nStrip][k];
                    const OGLVertex &vertex = g_aTileGrid[nIndex / 3][nIndex % 3];
                    glTexCoord2f(vertex.fU, vertex.fV);
                    glVertex2f(vertex.fX, vertex.fY);
                }
                glEnd();
            }
            bDrawn = TRUE;
        }
        return bDrawn;
    }

    int nClipRight = g_nScreenWidth;
    const int nClipBottom = g_nScreenHeight - 15;
    int nClipLeft;
    if (nScreenPanels == 2) {
        nClipLeft = 0x120;
    } else {
        nClipLeft = -0x20;
        if (nScreenPanels == 1)
            nClipRight += -0x140;
    }

    OGL_SetVertexColorFromGlobalLight(&g_aTileQuad[0], nIntensity);
    g_aTileQuad[0].a = nAlpha;
    glColor4ubv(&g_aTileQuad[0].r);

    for (int i = 0; i < pTile->nSubtileCount; i++) {
        const D2TileLibraryBlockStrc *pBlock = &pTile->pSubtiles[i];
        const int nLeft = pBlock->nPosX + nX;
        if (nLeft < nClipLeft || nLeft >= nClipRight)
            continue;
        const int nBottom = nY + 0x20 + pBlock->nPosY;
        if (nBottom < 0 || nBottom >= nClipBottom)
            continue;

        OGL_BindWallBlockTexture(&pTile->pSubtiles[i]);

        const int nScreenHeight = g_nScreenHeight;
        const float fLeft = (float)nLeft;
        const float fTop = (float)(nScreenHeight - (pBlock->nPosY + nY));
        const float fBottom = (float)(nScreenHeight - nBottom);
        const float fRight = (float)(nLeft + 32);
        g_aTileQuad[0].fX = fLeft;
        g_aTileQuad[0].fY = fTop;
        g_aTileQuad[1].fX = fLeft;
        g_aTileQuad[1].fY = fBottom;
        g_aTileQuad[2].fX = fRight;
        g_aTileQuad[2].fY = fBottom;
        g_aTileQuad[3].fX = fRight;
        g_aTileQuad[3].fY = fTop;

        glBegin(GL_TRIANGLE_FAN);
        for (int k = 0; k < 4; k++) {
            glTexCoord2f(g_aTileQuad[k].fU, g_aTileQuad[k].fV);
            glVertex2f(g_aTileQuad[k].fX, g_aTileQuad[k].fY);
        }
        glEnd();
        bDrawn = TRUE;
    }
    return bDrawn;
}

// Mac 002e531b
void OGL_SetTileVerticesTexelUV(void)
{
    const float fScreenHeight = (float)g_nScreenHeight;
    g_aTileQuad[0].fX = 0.0f;
    g_aTileQuad[0].fY = fScreenHeight;
    g_aTileQuad[0].fU = 0.0f;
    g_aTileQuad[0].fV = 0.0f;
    g_aTileQuad[1].fX = 0.0f;
    g_aTileQuad[1].fY = fScreenHeight;
    g_aTileQuad[1].fU = 0.0f;
    g_aTileQuad[1].fV = 32.0f;
    g_aTileQuad[2].fX = 0.0f;
    g_aTileQuad[2].fY = fScreenHeight;
    g_aTileQuad[2].fU = 32.0f;
    g_aTileQuad[2].fV = 32.0f;
    g_aTileQuad[3].fX = 0.0f;
    g_aTileQuad[3].fY = fScreenHeight;
    g_aTileQuad[3].fU = 32.0f;
    g_aTileQuad[3].fV = 0.0f;

    for (int nRow = 0; nRow < 5; nRow++) {
        const float fV = (float)(nRow << 11) * 0.00390625f;
        for (int nCol = 0; nCol < 3; nCol++) {
            g_aTileGrid[nRow][nCol].fU = (float)(nCol * 0x1000) * 0.00390625f;
            g_aTileGrid[nRow][nCol].fV = fV;
        }
    }
}

// Mac 002e5419 OGL_DrawTransWallTile
BOOL OGL_DrawTransWallTile(D2TileLibraryEntryStrc *pTile, int nX, int nY, D2GfxLightStrc *pLight, int nScreenPanels,
                           BYTE nAlpha)
{
    if (g_bTextureRectangle && g_bTileVertsNormalizedUV) {
        OGL_SetTileVerticesTexelUV();
        g_bTileVertsNormalizedUV = false;
    }
    return OGL_DrawWallTileBlocks(pTile, nX, nY, pLight, nAlpha, nScreenPanels);
}

// Mac 002e547c
BOOL OGL_DrawWallTileBlocks(D2TileLibraryEntryStrc *pTile, int nX, int nY, const D2GfxLightStrc *pLight, BYTE nAlpha,
                            int nScreenPanels)
{
    BOOL bDrawn = FALSE;
    if (!g_host.AllocTileEntry(pTile, 1, 0))
        return bDrawn;

    OGL_SetTextureMode(OGL_TEXMODE_TEXTURED);
    OGL_SetBlendMode(nAlpha == 0xff ? OGL_BLEND_OFF : OGL_BLEND_ALPHA);

    if (OGL_IsPerspectiveEnabled()) {
        const int nScreenHeight = g_nScreenHeight;
        const int nScreenWidth = g_nScreenWidth;
        float fOffsetX;
        if (nScreenPanels == 1)
            fOffsetX = -160.0f;
        else
            fOffsetX = (nScreenPanels == 2) ? 160.0f : 0.0f;
        const bool bPanelOpen = (unsigned)(nScreenPanels - 1) < 2;
        const int nClipRight = bPanelOpen ? nScreenWidth - 160 : nScreenWidth;
        const float fClipLeft = bPanelOpen ? 160.0f : 0.0f;

        const int nDirection = pTile->nDirection;
        if ((unsigned)nDirection >= 10)
            OGL_HALT(620);
        if (pTile->nSubtileCount <= 0)
            return bDrawn;

        const int nFixedY = ShiftLeft(nY, 13);
        const int nFixedX = ShiftLeft(nX, 13);
        const float fScreenHeight = (float)nScreenHeight;
        const float fClipRight = (float)nClipRight;

        for (int i = 0; i < pTile->nSubtileCount; i++) {
            const D2TileLibraryBlockStrc *pBlock = &pTile->pSubtiles[i];
            const int nBlockY = pBlock->nPosY;
            const int nCol = pBlock->nPosX >> 5;
            if ((unsigned)nCol >= 5)
                OGL_HALT(628);

            const OGLWallCorner &left = g_aWallCornerTable[nDirection][nCol];
            const OGLWallCorner &right = g_aWallCornerTable[nDirection][nCol + 1];
            const int nRightX = right.nX + nFixedX;
            const int nRightY = right.nY + nFixedY;
            const int nLeftX = left.nX + nFixedX;
            const int nLeftY = left.nY + nFixedY;
            const int nLeftZ = left.nZ - nBlockY;
            const int nRightZ = right.nZ - nBlockY;
            int nScreenX;
            int nScreenY;

            OGL_AdjustPerspectivePosition(nLeftX, nLeftY, nLeftZ, &nScreenX, &nScreenY);
            g_aTileGrid[0][0].fX = (float)nScreenX;
            g_aTileGrid[0][0].fY = (float)(g_nScreenHeight - nScreenY);
            OGL_AdjustPerspectivePosition(nLeftX, nLeftY, nLeftZ - 32, &nScreenX, &nScreenY);
            g_aTileGrid[4][0].fX = (float)nScreenX;
            g_aTileGrid[4][0].fY = (float)(g_nScreenHeight - nScreenY);
            OGL_AdjustPerspectivePosition(nRightX, nRightY, nRightZ - 32, &nScreenX, &nScreenY);
            g_aTileGrid[4][2].fX = (float)nScreenX;
            g_aTileGrid[4][2].fY = (float)(g_nScreenHeight - nScreenY);
            OGL_AdjustPerspectivePosition(nRightX, nRightY, nRightZ, &nScreenX, &nScreenY);
            g_aTileGrid[0][2].fX = (float)nScreenX;
            g_aTileGrid[0][2].fY = (float)(g_nScreenHeight - nScreenY);

            if (!IsInPerspectiveWindow(g_aTileGrid[0][0], fClipLeft, fClipRight, fScreenHeight) &&
                !IsInPerspectiveWindow(g_aTileGrid[4][0], fClipLeft, fClipRight, fScreenHeight) &&
                !IsInPerspectiveWindow(g_aTileGrid[4][2], fClipLeft, fClipRight, fScreenHeight) &&
                !IsInPerspectiveWindow(g_aTileGrid[0][2], fClipLeft, fClipRight, fScreenHeight))
                continue;

            const D2GfxLightStrc &lightLeft = pLight[nCol];
            const D2GfxLightStrc &lightRight = pLight[nCol + 1];
            D2GfxLightStrc lightMid;
            lightMid.nIntensity = (BYTE)((lightLeft.nIntensity + lightRight.nIntensity) >> 1);
            lightMid.nRed = (BYTE)((lightLeft.nRed + lightRight.nRed) >> 1);
            lightMid.nGreen = (BYTE)((lightLeft.nGreen + lightRight.nGreen) >> 1);
            lightMid.nBlue = (BYTE)((lightLeft.nBlue + lightRight.nBlue) >> 1);
            const DWORD adwLight[3] = {OGL_LightToDword(lightLeft), OGL_LightToDword(lightMid),
                                       OGL_LightToDword(lightRight)};

            const int nMidZ = (nRightZ + nLeftZ) >> 1;
            const int nMidY = (nRightY + nLeftY) >> 1;
            const int nMidX = (nRightX + nLeftX) >> 1;
            OGL_BindWallBlockTexture(&pTile->pSubtiles[i]);

            for (int nRow = 0; nRow < 5; nRow++) {
                const int nDrop = nRow * 8;
                if ((nRow | 4) != 4) {
                    OGL_AdjustPerspectivePosition(nLeftX, nLeftY, nLeftZ - nDrop, &nScreenX, &nScreenY);
                    g_aTileGrid[nRow][0].fX = (float)nScreenX;
                    g_aTileGrid[nRow][0].fY = (float)(g_nScreenHeight - nScreenY);
                    OGL_AdjustPerspectivePosition(nRightX, nRightY, nRightZ - nDrop, &nScreenX, &nScreenY);
                    g_aTileGrid[nRow][2].fX = (float)nScreenX;
                    g_aTileGrid[nRow][2].fY = (float)(g_nScreenHeight - nScreenY);
                }
                OGL_AdjustPerspectivePosition(nMidX, nMidY, nMidZ - nDrop, &nScreenX, &nScreenY);
                g_aTileGrid[nRow][1].fX = (float)nScreenX;
                g_aTileGrid[nRow][1].fY = (float)(g_nScreenHeight - nScreenY);

                for (int nGridCol = 0; nGridCol < 3; nGridCol++) {
                    OGLVertex &vertex = g_aTileGrid[nRow][nGridCol];
                    vertex.fX = vertex.fX + fOffsetX;
                    OGL_SetVertexColorFromLight(&vertex, adwLight[nGridCol]);
                    vertex.a = nAlpha;
                }
            }

            for (int nStrip = 0; nStrip < 2; nStrip++) {
                glBegin(GL_TRIANGLE_STRIP);
                for (int k = 0; k < 10; k++) {
                    const int nIndex = g_aTileGridStripIndex[nStrip][k];
                    const OGLVertex &vertex = g_aTileGrid[nIndex / 3][nIndex % 3];
                    glColor4ubv(&vertex.r);
                    glTexCoord2f(vertex.fU, vertex.fV);
                    glVertex2f(vertex.fX, vertex.fY);
                }
                glEnd();
            }
            bDrawn = TRUE;
        }
        return bDrawn;
    }

    int nClipRight = g_nScreenWidth;
    const int nClipBottom = g_nScreenHeight - 15;
    int nClipLeft;
    if (nScreenPanels == 2) {
        nClipLeft = 0x120;
    } else {
        nClipLeft = -0x20;
        if (nScreenPanels == 1)
            nClipRight += -0x140;
    }

    g_aTileQuad[3].a = nAlpha;
    g_aTileQuad[2].a = nAlpha;
    g_aTileQuad[1].a = nAlpha;
    g_aTileQuad[0].a = nAlpha;

    for (int i = 0; i < pTile->nSubtileCount; i++) {
        const D2TileLibraryBlockStrc *pBlock = &pTile->pSubtiles[i];
        const int nLeft = pBlock->nPosX + nX;
        if (nLeft < nClipLeft || nLeft >= nClipRight)
            continue;
        const int nBottom = nY + 0x20 + pBlock->nPosY;
        if (nBottom < 0 || nBottom >= nClipBottom)
            continue;

        const int nCol = pBlock->nPosX >> 5;
        const DWORD dwLightLeft = OGL_LightToDword(pLight[nCol]);
        const DWORD dwLightRight = OGL_LightToDword(pLight[nCol + 1]);
        OGL_BindWallBlockTexture(&pTile->pSubtiles[i]);

        const int nScreenHeight = g_nScreenHeight;
        const float fLeft = (float)nLeft;
        const float fTop = (float)(nScreenHeight - (pBlock->nPosY + nY));
        const float fBottom = (float)(nScreenHeight - nBottom);
        const float fRight = (float)(nLeft + 32);
        g_aTileQuad[0].fX = fLeft;
        g_aTileQuad[0].fY = fTop;
        g_aTileQuad[1].fX = fLeft;
        g_aTileQuad[1].fY = fBottom;
        g_aTileQuad[2].fX = fRight;
        g_aTileQuad[2].fY = fBottom;
        g_aTileQuad[3].fX = fRight;
        g_aTileQuad[3].fY = fTop;
        OGL_SetVertexColorFromLight(&g_aTileQuad[0], dwLightLeft);
        OGL_SetVertexColorFromLight(&g_aTileQuad[2], dwLightRight);

        glBegin(GL_TRIANGLE_FAN);
        glColor4ubv(&g_aTileQuad[0].r);
        glTexCoord2f(g_aTileQuad[0].fU, g_aTileQuad[0].fV);
        glVertex2f(g_aTileQuad[0].fX, g_aTileQuad[0].fY);
        glTexCoord2f(g_aTileQuad[1].fU, g_aTileQuad[1].fV);
        glVertex2f(g_aTileQuad[1].fX, g_aTileQuad[1].fY);
        glColor4ubv(&g_aTileQuad[2].r);
        glTexCoord2f(g_aTileQuad[2].fU, g_aTileQuad[2].fV);
        glVertex2f(g_aTileQuad[2].fX, g_aTileQuad[2].fY);
        glTexCoord2f(g_aTileQuad[3].fU, g_aTileQuad[3].fV);
        glVertex2f(g_aTileQuad[3].fX, g_aTileQuad[3].fY);
        glEnd();
        bDrawn = TRUE;
    }
    return bDrawn;
}

// Mac 002e5ea1 OGL_DrawWallTile
BOOL OGL_DrawWallTile(D2TileLibraryEntryStrc *pTile, int nX, int nY, D2GfxLightStrc *pLight, int nScreenPanels)
{
    if (g_bTextureRectangle && g_bTileVertsNormalizedUV) {
        OGL_SetTileVerticesTexelUV();
        g_bTileVertsNormalizedUV = false;
    }
    return OGL_DrawWallTileBlocks(pTile, nX, nY, pLight, 0xff, nScreenPanels);
}
