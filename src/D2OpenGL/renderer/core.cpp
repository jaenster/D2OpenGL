// The cutscene GL state and Bink player, the oglBlocks decoders, and the init/window/scene/cutscene
// slot functions (Mac 002de719-002df9b9).

#include "renderer.h"

// GL_TEXTURE_RECTANGLE_ARB is only a valid capability where the driver has a texture rectangle
// extension; elsewhere glGetBooleanv/glEnable/glDisable on it raise GL_INVALID_ENUM.
static bool OGL_HasTextureRectangle(void)
{
    const char *szExtensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
    if (!szExtensions)
        return false;
    return strstr(szExtensions, "GL_ARB_texture_rectangle") || strstr(szExtensions, "GL_EXT_texture_rectangle") ||
           strstr(szExtensions, "GL_NV_texture_rectangle");
}

// Mac 002de719 OGL_CutsceneBeginGL
void OGL_CutsceneBeginGL(int nScreenWidth, int nScreenHeight, int nTexWidth, int nTexHeight)
{
    if (nScreenWidth < 1 || nScreenHeight < 1)
        OGL_HALT(55);

    g_nCutsceneScreenWidth = nScreenWidth;
    g_nCutsceneScreenHeight = nScreenHeight;
    g_nCutsceneTexWidth = nTexWidth;
    g_nCutsceneTexHeight = nTexHeight;
    if (g_nCutsceneTexture == 0)
        glGenTextures(1, &g_nCutsceneTexture);
    Platform_glActiveTexture(GL_TEXTURE0);

    bool bTextureRectangle = OGL_HasTextureRectangle();
    CutsceneGLCapState *pCap = g_aCutsceneGLCaps;
    for (int i = 3; i != 0; --i, ++pCap) {
        if (pCap->eCap == GL_TEXTURE_RECTANGLE_ARB && !bTextureRectangle)
            continue;
        glGetBooleanv(pCap->eCap, &pCap->bSaved);
        if (pCap->bWanted)
            glEnable(pCap->eCap);
        else
            glDisable(pCap->eCap);
    }
}

// Mac 002de7e4 OGL_CutsceneRestoreGL
void OGL_CutsceneRestoreGL(void)
{
    bool bTextureRectangle = OGL_HasTextureRectangle();
    CutsceneGLCapState *pCap = g_aCutsceneGLCaps;
    for (int i = 3; i != 0; --i, ++pCap) {
        if (pCap->eCap == GL_TEXTURE_RECTANGLE_ARB && !bTextureRectangle)
            continue;
        if (pCap->bSaved)
            glEnable(pCap->eCap);
        else
            glDisable(pCap->eCap);
    }
}

