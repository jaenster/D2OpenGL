#ifndef D2OPENGL_RENDERER_TEXTURES_H
#define D2OPENGL_RENDERER_TEXTURES_H

// The texture manager classes. COGLTextures and COGLAGPTextures are siblings, both derived directly
// from the abstract CD2Textures (Mac RTTI). All three keep the Mac field offsets and the Mac vtable
// order (Itanium on both sides): 0 complete dtor, 1 deleting dtor, then slots 2-9 below. The Mac
// passes `this` on the stack; here every method is an ordinary member.
//
// The Mac destructor variants D0 (deleting), D1 (complete) and D2 (base object) are all generated
// from one C++ destructor, and the C1 constructor thunks from one constructor; the provenance line of
// each declaration lists every Mac address it stands for.
//
// Objects are allocated with Storm operator new (Mac 000095fc: SMemAlloc(size, STORM.CPP, 35, 0)),
// which the class-level operators below reproduce through g_host.

#include "types.h"

class CD2Textures {
public:
    static void *operator new(size_t nSize);
    static void operator delete(void *p);

    // Mac 002b106e ~CD2Textures_D2, 002b1064 ~CD2Textures (D1), 002b104a ~CD2Textures_D0 (D0)
    virtual ~CD2Textures();
    // Mac 002b10e6
    virtual int CreateTexture(const void *pPixels, int nWidth, int nHeight, int nOwnerType, void *pOwner,
                              int nOwnerIndex, int nOwnerKey);
    // Mac 002b16de
    virtual void EndFrame();
    // Mac 002b1700
    virtual int GetFreeMemory(bool bExcludeReserve);
    virtual bool UploadTexture(D2TextureNode *pNode, const void *pPixels) = 0;         // slot 5
    virtual void ReleaseTexture(D2TextureNode *pNode) = 0;                             // slot 6
    virtual void BindTexture(D2TextureNode *pNode) = 0;                                // slot 7
    virtual bool IsTextureBound(D2TextureNode *pNode) = 0;                             // slot 8
    virtual void ConvertPixels(int nTexWidth, int nTexHeight, int nSrcWidth, int nSrcHeight,
                               const BYTE *pSrc) = 0;                                  // slot 9

    // Mac 002b1008
    void Flush();
    // Mac 002b1012
    void FreeAllTextures();
    // Mac 002b1290
    void FreeTexture(D2TextureNode *pNode);
    // Mac 002b1372
    D2TextureNode *AllocNode();
    // Mac 002b13b8
    void SetNodeOwner(D2TextureNode *pNode, int nOwnerType, void *pOwner, int nOwnerIndex, int nOwnerKey);
    // Mac 002b14b4
    void ListAppend(D2TextureNode **ppHead, D2TextureNode **ppTail, D2TextureNode *pNode);
    // Mac 002b156c
    void FreeTextureById(int nTextureId);
    // Mac 002b159e
    void BindTextureById(int nTextureId);
    // Mac 002b1600
    D2TextureNode *ListRemove(D2TextureNode **ppHead, D2TextureNode **ppTail, int nIndex);
    // Mac 002b1718
    void GrowNodePool();

protected:
    // Mac 002b0e04
    CD2Textures(int nMaxWidth, int nMaxHeight, int nTextureOverhead, int nMemoryBudget);
    // Mac 002b0f36
    void InitStaging(int nBytesPerPixel);

public:
    // +0x00 vptr
    int m_nUnknown04;               // +0x04
    int m_nReserveBytes;            // +0x08
    int m_nMaxWidth;                // +0x0C
    int m_nMaxHeight;               // +0x10
    int m_nTextureOverhead;         // +0x14
    int m_nMemoryBudget;            // +0x18
    int m_nBytesPerPixel;           // +0x1C
    int m_anUnknown20[6];           // +0x20
    void *m_pStagingAlloc;          // +0x38
    BYTE *m_pStaging;               // +0x3C  32-byte aligned
    int m_nUsedBytes;               // +0x40
    D2TextureNode *m_pNodes;        // +0x44  read directly by OGL_SpriteBindTexture
    int m_nNodeCount;               // +0x48
    D2TextureNode *m_pUsedHead;     // +0x4C  LRU end
    D2TextureNode *m_pUsedTail;     // +0x50  MRU end
    D2TextureNode *m_pFreeHead;     // +0x54
    D2TextureNode *m_pFreeTail;     // +0x58
};

