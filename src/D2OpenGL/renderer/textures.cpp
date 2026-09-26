// The texture manager: CD2Textures (CD2Textures.cpp), COGLTextures and COGLAGPTextures
// (COGLAGPTextures.cpp).

#include "renderer.h"

#include "../upscale/sprite_upscale.h"

#include <stdlib.h>

// ---- CD2Textures

// Mac 002b0e04
CD2Textures::CD2Textures(int nMaxWidth, int nMaxHeight, int nTextureOverhead, int nMemoryBudget)
{
    m_nMaxWidth = nMaxWidth;
    m_nMaxHeight = nMaxHeight;
    m_nTextureOverhead = nTextureOverhead;
    m_nMemoryBudget = nMemoryBudget;
    m_nNodeCount = 0x800;
    m_pNodes = (D2TextureNode *)g_host.AllocClientMemory(0x800 * sizeof(D2TextureNode), g_szFileCD2Textures,
                                                         105, 0);
    memset(m_pNodes, 0, 0x800 * sizeof(D2TextureNode));

    m_pUsedTail = NULL;
    m_pUsedHead = NULL;
    m_pFreeHead = m_pNodes;
    m_pNodes[0].pPrev = NULL;
    m_pFreeHead->pNext = &m_pNodes[1];
    m_pFreeHead->nId = 1;
    int nLast = m_nNodeCount - 1;
    for (int i = 1; i < nLast; i++) {
        m_pNodes[i].pPrev = &m_pNodes[i - 1];
        m_pNodes[i].pNext = &m_pNodes[i + 1];
        m_pNodes[i].nId = i + 1;
    }
    m_pFreeTail = &m_pNodes[nLast];
    m_pNodes[nLast].pPrev = &m_pNodes[nLast - 1];
    m_pFreeTail->pNext = NULL;
    m_pFreeTail->nId = m_nNodeCount;

    m_nUsedBytes = 0;
    m_nReserveBytes = 0;
    g_bAGPTextures = FALSE;
    g_bTextureRectangle = FALSE;
}

// Mac 002b0f36
void CD2Textures::InitStaging(int nBytesPerPixel)
{
    m_nBytesPerPixel = nBytesPerPixel;
    if (!g_bAGPTextures) {
        size_t nSize = m_nMaxHeight * nBytesPerPixel * m_nMaxWidth + 0x1f;
        m_pStagingAlloc = g_host.AllocClientMemory(nSize, g_szFileCD2Textures, 184, 0);
        memset(m_pStagingAlloc, 0, nSize);
        m_pStaging = (BYTE *)(((uintptr_t)m_pStagingAlloc + 0x1f) & ~(uintptr_t)0x1f);
    } else {
        m_pStagingAlloc = NULL;
        m_pStaging = NULL;
    }

    int nFree = GetFreeMemory(false);
    if (nFree <= 0x700000)
        m_nReserveBytes = nFree / 10;
    else
        m_nReserveBytes = (int)((uint32_t)nFree * 5 - 0x2300000) / 10 + 0xb3333;
}

// Mac 002b1008
void CD2Textures::Flush()
{
    FreeAllTextures();
}

// Mac 002b1012
void CD2Textures::FreeAllTextures()
{
    for (int i = 0; i < m_nNodeCount; i++)
        FreeTexture(&m_pNodes[i]);
}

// Mac 002b106e ~CD2Textures_D2 (also 002b1064 D1, 002b104a D0)
CD2Textures::~CD2Textures()
{
    g_host.FreeClientMemory(m_pNodes, g_szFileCD2Textures, 245, 0);
    if (!g_bAGPTextures)
        g_host.FreeClientMemory(m_pStagingAlloc, g_szFileCD2Textures, 254, 0);
}