// Mac 002de824 OGL_PlayBinkMovie
void OGL_PlayBinkMovie(const char *szFile, int nResolutionMode, BYTE *pBuffer, int nPitch, OGLFrameCallback fpFrame)
{
    if (pBuffer == NULL)
        OGL_HALT(189);

    memset(pBuffer, 0, g_nCutsceneTexWidth * g_nCutsceneTexHeight * 4);
    g_bCutsceneDone = false;

    int nWidth;
    int nHeight;
    g_host.GetScreenSize(&nWidth, &nHeight);
    if (g_pBink != NULL || g_bSurfacePaused)
        return;

    Platform_FlushEvents(0xffff);
    g_host.BinkSetSoundSystem(g_host.BinkOpenDirectSound, 0);

    HANDLE hFile = NULL;
    g_host.SFileOpenFileEx(NULL, szFile, (g_host.GetUseDirectCommand() != 0) | 4, &hFile);
    if (hFile == NULL) {
        if (g_pBink != NULL) {
            g_host.BinkClose(g_pBink);
            g_pBink = NULL;
        }
        return;
    }

    uint32_t dwFlags;
    if (nResolutionMode == 3)
        dwFlags = 0x40800000;
    else if (nResolutionMode == 2)
        dwFlags = 0x10800000;
    else {
        dwFlags = 0x800000;
        if (nResolutionMode == 1)
            dwFlags = 0x30800000;
    }

    g_pBink = g_host.BinkOpen(hFile, dwFlags);
    if (g_pBink != NULL) {
        g_nBinkDestX = (0x280U - g_pBink->Width) >> 1;
        if (g_nBinkDestX & 3)
            g_nBinkDestX = (g_nBinkDestX | 3) + 1;
        g_nBinkDestY = (0x1e0U - g_pBink->Height) >> 1;

        DWORD dwStopTime = GetTickCount() + 500;
        for (;;) {
            if (g_bCutsceneDone == 1 && GetTickCount() >= dwStopTime)
                break;
            if (g_bSurfacePaused)
                break;

            if (fpFrame)
                fpFrame();

            POINT ptCursor;
            Platform_GetCursorPos(&ptCursor);
            POINT ptOrigin;
            Platform_GetClientOrigin(&ptOrigin);
            short nCursorX = (short)ptCursor.x;
            short nCursorY = (short)ptCursor.y;
            short nOriginX = (short)ptOrigin.x;
            short nOriginY = (short)ptOrigin.y;
            if (nCursorX < nWidth + nOriginX && nCursorX >= nOriginX && nCursorY < nHeight + nOriginY &&
                nCursorY >= nOriginY)
                g_host.HideOSCursor();
            else
                g_host.ShowOSCursor();

            if (Platform_EventAvail(0x2a)) {
                g_bCutsceneDone = true;
                Platform_FlushEvents(0x3e);
                continue;
            }

            if (g_host.BinkWait(g_pBink) != 0)
                continue;

            g_host.BinkDoFrame(g_pBink);
            g_host.BinkCopyToBuffer(g_pBink, pBuffer, nPitch, g_pBink->Height, g_nBinkDestX, g_nBinkDestY,
                                    g_nBinkSurfaceType);
            if (g_pBink->FrameNum == g_pBink->Frames)
                g_bCutsceneDone = true;
            else
                g_host.BinkNextFrame(g_pBink);

            uint32_t nMovieHeight = g_pBink->Height;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glBindTexture(GL_TEXTURE_2D, g_nCutsceneTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_nCutsceneTexWidth, g_nCutsceneTexHeight, 0,
                         Platform_GetBinkPixelFormat(), GL_UNSIGNED_BYTE, pBuffer);
            if (g_nCutsceneTexHeight < 1)
                OGL_HALT(114);

            float fV = (float)nMovieHeight / (float)g_nCutsceneTexHeight;
            float fMargin = (1.0f - fV) * 0.5f;
            float fScreenHeight = (float)g_nCutsceneScreenHeight;
            float fBottom = (1.0f - fMargin) * fScreenHeight;
            float fTop = fMargin * fScreenHeight;
            float fRight = (float)g_nCutsceneScreenWidth;

            CutsceneVertex aQuad[4];
            aQuad[0] = {0.0f, fBottom, 0.0f, 0.0f};
            aQuad[1] = {0.0f, fTop, 0.0f, fV};
            aQuad[2] = {fRight, fBottom, 1.0f, 0.0f};
            aQuad[3] = {fRight, fTop, 1.0f, fV};

            glClear(GL_COLOR_BUFFER_BIT);
            glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(0.0f, aQuad[0].fY);
            for (int i = 1; i != 4; ++i) {
                glTexCoord2f(aQuad[i].fU, aQuad[i].fV);
                glVertex2f(aQuad[i].fX, aQuad[i].fY);
            }
            glEnd();
            OGL_SwapBuffers();
        }

        if (g_pBink != NULL) {
            g_host.SetGlobalVolume(0);
            g_host.BinkClose(g_pBink);
            g_host.SetGlobalVolume(0xff);
            g_pBink = NULL;
        }
    }

    if (hFile != NULL)
        CloseHandle(hFile);
}

// Mac 002dee38 OGL_BlocksInit
void OGL_BlocksInit(void)
{
    g_pWallBlockBuffer = static_cast<BYTE *>(g_host.AllocClientMemory(0x400, g_szFileOglBlocks, 49, 0));
    g_pBlockScratch = static_cast<BYTE *>(g_host.AllocClientMemory(0x1000, g_szFileOglBlocks, 50, 0));
    g_pFloorTileBuffer = static_cast<BYTE *>(g_host.AllocClientMemory(0x20000, g_szFileOglBlocks, 51, 0));
}