class COGLTextures : public CD2Textures {
public:
    // Mac 002de2da (C2), 002de2d0 (C1 thunk)
    explicit COGLTextures(int nMemoryBudget);
    // Mac 002de398 ~COGLTextures_D2, 002de38e ~COGLTextures (D1), 002de374 ~COGLTextures_D0 (D0)
    ~COGLTextures() override;
    // Mac 002de3ce
    bool UploadTexture(D2TextureNode *pNode, const void *pPixels) override;
    // Mac 002de4fa
    void ReleaseTexture(D2TextureNode *pNode) override;
    // Mac 002de52a
    void BindTexture(D2TextureNode *pNode) override;
    // Mac 002de552
    bool IsTextureBound(D2TextureNode *pNode) override;
    // Mac 002de56a
    void ConvertPixels(int nTexWidth, int nTexHeight, int nSrcWidth, int nSrcHeight, const BYTE *pSrc) override;

    GLuint m_nBoundTexture;         // +0x5C
    bool m_bPackedPixels;           // +0x60  GL_APPLE_packed_pixel present
};

class COGLAGPTextures : public CD2Textures {
public:
    // Mac 002e5f9a (C2), 002e5f90 (C1 thunk)
    COGLAGPTextures();
    // Mac 002e66ae ~COGLAGPTextures_D2, 002e66a4 ~COGLAGPTextures (D1), 002e668a ~COGLAGPTextures_D0 (D0)
    ~COGLAGPTextures() override;
    // Mac 002e6e00
    int CreateTexture(const void *pPixels, int nWidth, int nHeight, int nOwnerType, void *pOwner,
                      int nOwnerIndex, int nOwnerKey) override;
    // Mac 002e67a4
    void EndFrame() override;
    // Mac 002e68c4
    bool UploadTexture(D2TextureNode *pNode, const void *pPixels) override;
    // Mac 002e69e2
    void ReleaseTexture(D2TextureNode *pNode) override;
    // Mac 002e6bb4
    void BindTexture(D2TextureNode *pNode) override;
    // Mac 002e6c18
    bool IsTextureBound(D2TextureNode *pNode) override;
    // Mac 002e6c32
    void ConvertPixels(int nTexWidth, int nTexHeight, int nSrcWidth, int nSrcHeight, const BYTE *pSrc) override;
    // Mac 002e67fe
    bool AllocRecord(D2TextureNode *pNode, BYTE *pPixels);
    // Mac 002e6fa4
    void AcquireSlot(int nClass);

    GLuint m_nBoundTexture;         // +0x5C
    bool m_bPackedPixels;           // +0x60
    GLenum m_eTarget;               // +0x64  GL_TEXTURE_RECTANGLE_ARB, or GL_TEXTURE_2D on Rage 128
    void *m_pArenaAlloc;            // +0x68  malloc
    BYTE *m_pArena;                 // +0x6C  page-aligned
    int m_nArenaSize;               // +0x70
    int m_anUnknown74[6];           // +0x74
    BYTE *m_apPoolBase[6];          // +0x8C
    void *m_pPageBufferAlloc;       // +0xA4
    BYTE *m_pPageBuffer;            // +0xA8
    AGPSlot m_aSlots5[64];          // +0xAC   256x256
    AGPSlot m_aSlots4[256];         // +0x2AC  160x80
    AGPSlot m_aSlots3[256];         // +0xAAC  128x128
    AGPSlot m_aSlots2[2048];        // +0x12AC 64x64
    AGPSlot m_aSlots1[2048];        // +0x52AC 32x32
    AGPSlot m_aSlots0[512];         // +0x92AC 16x16
    AGPSlot *m_apSlots[6];          // +0xA2AC m_apSlots[i] = the class-i array
    AGPPage m_aPages[1];            // +0xA2C4
    int m_anCursor[6];              // +0xA2D0
    int m_anUnknownA2E8[6];         // +0xA2E8
    int m_anProbeCount[6];          // +0xA300
    int m_nPageCount;               // +0xA318
};

