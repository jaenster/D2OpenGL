// The HD pack (hdpack.h).

#include "hdpack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <d2util.h>

#include "../../common/log.h"

namespace {

// One pack file: its entry table in memory, its images read from the file on a hit, so the size of
// the packs never counts against the 32-bit process's address space.
struct Pack {
    HANDLE hFile;
    uint32_t nCount;
    int nEntryLen;    // 16 (version 1) or 20 (version 2)
    uint8_t *pTable;  // nCount entries: u64 key, u16 bw, u16 bh, u32 offset, and in version 2 u32 length
    uint64_t nSize;
};

const int kHeaderLen = 16;
const int kMaxPacks = 64;

bool s_bOpened;
Pack s_aPacks[kMaxPacks];
int s_nPacks;
int s_nScale;

uint8_t *s_pImage;  // the last image read
size_t s_nImage;
uint8_t *s_pStored;  // a version 2 entry as stored, before it is inflated
size_t s_nStored;

unsigned s_nLookups;
unsigned s_nHits;

const unsigned kLogEvery = 500;

uint64_t Read64(const uint8_t *p)
{
    uint64_t n;
    memcpy(&n, p, 8);
    return n;
}

uint32_t Read32(const uint8_t *p)
{
    uint32_t n;
    memcpy(&n, p, 4);
    return n;
}

uint16_t Read16(const uint8_t *p)
{
    uint16_t n;
    memcpy(&n, p, 2);
    return n;
}

bool ReadAt(HANDLE hFile, uint64_t nOffset, void *pDst, DWORD nLength)
{
    OVERLAPPED ov = {};
    ov.Offset = (DWORD)nOffset;
    ov.OffsetHigh = (DWORD)(nOffset >> 32);
    DWORD nRead = 0;
    return ReadFile(hFile, pDst, nLength, &nRead, &ov) && nRead == nLength;
}

// Opens one pack file: reads and checks its header and entry table. Packs whose scale differs from
// the first one's are left out.
void OpenFile(const char *szPath)
{
    if (s_nPacks == kMaxPacks) {
        d2log("hdpack: %s: more than %d packs, left out", szPath, kMaxPacks);
        return;
    }
    HANDLE hFile = CreateFileA(szPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        d2log("hdpack: %s does not open (%lu)", szPath, GetLastError());
        return;
    }
    LARGE_INTEGER size;
    uint8_t aHeader[kHeaderLen];
    if (!GetFileSizeEx(hFile, &size) || !ReadAt(hFile, 0, aHeader, kHeaderLen)) {
        d2log("hdpack: %s does not read", szPath);
        CloseHandle(hFile);
        return;
    }
    uint32_t nCount = Read32(aHeader + 12);
    uint32_t nVersion = Read32(aHeader + 4);
    int nEntryLen = nVersion == 2 ? 20 : 16;
    uint64_t nTableLen = (uint64_t)nCount * nEntryLen;
    uint8_t *pHead = nTableLen + kHeaderLen <= (uint64_t)size.QuadPart && nTableLen < 0x40000000
                         ? static_cast<uint8_t *>(malloc((size_t)nTableLen + kHeaderLen))
                         : NULL;
    // The header as libd2 writes it (d2util.h: "D2HD", version 1 or 2, scale 1..16, count); each
    // entry's image is checked against the file size when it is read.
    uint32_t nScale = Read32(aHeader + 8);
    if (!pHead || memcmp(aHeader, "D2HD", 4) != 0 || (nVersion != 1 && nVersion != 2) || nScale < 1 || nScale > 16 ||
        !ReadAt(hFile, 0, pHead, (DWORD)nTableLen + kHeaderLen)) {
        d2log("hdpack: %s is not a valid HD pack", szPath);
        free(pHead);
        CloseHandle(hFile);
        return;
    }
    if (s_nScale && (int)nScale != s_nScale) {
        d2log("hdpack: %s: scale %u, the first pack's is %d; left out", szPath, nScale, s_nScale);
        free(pHead);
        CloseHandle(hFile);
        return;
    }
    s_nScale = (int)nScale;
    // Keep only the table.
    memmove(pHead, pHead + kHeaderLen, (size_t)nTableLen);
    Pack &pack = s_aPacks[s_nPacks++];
    pack.hFile = hFile;
    pack.nCount = nCount;
    pack.nEntryLen = nEntryLen;
    pack.pTable = pHead;
    pack.nSize = (uint64_t)size.QuadPart;
    d2log("hdpack: %s, version %u, scale %u, %u entries, %u MB", szPath, nVersion, nScale, nCount,
          (unsigned)(pack.nSize >> 20));
}

int Compare(const uint8_t *pEntry, uint64_t nKey, uint16_t nWidth, uint16_t nHeight)
{
    uint64_t k = Read64(pEntry);
    if (k != nKey)
        return k < nKey ? -1 : 1;
    uint16_t w = Read16(pEntry + 8);
    if (w != nWidth)
        return w < nWidth ? -1 : 1;
    uint16_t h = Read16(pEntry + 10);
    if (h != nHeight)
        return h < nHeight ? -1 : 1;
    return 0;
}

bool Reserve(uint8_t **ppBuf, size_t *pnSize, size_t nLength)
{
    if (nLength <= *pnSize)
        return true;
    uint8_t *pNew = static_cast<uint8_t *>(realloc(*ppBuf, nLength));
    if (!pNew)
        return false;
    *ppBuf = pNew;
    *pnSize = nLength;
    return true;
}

// The image of (key, w, h) in one pack, read (and in version 2 inflated) into s_pImage; NULL when the
// pack does not hold it.
const uint8_t *Lookup(const Pack &pack, uint64_t nKey, uint16_t nWidth, uint16_t nHeight)
{
    uint32_t lo = 0;
    uint32_t hi = pack.nCount;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2;
        const uint8_t *pEntry = pack.pTable + (size_t)mid * pack.nEntryLen;
        int c = Compare(pEntry, nKey, nWidth, nHeight);
        if (c < 0) {
            lo = mid + 1;
        } else if (c > 0) {
            hi = mid;
        } else {
            size_t nLength = (size_t)nWidth * s_nScale * nHeight * s_nScale;
            uint32_t nOffset = Read32(pEntry + 12);
            // Version 2 stores a zlib stream, or the raw image when that is no smaller.
            size_t nStored = pack.nEntryLen == 20 ? Read32(pEntry + 16) : nLength;
            if (nLength == 0 || nStored == 0 || nOffset + (uint64_t)nStored > pack.nSize ||
                !Reserve(&s_pImage, &s_nImage, nLength))
                return NULL;
            if (nStored == nLength)
                return ReadAt(pack.hFile, nOffset, s_pImage, (DWORD)nLength) ? s_pImage : NULL;
            if (!Reserve(&s_pStored, &s_nStored, nStored) || !ReadAt(pack.hFile, nOffset, s_pStored, (DWORD)nStored))
                return NULL;
            return d2_hdpack_inflate(s_pStored, nStored, s_pImage, nLength) == 0 ? s_pImage : NULL;
        }
    }
    return NULL;
}

}  // namespace