// Mac 002deec5 OGL_BlocksShutdown
void OGL_BlocksShutdown(void)
{
    if (g_pWallBlockBuffer) {
        g_host.FreeClientMemory(g_pWallBlockBuffer, g_szFileOglBlocks, 58, 0);
        g_pWallBlockBuffer = NULL;
    }
    if (g_pBlockScratch) {
        g_host.FreeClientMemory(g_pBlockScratch, g_szFileOglBlocks, 64, 0);
        g_pBlockScratch = NULL;
    }
    if (g_pFloorTileBuffer) {
        g_host.FreeClientMemory(g_pFloorTileBuffer, g_szFileOglBlocks, 70, 0);
        g_pFloorTileBuffer = NULL;
    }
}

// Mac 002def7a OGL_BindFloorTileTexture
BOOL OGL_BindFloorTileTexture(D2TileLibraryEntryStrc *pTile)
{
    if (!g_host.AllocTileEntry(pTile, 1, 0))
        return FALSE;

    short nTextureId = pTile->nTextureCacheIndex;
    if (nTextureId < 0)
        OGL_HALT(381);

    if (nTextureId == 0) {
        if (g_bAGPTextures) {
            OGL_DecodeFloorTileBlocks(pTile, g_pFloorTileBuffer, 0xa0);
            g_pTextures->CreateTexture(g_pFloorTileBuffer, 0xa0, 0x50, TEXOWNER_TILE, pTile, 0, 0);
        } else {
            OGL_DecodeFloorTileBlocks(pTile, g_pFloorTileBuffer, 0x100);
            g_pTextures->CreateTexture(g_pFloorTileBuffer, 0x100, 0x80, TEXOWNER_TILE, pTile, 0, 0);
        }
        OGL_SetTextureFilter(OGL_FILTER_LINEAR);
        nTextureId = pTile->nTextureCacheIndex;
    }
    g_pTextures->BindTextureById(nTextureId);
    return TRUE;
}

// Mac 002df0d1 OGL_DecodeFloorTileBlocks
void OGL_DecodeFloorTileBlocks(D2TileLibraryEntryStrc *pTile, BYTE *pDest, int nPitch)
{
    memset(pDest, 0, nPitch << 7);

    for (int i = 0; i < pTile->nSubtileCount; ++i) {
        D2TileLibraryBlockStrc *pBlock = &pTile->pSubtiles[i];
        if (!(pBlock->nFormat & 1))
            continue;

        int nOffset = pBlock->nPosY * nPitch + pBlock->nPosX;
        if (!(pBlock->nFormat & 4)) {
            // Isometric diamond: 15 rows of 4, 8, .., 32, .., 8, 4 bytes, centred.
            BYTE *pDst = pDest + nOffset + 14;
            const BYTE *pSrc = pBlock->pPixelData;
            int nLength = 4;
            for (int nRow = 0; nRow != 15; ++nRow) {
                int nShift = nRow < 7 ? -2 : 2;
                int nGrow = nRow < 7 ? 4 : -4;
                memcpy(pDst, pSrc, nLength);
                pDst += nPitch + nShift;
                pSrc += nLength;
                nLength += nGrow;
            }
        } else {
            // RLE: (skip, count) pairs, (0, 0) ends a row; 15 rows.
            BYTE *pRow = pDest + nOffset;
            const BYTE *pSrc = pBlock->pPixelData;
            for (int nRow = 15; nRow != 0; --nRow) {
                BYTE *pDst = pRow;
                for (;;) {
                    BYTE nSkip = pSrc[0];
                    BYTE nCount = pSrc[1];
                    pSrc += 2;
                    if (nSkip == 0 && nCount == 0)
                        break;
                    pDst += nSkip;
                    for (; nCount != 0; --nCount)
                        *pDst++ = *pSrc++;
                }
                pRow += nPitch;
            }
        }
    }
}

// Mac 002df201 OGL_BindWallBlockTexture
void OGL_BindWallBlockTexture(D2TileLibraryBlockStrc *pBlock)
{
    short nTextureId = pBlock->nTextureIndex;
    if (nTextureId < 0)
        OGL_HALT(444);

    if (nTextureId == 0) {
        const BYTE *pSrc = pBlock->pPixelData;
        BYTE *pRow = g_pWallBlockBuffer;
        memset(pRow, 0, 0x400);
        // RLE: (skip, count) pairs, (0, 0) ends a row; 32 rows of 32 bytes.
        for (int nRow = 0x20; nRow != 0; --nRow) {
            BYTE *pDst = pRow;
            for (;;) {
                BYTE nSkip = pSrc[0];
                BYTE nCount = pSrc[1];
                pSrc += 2;
                if (nSkip == 0 && nCount == 0)
                    break;
                pDst += nSkip;
                for (; nCount != 0; --nCount)
                    *pDst++ = *pSrc++;
            }
            pRow += 0x20;
        }
        g_pTextures->CreateTexture(g_pWallBlockBuffer, 0x20, 0x20, TEXOWNER_BLOCK, pBlock, 0, 0);
        OGL_SetTextureFilter(OGL_FILTER_NEAREST);
        nTextureId = pBlock->nTextureIndex;
    }
    g_pTextures->BindTextureById(nTextureId);
}