// Mac 002b10e6
int CD2Textures::CreateTexture(const void *pPixels, int nWidth, int nHeight, int nOwnerType, void *pOwner,
                               int nOwnerIndex, int nOwnerKey)
{
    int nTexWidth = 2;
    while (nTexWidth < nWidth)
        nTexWidth *= 2;
    int nTexHeight = 1;
    while (nTexHeight < nHeight)
        nTexHeight *= 2;

    int nCost = nTexHeight * nTexWidth * m_nBytesPerPixel + m_nTextureOverhead;
    while (GetFreeMemory(true) < nCost)
        FreeTexture(NULL);

    D2TextureNode *pNode = AllocNode();
    const void *pUpload;
    if (nOwnerType) {
        ConvertPixels(nTexWidth, nTexHeight, nWidth, nHeight, (const BYTE *)pPixels);
        pNode->nWidth = nTexWidth;
        pNode->nHeight = nTexHeight;
        SetNodeOwner(pNode, nOwnerType, pOwner, nOwnerIndex, nOwnerKey);
        pUpload = m_pStaging;
    } else {
        pNode->nWidth = nTexWidth;
        pNode->nHeight = nTexHeight;
        SetNodeOwner(pNode, 0, pOwner, nOwnerIndex, nOwnerKey);
        pUpload = pPixels;
    }

    while (!UploadTexture(pNode, pUpload)) {
        if (!m_pUsedHead) {
            OGL_HALT(368);
            return 0;
        }
        FreeTexture(NULL);
    }

    m_nUsedBytes += nCost;
    ListAppend(&m_pUsedHead, &m_pUsedTail, pNode);
    return pNode->nId;
}

// Mac 002b1290
void CD2Textures::FreeTexture(D2TextureNode *pNode)
{
    if (!m_pUsedHead)
        return;

    if (!pNode) {
        pNode = m_pUsedHead;
        if (pNode->bUsedLastFrame) {
            pNode = m_pUsedTail;
            if (IsTextureBound(pNode))
                pNode = pNode->pPrev;
        }
    }
    if (!pNode)
        return;
    if (!pNode->hTexture)
        return;

    ReleaseTexture(pNode);
    if (pNode->pOwner) {
        BYTE *pOwner = (BYTE *)pNode->pOwner;
        switch (pNode->nOwnerType) {
        case TEXOWNER_SPRITE:
            *(int *)(pOwner + 4 + pNode->nOwnerIndex * 8) = 0;
            break;
        case TEXOWNER_BLOCK:
            *(int16_t *)(pOwner + 4) = 0;
            break;
        case TEXOWNER_TILE:
            *(int16_t *)(pOwner + 0x42) = 0;
            break;
        default:
            goto owner_done;
        }
        pNode->pOwner = NULL;
    }
owner_done:
    m_nUsedBytes -= pNode->nHeight * pNode->nWidth * m_nBytesPerPixel + m_nTextureOverhead;
    ListRemove(&m_pUsedHead, &m_pUsedTail, pNode->nId - 1);
    ListAppend(&m_pFreeHead, &m_pFreeTail, pNode);
}

// Mac 002b1372
D2TextureNode *CD2Textures::AllocNode()
{
    D2TextureNode *pNode = m_pFreeHead;
    if (!pNode) {
        GrowNodePool();
        pNode = m_pFreeHead;
    }
    ListRemove(&m_pFreeHead, &m_pFreeTail, pNode->nId - 1);
    return pNode;
}

// Mac 002b13b8
void CD2Textures::SetNodeOwner(D2TextureNode *pNode, int nOwnerType, void *pOwner, int nOwnerIndex, int nOwnerKey)
{
    pNode->nOwnerType = nOwnerType;
    switch (nOwnerType) {
    case TEXOWNER_NONE:
        pNode->pOwner = NULL;
        break;
    case TEXOWNER_TILE:
        pNode->pOwner = pOwner;
        if (*(int16_t *)((BYTE *)pOwner + 0x42) != 0)
            OGL_HALT(847);
        *(int16_t *)((BYTE *)pNode->pOwner + 0x42) = (int16_t)pNode->nId;
        break;
    case TEXOWNER_BLOCK:
        pNode->pOwner = pOwner;
        if (*(int16_t *)((BYTE *)pOwner + 4) != 0)
            OGL_HALT(853);
        *(int16_t *)((BYTE *)pNode->pOwner + 4) = (int16_t)pNode->nId;
        break;
    case TEXOWNER_SPRITE:
        pNode->pOwner = pOwner;
        pNode->nOwnerIndex = nOwnerIndex;
        pNode->nOwnerKey = nOwnerKey;
        if (*(int *)((BYTE *)pOwner + 4 + nOwnerIndex * 8) != 0)
            OGL_HALT(862);
        *(int *)((BYTE *)pNode->pOwner + 4 + pNode->nOwnerIndex * 8) = pNode->nId;
        break;
    }
}

