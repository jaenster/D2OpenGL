#ifndef D2OPENGL_HOST_HOST_H
#define D2OPENGL_HOST_HOST_H

// Everything the renderer needs from the game that is not OpenGL or Win32. The renderer only
// calls through g_host; each supported build fills it in (host_114d.cpp), so the renderer
// itself contains no game address.

#include <windows.h>
#include <stdint.h>

#include "../../common/build.h"
#include "../renderer/types.h"

struct HostApi {
    // Storm
    void *(__stdcall *SMemAlloc)(size_t size, const char *file, int line, int flags);
    BOOL(__stdcall *SMemFree)(void *ptr, const char *file, int line, int flags);
    uint32_t(__stdcall *SBmpDecodeImage)(int format, void *data, unsigned data_size, void *palette,
                                         uint8_t *pixels, int pixel_buf_size, unsigned *width,
                                         unsigned *height, unsigned *bpp);
    BOOL(__stdcall *SFileLoadFileEx)(void *archive, const char *name, void **buffer, DWORD *size,
                                     DWORD extra, DWORD flags, void *overlapped);
    BOOL(__stdcall *SFileOpenFileEx)(void *archive, const char *name, DWORD scope, HANDLE *file);

    // Fog
    void(__cdecl *Halt)(const char *message, uintptr_t address, int line);
    void(__cdecl *LogWrite)(const char *format, ...);
    void *(__fastcall *AllocClientMemory)(size_t size, const char *file, int line, int zero);
    void(__fastcall *FreeClientMemory)(void *ptr, const char *file, int line, int zero);
    BOOL(__stdcall *GetUseDirectCommand)(void);
    void(__fastcall *BuildGammaRamp)(int gamma, uint32_t *out, double contrast, uint32_t *element_size,
                                     int count, int max_value);

    // D2Win / D2Lang / D2Sound
    int(__fastcall *SetFont)(int font);
    void(__fastcall *DrawGameText)(const wchar_t *text, int x, int y, int color, BOOL center);
    wchar_t *(__fastcall *Utf8ToWide)(wchar_t *dst, const char *src, int max_chars);
    void(__fastcall *SetGlobalVolume)(unsigned volume);

    // D2gfx
    HWND(__stdcall *GetHwnd)(void);
    BOOL(__stdcall *GetScreenSize)(int *width, int *height);
    void(__stdcall *HideOSCursor)(void);
    void(__stdcall *ShowOSCursor)(void);

    // D2CMP
    // Args 6-8 are the destination buffer, its height and its pitch (CELCMP_DrawCelFrame 006014c0
    // writes at dest + y * pitch + x).
    void(__stdcall *DrawCelFrame)(D2GfxDataStrc *gfx, int x, int y, int clip_left, int clip_right,
                                  BYTE *dest, int dest_height, int dest_pitch, int arg1, int arg2,
                                  BYTE *palette);
    int(__stdcall *GetDC6Width)(DC6Block *block);
    int(__stdcall *GetDC6Height)(DC6Block *block);
    int(__stdcall *GetDC6OffsetX)(DC6Block *block);
    int(__stdcall *GetDC6OffsetY)(DC6Block *block);
    void(__stdcall *SetPixelDataFreeCallback)(OGLPixelDataFreeCallback callback);
    void *(__stdcall *GetDC6BlockPixelData)(DC6Block *block);
    int(__stdcall *GetOrLoadSprite)(D2GfxDataStrc *gfx, int check_only, int param2);
    uint32_t(__stdcall *AllocTileEntry)(D2TileLibraryEntryStrc *tile, uint32_t flags, uint32_t reserved);
    // Stores the tile free callback that SPRITECACHE_LRU_FreeEntryCallback 005fde50 calls with the
    // tile in ECX.
    void(__stdcall *SetTileFreeCallback)(OGLTileFreeCallback callback);

    // RAD Bink, from the game's own binkw32.dll
    HBINK(__stdcall *BinkOpen)(HANDLE file, uint32_t flags);
    void(__stdcall *BinkClose)(HBINK bink);
    int(__stdcall *BinkSetSoundSystem)(void *open_fn, uintptr_t param);
    int(__stdcall *BinkCopyToBuffer)(HBINK bink, void *dest, int pitch, unsigned height, unsigned x,
                                     unsigned y, unsigned flags);
    int(__stdcall *BinkDoFrame)(HBINK bink);
    void(__stdcall *BinkNextFrame)(HBINK bink);
    int(__stdcall *BinkWait)(HBINK bink);
    void *BinkOpenDirectSound;

    // Game data the renderer reads.
    // Fog system info: total physical RAM (Mac 005ebc38+0xc, read by OGL_CanUseAGPTextures 002e5f00).
    uint32_t *pdwTotalPhysicalMemory;
};

extern HostApi g_host;

// Fills g_host for the running build. False if the build is unknown or a symbol is missing.
bool host_init(GameBuildId build);

#endif