// Mac 002df347 OGL_ReleaseTileTextures
void OGL_FASTCALL OGL_ReleaseTileTextures(D2TileLibraryEntryStrc *pTile)
{
    if (pTile->nTextureCacheIndex > 0)
        g_pTextures->FreeTextureById(pTile->nTextureCacheIndex);

    for (int i = 0; i < pTile->nSubtileCount; ++i) {
        int nTextureId = pTile->pSubtiles[i].nTextureIndex;
        if (nTextureId > 0)
            g_pTextures->FreeTextureById(nTextureId);
    }
}

// Mac 002df3c4 OGL_Initialize
BOOL OGL_Initialize(HINSTANCE hInstance)
{
    return OGL_PlatformInitialize(hInstance);
}

// Mac 002df3cd OGL_InitPerspective
BOOL OGL_InitPerspective(D2GfxSettingsStrc *pSettings, D2GfxHelperStrc *pHelpers)
{
    g_pGfxSettings = pSettings;
    g_pGfxHelpers = pHelpers;
    OGL_BlocksInit();
    OGL_AllocPerspectiveTable();
    OGL_AllocModulateTable();
    platform_atexit(OGL_AtExitRelease);
    OGL_BuildModulateTable();
    OGL_BuildPerspectiveTable();
    OGL_InitTileVertices();
    OGL_InvalidateGroundMesh();
    OGL_InstallCursorCallbacks();
    return TRUE;
}

// Mac 002df42b OGL_AtExitRelease
void OGL_AtExitRelease(void)
{
    if (!g_bAtExitReleased) {
        OGL_BlocksShutdown();
        OGL_FreePerspectiveTable();
        OGL_FreeModulateTable();
        g_bAtExitReleased = true;
    }
}

// Mac 002df45c OGL_Release
BOOL OGL_Release(void)
{
    OGL_BlocksShutdown();
    OGL_FreePerspectiveTable();
    OGL_FreeModulateTable();
    return TRUE;
}

// Mac 002df47b OGL_CreateWindow
BOOL OGL_CreateWindow(HWND hWnd, int nResolutionMode)
{
    (void)hWnd;
    int nResolution = nResolutionMode == 2 ? 1 : nResolutionMode;
    OGL_GammaBlack();
    OGL_PlatformCreateSurface(nResolution);
    if (OGL_OpenOpenGLWindow(nResolution)) {
        OGL_ClearDisplay();
        OGL_ApplyGamma();
        return TRUE;
    }
    g_host.LogWrite("OpenGLCreateSurface: *** sOpenOpenGLWindow failed.");
    OGL_ApplyGamma();
    return FALSE;
}

// Mac 002df4db OGL_OpenOpenGLWindow
BOOL OGL_OpenOpenGLWindow(int nResolutionMode)
{
    OGL_OpenWindowHook1();
    g_nScreenWidth = nResolutionMode ? 800 : 640;
    g_nScreenHeight = nResolutionMode ? 600 : 480;
    if (!OGL_PlatformOpenWindow(nResolutionMode))
        return FALSE;

    glMatrixMode(GL_PROJECTION);
    glOrtho(0.0, (double)g_nScreenWidth, 0.0, (double)g_nScreenHeight, -1.0, 1.0);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glAlphaFunc(GL_NOTEQUAL, 0.0f);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, 8448.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    if (glGetError() != GL_NO_ERROR)
        OGL_HALT(292);

    OGL_InitSpriteBuffers();
    g_nRenderStat0 = 0;
    g_nRenderStat3 = 0;
    g_nRenderStat6 = 0;
    g_nRenderStat7 = 0;
    g_nRenderStat8 = 0;
    OGL_OpenWindowHook2();

    if (g_pTextures) {
        OGL_HALT(307);
        if (g_pTextures) {
            delete g_pTextures;
            g_pTextures = NULL;
        }
    }

    if (OGL_CanUseAGPTextures())
        g_pTextures = new COGLAGPTextures();
    else
        g_pTextures = new COGLTextures(OGL_GetTextureMemoryBudget());

    if (g_nScreenWidth == 800 && g_nScreenHeight == 600)
        OGL_InitSmackerTextures();

    g_host.SetTileFreeCallback(OGL_ReleaseTileTextures);
    return TRUE;
}

