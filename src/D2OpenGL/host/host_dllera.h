#ifndef D2OPENGL_HOST_DLLERA_H
#define D2OPENGL_HOST_DLLERA_H

// The DLL-era games (1.13c and older) export the renderer's game dependencies from their own DLLs. Each
// build describes where every HostApi entry lives; host_bind_dllera resolves them once the DLLs are loaded.

#include "host.h"

// Where one HostApi entry lives: an ordinal export, or an RVA inside the module. A NULL module means the
// build supplies that entry itself.
struct HostImport {
    const char *module;
    unsigned ordinal;
    uint32_t rva;
};

#define HOST_DLLERA_ENTRIES(X)                                                                           \
    X(SMemAlloc) X(SMemFree) X(SBmpDecodeImage) X(SFileLoadFileEx) X(SFileOpenFileEx)                  \
    X(Halt) X(LogWrite) X(AllocClientMemory) X(FreeClientMemory) X(GetUseDirectCommand)                \
    X(BuildGammaRamp) X(SetFont) X(DrawGameText) X(Utf8ToWide) X(SetGlobalVolume) X(GetHwnd)           \
    X(GetScreenSize) X(HideOSCursor) X(ShowOSCursor) X(DrawCelFrame) X(GetDC6Width) X(GetDC6Height)    \
    X(GetDC6OffsetX) X(GetDC6OffsetY) X(SetPixelDataFreeCallback) X(GetDC6BlockPixelData)              \
    X(GetOrLoadSprite) X(AllocTileEntry) X(SetTileFreeCallback) X(pdwTotalPhysicalMemory)

struct HostImportTable {
#define HOST_DLLERA_FIELD(name) HostImport name;
    HOST_DLLERA_ENTRIES(HOST_DLLERA_FIELD)
#undef HOST_DLLERA_FIELD
};

// Fills g_host from the table and from the game's binkw32.dll (Bink 1.0f in every supported build).
bool host_bind_dllera(const HostImportTable &table);

#endif