// Mac 002b14b4
void CD2Textures::ListAppend(D2TextureNode **ppHead, D2TextureNode **ppTail, D2TextureNode *pNode)
{
    if (!pNode)
        OGL_HALT(879);
    if (!ppHead)
        OGL_HALT(880);
    if (!ppTail)
        OGL_HALT(881);

    if (!*ppHead) {
        *ppHead = pNode;
        *ppTail = pNode;
        pNode->pPrev = NULL;
        pNode->pNext = NULL;
    } else {
        (*ppTail)->pNext = pNode;
        pNode->pPrev = *ppTail;
        pNode->pNext = NULL;
        *ppTail = pNode;
    }
}

// Mac 002b156c
void CD2Textures::FreeTextureById(int nTextureId)
{
    if (nTextureId > 0 && nTextureId <= m_nNodeCount) {
        D2TextureNode *pNode = &m_pNodes[nTextureId - 1];
        if (pNode)
            FreeTexture(pNode);
    }
}

// Mac 002b159e
void CD2Textures::BindTextureById(int nTextureId)
{
    BindTexture(&m_pNodes[nTextureId - 1]);
    m_pNodes[nTextureId - 1].bUsedThisFrame = TRUE;
    D2TextureNode *pNode = ListRemove(&m_pUsedHead, &m_pUsedTail, nTextureId - 1);
    ListAppend(&m_pUsedHead, &m_pUsedTail, pNode);
}

// Mac 002b1600
D2TextureNode *CD2Textures::ListRemove(D2TextureNode **ppHead, D2TextureNode **ppTail, int nIndex)
{
    if (!ppHead)
        OGL_HALT(909);
    if (!ppTail)
        OGL_HALT(910);

    D2TextureNode *pNode = NULL;
    if (*ppHead) {
        pNode = &m_pNodes[nIndex];
        if (!pNode)
            OGL_HALT(916);
        if (!pNode->pPrev)
            *ppHead = pNode->pNext;
        else
            pNode->pPrev->pNext = pNode->pNext;
        if (!pNode->pNext)
            *ppTail = pNode->pPrev;
        else
            pNode->pNext->pPrev = pNode->pPrev;
        pNode->pNext = NULL;
        pNode->pPrev = NULL;
    }
    return pNode;
}

// Mac 002b16de
void CD2Textures::EndFrame()
{
    for (D2TextureNode *pNode = m_pUsedHead; pNode; pNode = pNode->pNext) {
        pNode->bUsedLastFrame = pNode->bUsedThisFrame;
        pNode->bUsedThisFrame = FALSE;
    }
}

// Mac 002b1700
int CD2Textures::GetFreeMemory(bool bExcludeReserve)
{
    int nFree = m_nMemoryBudget - m_nUsedBytes;
    if (bExcludeReserve)
        nFree -= m_nReserveBytes;
    return nFree;
}

// Mac 002b1718
void CD2Textures::GrowNodePool()
{
    int nOldCount = m_nNodeCount;
    m_nNodeCount = nOldCount + 0x400;
    size_t nSize = m_nNodeCount * sizeof(D2TextureNode);
    D2TextureNode *pNew = (D2TextureNode *)g_host.AllocClientMemory(nSize, g_szFileCD2Textures, 661, 0);
    memset(pNew, 0, nSize);
    memcpy(pNew, m_pNodes, nOldCount * sizeof(D2TextureNode));

    if (m_pFreeHead) {
        m_pFreeHead = &pNew[m_pFreeHead->nId - 1];
        m_pFreeTail = &pNew[m_pFreeTail->nId - 1];
    }
    if (m_pUsedHead) {
        m_pUsedHead = &pNew[m_pUsedHead->nId - 1];
        m_pUsedTail = &pNew[m_pUsedTail->nId - 1];
    }
    for (int i = 0; i < nOldCount; i++) {
        if (m_pNodes[i].pPrev)
            pNew[i].pPrev = &pNew[m_pNodes[i].pPrev->nId - 1];
        if (m_pNodes[i].pNext)
            pNew[i].pNext = &pNew[m_pNodes[i].pNext->nId - 1];
    }
    for (int i = nOldCount; i < m_nNodeCount;) {
        D2TextureNode *pNode = &pNew[i];
        pNode->nId = ++i;
        ListAppend(&m_pFreeHead, &m_pFreeTail, pNode);
    }

    g_host.FreeClientMemory(m_pNodes, g_szFileCD2Textures, 712, 0);
    m_pNodes = pNew;
}