// Mac 002df760 OGL_DestroyWindow
BOOL OGL_DestroyWindow(void)
{
    OGL_GammaBlack();
    OGL_CloseOpenGLWindow();
    OGL_RestoreDisplayMode();
    OGL_RestoreGamma();
    return TRUE;
}

// Mac 002df784 OGL_CloseOpenGLWindow
void OGL_CloseOpenGLWindow(void)
{
    OGL_CloseWindowHook1();
    if (g_pTextures)
        delete g_pTextures;
    g_pTextures = NULL;
    if (g_nScreenWidth == 800 && g_nScreenHeight == 600)
        OGL_FreeSmackerTextures();
    OGL_CloseWindowHook2();
    OGL_FreeSpriteBuffers();
    g_host.SetTileFreeCallback(NULL);
    OGL_DisposeWindow();
}

// Mac 002df7ed OGL_ClearCaches
void OGL_ClearCaches(void)
{
    if (g_pTextures) {
        delete g_pTextures;
        g_pTextures = NULL;
    }
}

// Mac 002df81a OGL_EndCutScene
void OGL_EndCutScene(HWND hWnd, int nResolutionMode, int nWindowState)
{
    (void)hWnd;
    while (g_bInScene)
        Platform_Yield();

    if (nWindowState != 0) {
        OGL_DisposeWindow();
        g_bSurfacePaused = TRUE;
    } else {
        OGL_PlatformOpenWindow(nResolutionMode);
        g_bSurfacePaused = FALSE;
    }
}

// Mac 002df864 OGL_BeginCutScene
BOOL OGL_BeginCutScene(void)
{
    return TRUE;
}

// Mac 002df86e OGL_PlayCutScene
void OGL_PlayCutScene(const char *szFile, int nResolutionMode, OGLFrameCallback fpFrame)
{
    BYTE *pBuffer;
    int nPitch;
    int nUnused;
    OGL_GammaBlack();
    if (!OGL_StartCutscene(&pBuffer, &nPitch, &nUnused)) {
        g_host.LogWrite("OpenGLPlayCutscene: *** OpenGLMacStartCutscene failed");
        return;
    }
    OGL_GammaIdentity();
    g_bInCutscene = true;
    OGL_PlayBinkMovie(szFile, nResolutionMode, pBuffer, nPitch, fpFrame);
    g_bInCutscene = false;
}

// Mac 002df8f1 OGL_CheckCutScene
BOOL OGL_CheckCutScene(void)
{
    return g_bInCutscene;
}

// Mac 002df903 OGL_BeginScene
BOOL OGL_BeginScene(BOOL bClear, BYTE nRed, BYTE nGreen, BYTE nBlue)
{
    (void)nRed;
    (void)nGreen;
    (void)nBlue;
    if (g_bInScene | g_bSurfacePaused)
        return FALSE;

    g_bInScene = TRUE;
    if (bClear)
        glClear(GL_COLOR_BUFFER_BIT);
    g_nCurTextureMode = OGL_TEXMODE_UNSET;
    g_nCurBlendMode = OGL_BLEND_UNSET;
    OGL_SetGlobalLight(0xff, 0xff, 0xff);
    return TRUE;
}

// Mac 002df978 OGL_EndScene1
BOOL OGL_EndScene1(void)
{
    g_bInScene = FALSE;
    return TRUE;
}

// Mac 002df992 OGL_UpdateScaleFactor
void OGL_UpdateScaleFactor(int nScaleFactor)
{
    g_nScaleFactor = nScaleFactor;
}

// Mac 002df9a6 OGL_SetGamma
void OGL_SetGamma(int nGamma)
{
    OGL_SetGammaLevel(nGamma);
}

// Mac 002df9af OGL_CheckGamma
int OGL_CheckGamma(void)
{
    return 1;
}

// Mac 002df9b9 OGL_EndScene2
BOOL OGL_EndScene2(void)
{
    g_pTextures->EndFrame();
    OGL_SwapBuffers();
    return TRUE;
}