int HDPack_Open(const char *szPath)
{
    if (s_bOpened)
        return s_nScale;
    s_bOpened = true;
    if (!szPath) {
        d2log("hdpack: none");
        return 0;
    }
    DWORD nAttributes = GetFileAttributesA(szPath);
    if (nAttributes != INVALID_FILE_ATTRIBUTES && (nAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        // A folder: every .hd file in it, in name order (FindFirstFile's order is not guaranteed).
        char szPattern[MAX_PATH];
        if (snprintf(szPattern, MAX_PATH, "%s\\*.hd", szPath) >= MAX_PATH)
            return 0;
        char aNames[kMaxPacks][MAX_PATH];
        int nNames = 0;
        WIN32_FIND_DATAA find;
        HANDLE hFind = FindFirstFileA(szPattern, &find);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && nNames < kMaxPacks)
                    lstrcpynA(aNames[nNames++], find.cFileName, MAX_PATH);
            } while (FindNextFileA(hFind, &find));
            FindClose(hFind);
        }
        qsort(aNames, nNames, MAX_PATH, [](const void *a, const void *b) {
            return lstrcmpiA(static_cast<const char *>(a), static_cast<const char *>(b));
        });
        for (int i = 0; i < nNames; i++) {
            char szFile[MAX_PATH];
            if (snprintf(szFile, MAX_PATH, "%s\\%s", szPath, aNames[i]) < MAX_PATH)
                OpenFile(szFile);
        }
        if (!s_nPacks)
            d2log("hdpack: %s holds no usable .hd file", szPath);
    } else {
        OpenFile(szPath);
    }
    return s_nPacks ? s_nScale : 0;
}

int HDPack_Scale(void)
{
    return s_nPacks ? s_nScale : 0;
}

bool HDPack_Find(const uint8_t *pIndices, int nWidth, int nHeight, int nPitch, HDPackFrame *pFrame)
{
    if (!s_nPacks)
        return false;
    bool bHit = false;
    uint64_t nKey = 0;
    int32_t aBox[4];
    if (d2_frame_key(pIndices, nWidth, nHeight, nPitch, &nKey, aBox) == 0 && aBox[2] > 0) {
        for (int i = 0; i < s_nPacks && !bHit; i++) {
            const uint8_t *pPixels = Lookup(s_aPacks[i], nKey, (uint16_t)aBox[2], (uint16_t)aBox[3]);
            if (pPixels) {
                pFrame->x0 = aBox[0];
                pFrame->y0 = aBox[1];
                pFrame->w = aBox[2];
                pFrame->h = aBox[3];
                pFrame->pPixels = pPixels;
                bHit = true;
            }
        }
    }
    if (bHit)
        ++s_nHits;
    if (++s_nLookups % kLogEvery == 0)
        d2log("hdpack: %u lookups, %u hits, %u misses", s_nLookups, s_nHits, s_nLookups - s_nHits);
    return bHit;
}