// ---- COGLTextures

// Mac 002de2da (C2), 002de2d0 (C1 thunk)
COGLTextures::COGLTextures(int nMemoryBudget) : CD2Textures(256, 256, 0, nMemoryBudget)
{
    m_nBoundTexture = 0;
    m_bPackedPixels = strstr((const char *)glGetString(GL_EXTENSIONS), "GL_APPLE_packed_pixel") != NULL;
    // Upscale: sprite textures are made larger (upscale/sprite_upscale.h); the staging buffer holds them.
    int nUpscale = Upscale_OpenTextures();
    m_nMaxWidth *= nUpscale;
    m_nMaxHeight *= nUpscale;
    InitStaging(m_bPackedPixels ? 2 : 4);
}

// Mac 002de398 ~COGLTextures_D2 (also 002de38e D1, 002de374 D0)
COGLTextures::~COGLTextures()
{
    Flush();
    Upscale_CloseTextures();  // upscale
}

// Mac 002de3ce
bool COGLTextures::UploadTexture(D2TextureNode *pNode, const void *pPixels)
{
    glGenTextures(1, &pNode->hTexture);
    glBindTexture(GL_TEXTURE_2D, pNode->hTexture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 9728.0f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 9728.0f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 10496.0f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 10496.0f);
    if (m_bPackedPixels)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pNode->nWidth, pNode->nHeight, 0, GL_BGRA,
                     GL_UNSIGNED_SHORT_1_5_5_5_REV, pPixels);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, pNode->nWidth, pNode->nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     pPixels);
    return true;
}

// Mac 002de4fa
void COGLTextures::ReleaseTexture(D2TextureNode *pNode)
{
    if (pNode->hTexture) {
        glDeleteTextures(1, &pNode->hTexture);
        pNode->hTexture = 0;
    }
}

// Mac 002de52a
void COGLTextures::BindTexture(D2TextureNode *pNode)
{
    m_nBoundTexture = pNode->hTexture;
    glBindTexture(GL_TEXTURE_2D, m_nBoundTexture);
}

// Mac 002de552
bool COGLTextures::IsTextureBound(D2TextureNode *pNode)
{
    return m_nBoundTexture == pNode->hTexture;
}

// Mac 002de56a
void COGLTextures::ConvertPixels(int nTexWidth, int nTexHeight, int nSrcWidth, int nSrcHeight, const BYTE *pSrc)
{
    BYTE *pDst = m_pStaging + (((nTexHeight - nSrcHeight) * nTexWidth) << 2);
    if (nTexWidth & 3)
        OGL_HALT(209);
    if (nSrcWidth & 3)
        OGL_HALT(210);

    int nQuads = nSrcWidth >> 2;
    if (!m_bPackedPixels) {
        for (int y = 0; y < nSrcHeight; y++) {
            uint32_t *pRow = (uint32_t *)pDst;
            for (int x = 0; x < nQuads; x++) {
                uint32_t dwQuad;
                memcpy(&dwQuad, pSrc + x * 4, 4);
                pRow[x * 4 + 0] = g_aPaletteRGBA[dwQuad & 0xff];
                pRow[x * 4 + 1] = g_aPaletteRGBA[(dwQuad >> 8) & 0xff];
                pRow[x * 4 + 2] = g_aPaletteRGBA[(dwQuad >> 16) & 0xff];
                pRow[x * 4 + 3] = g_aPaletteRGBA[dwQuad >> 24];
            }
            if (nQuads > 0)
                pSrc += nQuads * 4;
            pDst += nTexWidth * 4;
        }
    } else {
        for (int y = 0; y < nSrcHeight; y++) {
            uint32_t *pRow = (uint32_t *)pDst;
            for (int x = 0; x < nQuads; x++) {
                uint32_t dwQuad;
                memcpy(&dwQuad, pSrc + x * 4, 4);
                pRow[x * 2 + 0] = (uint32_t)g_aPalette1555[dwQuad & 0xff] |
                                  ((uint32_t)g_aPalette1555[(dwQuad >> 8) & 0xff] << 16);
                pRow[x * 2 + 1] = (uint32_t)g_aPalette1555[(dwQuad >> 16) & 0xff] |
                                  ((uint32_t)g_aPalette1555[dwQuad >> 24] << 16);
            }
            if (nQuads > 0)
                pSrc += nQuads * 4;
            pDst += nTexWidth * 2;
        }
    }
}