static_assert(sizeof(CD2Textures) == 0x5c, "CD2Textures");
static_assert(sizeof(COGLTextures) == 0x64, "COGLTextures");
static_assert(sizeof(COGLAGPTextures) == 0xa31c, "COGLAGPTextures");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
static_assert(offsetof(CD2Textures, m_nUnknown04) == 0x04, "CD2Textures.m_nUnknown04");
static_assert(offsetof(CD2Textures, m_nReserveBytes) == 0x08, "CD2Textures.m_nReserveBytes");
static_assert(offsetof(CD2Textures, m_nMaxWidth) == 0x0c, "CD2Textures.m_nMaxWidth");
static_assert(offsetof(CD2Textures, m_nMaxHeight) == 0x10, "CD2Textures.m_nMaxHeight");
static_assert(offsetof(CD2Textures, m_nTextureOverhead) == 0x14, "CD2Textures.m_nTextureOverhead");
static_assert(offsetof(CD2Textures, m_nMemoryBudget) == 0x18, "CD2Textures.m_nMemoryBudget");
static_assert(offsetof(CD2Textures, m_nBytesPerPixel) == 0x1c, "CD2Textures.m_nBytesPerPixel");
static_assert(offsetof(CD2Textures, m_anUnknown20) == 0x20, "CD2Textures.m_anUnknown20");
static_assert(offsetof(CD2Textures, m_pStagingAlloc) == 0x38, "CD2Textures.m_pStagingAlloc");
static_assert(offsetof(CD2Textures, m_pStaging) == 0x3c, "CD2Textures.m_pStaging");
static_assert(offsetof(CD2Textures, m_nUsedBytes) == 0x40, "CD2Textures.m_nUsedBytes");
static_assert(offsetof(CD2Textures, m_pNodes) == 0x44, "CD2Textures.m_pNodes");
static_assert(offsetof(CD2Textures, m_nNodeCount) == 0x48, "CD2Textures.m_nNodeCount");
static_assert(offsetof(CD2Textures, m_pUsedHead) == 0x4c, "CD2Textures.m_pUsedHead");
static_assert(offsetof(CD2Textures, m_pUsedTail) == 0x50, "CD2Textures.m_pUsedTail");
static_assert(offsetof(CD2Textures, m_pFreeHead) == 0x54, "CD2Textures.m_pFreeHead");
static_assert(offsetof(CD2Textures, m_pFreeTail) == 0x58, "CD2Textures.m_pFreeTail");

static_assert(offsetof(COGLTextures, m_nBoundTexture) == 0x5c, "COGLTextures.m_nBoundTexture");
static_assert(offsetof(COGLTextures, m_bPackedPixels) == 0x60, "COGLTextures.m_bPackedPixels");

static_assert(offsetof(COGLAGPTextures, m_nBoundTexture) == 0x5c, "COGLAGPTextures.m_nBoundTexture");
static_assert(offsetof(COGLAGPTextures, m_bPackedPixels) == 0x60, "COGLAGPTextures.m_bPackedPixels");
static_assert(offsetof(COGLAGPTextures, m_eTarget) == 0x64, "COGLAGPTextures.m_eTarget");
static_assert(offsetof(COGLAGPTextures, m_pArenaAlloc) == 0x68, "COGLAGPTextures.m_pArenaAlloc");
static_assert(offsetof(COGLAGPTextures, m_pArena) == 0x6c, "COGLAGPTextures.m_pArena");
static_assert(offsetof(COGLAGPTextures, m_nArenaSize) == 0x70, "COGLAGPTextures.m_nArenaSize");
static_assert(offsetof(COGLAGPTextures, m_anUnknown74) == 0x74, "COGLAGPTextures.m_anUnknown74");
static_assert(offsetof(COGLAGPTextures, m_apPoolBase) == 0x8c, "COGLAGPTextures.m_apPoolBase");
static_assert(offsetof(COGLAGPTextures, m_pPageBufferAlloc) == 0xa4, "COGLAGPTextures.m_pPageBufferAlloc");
static_assert(offsetof(COGLAGPTextures, m_pPageBuffer) == 0xa8, "COGLAGPTextures.m_pPageBuffer");
static_assert(offsetof(COGLAGPTextures, m_aSlots5) == 0xac, "COGLAGPTextures.m_aSlots5");
static_assert(offsetof(COGLAGPTextures, m_aSlots4) == 0x2ac, "COGLAGPTextures.m_aSlots4");
static_assert(offsetof(COGLAGPTextures, m_aSlots3) == 0xaac, "COGLAGPTextures.m_aSlots3");
static_assert(offsetof(COGLAGPTextures, m_aSlots2) == 0x12ac, "COGLAGPTextures.m_aSlots2");
static_assert(offsetof(COGLAGPTextures, m_aSlots1) == 0x52ac, "COGLAGPTextures.m_aSlots1");
static_assert(offsetof(COGLAGPTextures, m_aSlots0) == 0x92ac, "COGLAGPTextures.m_aSlots0");
static_assert(offsetof(COGLAGPTextures, m_apSlots) == 0xa2ac, "COGLAGPTextures.m_apSlots");
static_assert(offsetof(COGLAGPTextures, m_aPages) == 0xa2c4, "COGLAGPTextures.m_aPages");
static_assert(offsetof(COGLAGPTextures, m_anCursor) == 0xa2d0, "COGLAGPTextures.m_anCursor");
static_assert(offsetof(COGLAGPTextures, m_anUnknownA2E8) == 0xa2e8, "COGLAGPTextures.m_anUnknownA2E8");
static_assert(offsetof(COGLAGPTextures, m_anProbeCount) == 0xa300, "COGLAGPTextures.m_anProbeCount");
static_assert(offsetof(COGLAGPTextures, m_nPageCount) == 0xa318, "COGLAGPTextures.m_nPageCount");
#pragma GCC diagnostic pop

#endif