// ---- COGLAGPTextures

// Mac 002e5f00
bool OGL_CanUseAGPTextures(void)
{
    const char *pszExtensions = (const char *)glGetString(GL_EXTENSIONS);
    glGetString(GL_RENDERER);
    const char *pClientStorage = strstr(pszExtensions, "GL_APPLE_client_storage");
    const char *pTextureRange = strstr(pszExtensions, "GL_APPLE_texture_range");
    if (!pClientStorage || !pTextureRange)
        return false;
    uint32_t dwPhysicalMemory = *g_host.pdwTotalPhysicalMemory;
    return strstr(pszExtensions, "GL_EXT_texture_rectangle") != NULL && dwPhysicalMemory > 0x0fffffff;
}

// Mac 002e5f9a (C2), 002e5f90 (C1 thunk)
COGLAGPTextures::COGLAGPTextures() : CD2Textures(256, 256, 0, 0)
{
    m_nBoundTexture = 0;
    const char *pRage128 = strstr((const char *)glGetString(GL_RENDERER), "Rage 128");
    m_bPackedPixels = strstr((const char *)glGetString(GL_EXTENSIONS), "GL_APPLE_packed_pixel") != NULL;
    g_bAGPTextures = TRUE;
    if (pRage128) {
        g_bTextureRectangle = FALSE;
        m_eTarget = GL_TEXTURE_2D;
        m_nPageCount = 1;
    } else {
        g_bTextureRectangle = TRUE;
        m_eTarget = GL_TEXTURE_RECTANGLE_ARB;
        m_nPageCount = 0;
    }
    InitStaging(m_bPackedPixels ? 2 : 4);

    uint32_t adwPoolSize[6];
    m_nArenaSize = 0;
    bool bPacked = m_bPackedPixels;
    uint32_t dwArena = 0;
    for (int i = 0; i < 6; i++) {
        uint32_t dwSize = ((uint32_t)((bPacked ? 2 : 4) * (g_anAGPClassHeight[i] * g_anAGPClassWidth[i]) *
                                      g_anAGPClassCount[i]) +
                           0x101f) &
                          0xfffff000;
        adwPoolSize[i] = dwSize;
        dwArena += dwSize;
        m_nArenaSize = (int)dwArena;
    }
    m_nArenaSize = (int)(dwArena + 0x1000);
    m_pArenaAlloc = malloc(dwArena + 0x2000);
    if (m_pArenaAlloc) {
        m_pArena = (BYTE *)(((uintptr_t)m_pArenaAlloc + 0xfff) & ~(uintptr_t)0xfff);
        uint32_t dwOffset = 0;
        for (int i = 0;; i++) {
            m_apPoolBase[i] = m_pArena + dwOffset;
            m_anCursor[i] = 0;
            if (i == 5)
                break;
            dwOffset += adwPoolSize[i];
        }
    }

    m_apSlots[0] = m_aSlots0;
    m_apSlots[1] = m_aSlots1;
    m_apSlots[2] = m_aSlots2;
    m_apSlots[3] = m_aSlots3;
    m_apSlots[4] = m_aSlots4;
    m_apSlots[5] = m_aSlots5;

    GLuint *pNames = (GLuint *)g_host.AllocClientMemory(0x5100, g_szFileCOGLAGPTextures, 227, 0);
    glGenTextures(0x1440, pNames);
    int nName = 0;
    for (int i = 0; i < 6; i++) {
        AGPSlot *pSlots = m_apSlots[i];
        BYTE *pPixels = m_apPoolBase[i];
        int nWidth = g_anAGPClassWidth[i];
        int nHeight = g_anAGPClassHeight[i];
        int nSlotBytes = nHeight * nWidth * m_nBytesPerPixel;
        GLint nFilter = (i == 4) | GL_NEAREST;
        int j = 0;
        do {
            pSlots[j].name = pNames[nName + j];
            if (m_eTarget != GL_TEXTURE_RECTANGLE_ARB)
                OGL_HALT(245);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, pSlots[j].name);
            glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_PRIORITY, 0.0f);
            glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, nFilter);
            glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, nFilter);
            glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            if (!m_bPackedPixels)
                glTexImage2D(m_eTarget, 0, GL_RGB5_A1, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pPixels);
            else
                glTexImage2D(m_eTarget, 0, GL_RGB5_A1, nWidth, nHeight, 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV,
                             pPixels);
            pSlots[j].nOwner = -1;
            pPixels += nSlotBytes;
            j++;
        } while (j < g_anAGPClassCount[i]);
        nName += j;
    }
    g_host.FreeClientMemory(pNames, g_szFileCOGLAGPTextures, 314, 0);

    m_pPageBufferAlloc = NULL;
    m_pPageBuffer = NULL;
    if (m_nPageCount > 0) {
        pNames = (GLuint *)g_host.AllocClientMemory(m_nPageCount << 2, g_szFileCOGLAGPTextures, 322, 0);
        glGenTextures(m_nPageCount, pNames);
        m_pPageBufferAlloc = malloc(m_bPackedPixels ? 0x10020 : 0x20020);
        m_pPageBuffer = (BYTE *)(((uintptr_t)m_pPageBufferAlloc + 0x1f) & ~(uintptr_t)0x1f);
        for (int k = 0; k < m_nPageCount; k++) {
            AGPPage *pPage = &m_aPages[k];
            pPage->name = pNames[k];
            pPage->nOwner = -1;
            pPage->nUnknown08 = 0;
            glBindTexture(GL_TEXTURE_2D, pPage->name);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            if (!m_bPackedPixels)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, 256, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pPageBuffer);
            else
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, 256, 128, 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV,
                             m_pPageBuffer);
        }
        g_host.FreeClientMemory(pNames, g_szFileCOGLAGPTextures, 379, 0);
    }
}

// Mac 002e66ae ~COGLAGPTextures_D2 (also 002e66a4 D1, 002e668a D0)
COGLAGPTextures::~COGLAGPTextures()
{
    Flush();
    for (int i = 0; i < 6; i++) {
        AGPSlot *pSlot = m_apSlots[i];
        int j = 0;
        do {
            glDeleteTextures(1, &pSlot->name);
            pSlot++;
            j++;
        } while (j < g_anAGPClassCount[i]);
    }
    for (int k = 0; k < m_nPageCount; k++)
        glDeleteTextures(1, &m_aPages[k].name);
    glFlush();
    if (m_pArenaAlloc)
        free(m_pArenaAlloc);
    if (m_pPageBufferAlloc) {
        free(m_pPageBufferAlloc);
        m_pPageBufferAlloc = NULL;
    }
}

// Mac 002e67a4
void COGLAGPTextures::EndFrame()
{
    CD2Textures::EndFrame();
    for (D2TextureNode *pNode = m_pUsedHead; pNode; pNode = pNode->pNext) {
        if (!pNode->pAGP)
            OGL_HALT(530);
        pNode->pAGP->nUsageBits <<= 1;
    }
}

// Mac 002e67fe
bool COGLAGPTextures::AllocRecord(D2TextureNode *pNode, BYTE *pPixels)
{
    AGPTextureRecord *pRecord =
        (AGPTextureRecord *)g_host.AllocClientMemory(sizeof(AGPTextureRecord), g_szFileCOGLAGPTextures, 632, 0);
    pNode->pAGP = pRecord;
    int nClass;
    for (nClass = 0; nClass < 6; nClass++) {
        if (pNode->nWidth <= g_anAGPClassWidth[nClass] && pNode->nHeight <= g_anAGPClassHeight[nClass])
            break;
    }
    if (nClass == 6) {
        OGL_HALT(622);
        nClass = 5;
    }
    pRecord->name = m_apSlots[nClass][m_anCursor[nClass]].name;
    pRecord->hPage2D = 0;
    pRecord->nUsageBits = 0;
    pRecord->pPixels = pPixels;
    return true;
}

// Mac 002e68c4
bool COGLAGPTextures::UploadTexture(D2TextureNode *pNode, const void *pPixels)
{
    AGPTextureRecord *pRecord = pNode->pAGP;
    int nClass;
    for (nClass = 0; nClass < 6; nClass++) {
        if (pNode->nWidth <= g_anAGPClassWidth[nClass] && pNode->nHeight <= g_anAGPClassHeight[nClass])
            break;
    }
    if (nClass == 6) {
        OGL_HALT(622);
        nClass = 5;
    }
    glBindTexture(m_eTarget, pRecord->name);
    if (!m_bPackedPixels)
        glTexSubImage2D(m_eTarget, 0, 0, 0, pNode->nWidth, pNode->nHeight, GL_RGBA, GL_UNSIGNED_BYTE, pPixels);
    else
        glTexSubImage2D(m_eTarget, 0, 0, 0, pNode->nWidth, pNode->nHeight, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV,
                        pPixels);
    m_apSlots[nClass][m_anCursor[nClass]].nOwner = pNode ? pNode->nId : -1;
    return true;
}

// Mac 002e69e2
void COGLAGPTextures::ReleaseTexture(D2TextureNode *pNode)
{
    if (!pNode)
        OGL_HALT(735);
    AGPTextureRecord *pRecord = pNode->pAGP;
    if (!pRecord)
        OGL_HALT(736);

    if (pRecord->name)
        pRecord->name = 0;

    if (pRecord->hPage2D) {
        for (int k = 0; k < m_nPageCount; k++) {
            if (m_aPages[k].name == pRecord->hPage2D)
                m_aPages[k].nOwner = -1;
        }
        pRecord->hPage2D = 0;
    }

    if (pRecord->pPixels) {
        int nClass;
        int nWidth = 0;
        int nHeight = 0;
        for (nClass = 0; nClass < 6; nClass++) {
            nWidth = g_anAGPClassWidth[nClass];
            nHeight = g_anAGPClassHeight[nClass];
            if (pNode->nWidth <= nWidth && pNode->nHeight <= nHeight)
                break;
        }
        if (nClass == 6) {
            OGL_HALT(622);
            nClass = 5;
            nHeight = 256;
            nWidth = 256;
        }
        int nIndex = (int)(pRecord->pPixels - m_apPoolBase[nClass]) / (nWidth * nHeight * m_nBytesPerPixel);
        if (nIndex < 0 || nIndex >= g_anAGPClassCount[nClass])
            OGL_HALT(779);
        m_apSlots[nClass][nIndex].nOwner = -1;
        pRecord->pPixels = NULL;
    }

    g_host.FreeClientMemory(pNode->pAGP, g_szFileCOGLAGPTextures, 785, 0);
    pNode->pAGP = NULL;
}

// Mac 002e6bb4
void COGLAGPTextures::BindTexture(D2TextureNode *pNode)
{
    AGPTextureRecord *pRecord = pNode->pAGP;
    m_nBoundTexture = pRecord->name;
    pRecord->nUsageBits |= 1;
    if (!pRecord->hPage2D) {
        g_bBoundPage2D = FALSE;
        glBindTexture(m_eTarget, m_nBoundTexture);
    } else {
        glBindTexture(GL_TEXTURE_2D, pRecord->hPage2D);
        g_bBoundPage2D = TRUE;
    }
}

// Mac 002e6c18
bool COGLAGPTextures::IsTextureBound(D2TextureNode *pNode)
{
    return m_nBoundTexture == pNode->pAGP->name;
}

// Mac 002e6c32
void COGLAGPTextures::ConvertPixels(int nTexWidth, int nTexHeight, int nSrcWidth, int nSrcHeight, const BYTE *pSrc)
{
    if (!m_pStaging)
        OGL_HALT(873);
    BYTE *pStaging = m_pStaging;
    if (nTexWidth & 3)
        OGL_HALT(878);
    if (nSrcWidth & 3)
        OGL_HALT(879);

    BYTE *pDst = pStaging + (((nTexHeight - nSrcHeight) * nTexWidth) << 2);
    int nQuads = nSrcWidth >> 2;
    if (!m_bPackedPixels) {
        for (int y = 0; y < nSrcHeight; y++) {
            uint32_t *pRow = (uint32_t *)pDst;
            for (int x = 0; x < nQuads; x++) {
                uint32_t dwQuad;
                memcpy(&dwQuad, pSrc + x * 4, 4);
                pRow[x * 4 + 0] = g_aPaletteRGBA[dwQuad & 0xff];
                pRow[x * 4 + 1] = g_aPaletteRGBA[(dwQuad >> 8) & 0xff];
                pRow[x * 4 + 2] = g_aPaletteRGBA[(dwQuad >> 16) & 0xff];
                pRow[x * 4 + 3] = g_aPaletteRGBA[dwQuad >> 24];
            }
            if (nQuads > 0)
                pSrc += nQuads * 4;
            pDst += nTexWidth * 4;
        }
    } else {
        for (int y = 0; y < nSrcHeight; y++) {
            uint32_t *pRow = (uint32_t *)pDst;
            for (int x = 0; x < nQuads; x++) {
                uint32_t dwQuad;
                memcpy(&dwQuad, pSrc + x * 4, 4);
                pRow[x * 2 + 0] = (uint32_t)g_aPalette1555[dwQuad & 0xff] |
                                  ((uint32_t)g_aPalette1555[(dwQuad >> 8) & 0xff] << 16);
                pRow[x * 2 + 1] = (uint32_t)g_aPalette1555[(dwQuad >> 16) & 0xff] |
                                  ((uint32_t)g_aPalette1555[dwQuad >> 24] << 16);
            }
            if (nQuads > 0)
                pSrc += nQuads * 4;
            pDst += nTexWidth * 2;
        }
    }
}

// Mac 002e6e00
int COGLAGPTextures::CreateTexture(const void *pPixels, int nWidth, int nHeight, int nOwnerType, void *pOwner,
                                   int nOwnerIndex, int nOwnerKey)
{
    int nClass;
    int nClassWidth = 0;
    int nClassHeight = 0;
    for (nClass = 0; nClass < 6; nClass++) {
        nClassWidth = g_anAGPClassWidth[nClass];
        nClassHeight = g_anAGPClassHeight[nClass];
        if (nWidth <= nClassWidth && nHeight <= nClassHeight)
            break;
    }
    if (nClass == 6) {
        OGL_HALT(622);
        nClass = 5;
        nClassHeight = 256;
        nClassWidth = 256;
    }

    AcquireSlot(nClass);
    D2TextureNode *pNode = AllocNode();
    SetNodeOwner(pNode, nOwnerType, pOwner, nOwnerIndex, nOwnerKey);
    if (nOwnerType == TEXOWNER_NONE) {
        OGL_HALT(980);
        pNode->nWidth = nClassWidth;
        pNode->nHeight = nClassHeight;
    } else {
        pNode->nWidth = nClassWidth;
        pNode->nHeight = nClassHeight;
        m_pStaging = m_apPoolBase[nClass] + m_nBytesPerPixel * nClassHeight * nClassWidth * m_anCursor[nClass];
        AllocRecord(pNode, m_pStaging);
        ConvertPixels(nClassWidth, nClassHeight, nWidth, nHeight, (const BYTE *)pPixels);
        UploadTexture(pNode, m_pStaging);
    }

    m_nUsedBytes += nClassHeight * nClassWidth * m_nBytesPerPixel + m_nTextureOverhead;
    ListAppend(&m_pUsedHead, &m_pUsedTail, pNode);
    return pNode->nId;
}

// Mac 002e6fa4
void COGLAGPTextures::AcquireSlot(int nClass)
{
    int *pCursor = &m_anCursor[nClass];
    int nFound = -1;
    int nStale = -1;
    int nProbe = 0;
    int nCount;
    do {
        if (nFound >= 0)
            break;
        int nSlot = ++*pCursor;
        nCount = g_anAGPClassCount[nClass];
        if (nSlot >= nCount - 1) {
            *pCursor = 0;
            nSlot = 0;
        }
        m_anProbeCount[nClass]++;
        int nOwner = m_apSlots[nClass][nSlot].nOwner;
        D2TextureNode *pOwnerNode = nOwner == -1 ? NULL : &m_pNodes[nOwner - 1];
        if (!pOwnerNode)
            nFound = nSlot;
        else if (nStale < 0 && !pOwnerNode->bUsedLastFrame)
            nStale = nSlot;
        nProbe++;
    } while (nProbe < nCount);

    bool bFence;
    if (nFound >= 0) {
        *pCursor = nFound;
        bFence = false;
    } else if (nStale >= 0) {
        m_anCursor[nClass] = nStale;
        nFound = nStale;
        bFence = false;
    } else {
        g_nAGPRoundRobin = (g_nAGPRoundRobin + 1) & 3;
        m_anCursor[nClass] = g_nAGPRoundRobin;
        nFound = g_nAGPRoundRobin;
        bFence = true;
    }

    int nOwner = m_apSlots[nClass][nFound].nOwner;
    if (nOwner == -1)
        return;
    D2TextureNode *pNode = &m_pNodes[nOwner - 1];
    if (bFence && pNode && pNode->pAGP)
        Platform_FinishTextureObject(pNode->pAGP->name);
    else if (!pNode)
        return;
    FreeTexture(pNode);
}
