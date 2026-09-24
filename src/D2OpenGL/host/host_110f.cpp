// The renderer's game dependencies in 1.10f, by module and ordinal. Three entries differ from later builds:
// Fog's assert takes (expression, file, line) rather than (message, address, line), and D2gfx has no
// standalone OS cursor hide/show, so those are supplied here.

#include "host_dllera.h"


namespace {

typedef void(__cdecl *FogAssertFn)(const char *expression, const char *file, int line);

FogAssertFn g_fog_assert;
volatile LONG *g_cursor_visible; // D2gfx keeps the OS cursor state here (1 = shown)

void __cdecl halt_110f(const char *message, uintptr_t address, int line)
{
    char where[32];
    wsprintfA(where, "D2OpenGL.dll %08lx", (unsigned long)address);
    g_fog_assert(message ? message : "", where, line);
}

// The loops D2gfx uses for its own cursor handling.
void __stdcall hide_os_cursor_110f(void)
{
    if (*g_cursor_visible) {
        while (ShowCursor(FALSE) >= 0) {
        }
        *g_cursor_visible = 0;
    }
}

void __stdcall show_os_cursor_110f(void)
{
    if (!*g_cursor_visible) {
        while (ShowCursor(TRUE) < 0) {
        }
        *g_cursor_visible = 1;
    }
}

} // namespace

bool host_init_110f()
{
    static const HostImportTable table = {
        /* SMemAlloc */ {"Storm.dll", 401, 0},
        /* SMemFree */ {"Storm.dll", 403, 0},
        /* SBmpDecodeImage */ {"Storm.dll", 321, 0},
        /* SFileLoadFileEx */ {"Storm.dll", 281, 0},
        /* SFileOpenFileEx */ {"Storm.dll", 268, 0},
        /* Halt */ {nullptr, 0, 0},
        /* LogWrite */ {"Fog.dll", 10029, 0},
        /* AllocClientMemory */ {"Fog.dll", 10042, 0},
        /* FreeClientMemory */ {"Fog.dll", 10043, 0},
        /* GetUseDirectCommand */ {"Fog.dll", 10117, 0},
        /* BuildGammaRamp */ {"Fog.dll", 10198, 0},
        /* SetFont */ {"D2Win.dll", 10127, 0},
        /* DrawGameText */ {"D2Win.dll", 10117, 0},
        /* Utf8ToWide */ {"D2Lang.dll", 10051, 0},
        /* SetGlobalVolume */ {"D2Sound.dll", 10027, 0},
        /* GetHwnd */ {"D2gfx.dll", 10027, 0},
        /* GetScreenSize */ {"D2gfx.dll", 10041, 0},
        /* HideOSCursor */ {nullptr, 0, 0},
        /* ShowOSCursor */ {nullptr, 0, 0},
        /* DrawCelFrame */ {"D2CMP.dll", 10033, 0},
        /* GetDC6Width */ {"D2CMP.dll", 10037, 0},
        /* GetDC6Height */ {"D2CMP.dll", 10038, 0},
        /* GetDC6OffsetX */ {"D2CMP.dll", 10039, 0},
        /* GetDC6OffsetY */ {"D2CMP.dll", 10040, 0},
        /* SetPixelDataFreeCallback */ {"D2CMP.dll", 10044, 0},
        /* GetDC6BlockPixelData */ {"D2CMP.dll", 10043, 0},
        /* GetOrLoadSprite */ {"D2CMP.dll", 10055, 0},
        /* AllocTileEntry */ {"D2CMP.dll", 10091, 0},
        /* SetTileFreeCallback */ {"D2CMP.dll", 10076, 0},
        // Fog stores MEMORYSTATUS.dwTotalPhys here.
        /* pdwTotalPhysicalMemory */ {"Fog.dll", 0, 0x50118},
    };
    if (!host_bind_dllera(table))
        return false;

    HMODULE fog = GetModuleHandleA("Fog.dll");
    HMODULE d2gfx = GetModuleHandleA("D2gfx.dll");
    if (!fog || !d2gfx)
        return false;
    g_fog_assert = reinterpret_cast<FogAssertFn>(reinterpret_cast<void *>(GetProcAddress(fog, MAKEINTRESOURCEA(10023))));
    if (!g_fog_assert)
        return false;
    g_cursor_visible = reinterpret_cast<volatile LONG *>(reinterpret_cast<BYTE *>(d2gfx) + 0xe5f0);

    HostApi &h = g_host;
    h.Halt = halt_110f;
    h.HideOSCursor = hide_os_cursor_110f;
    h.ShowOSCursor = show_os_cursor_110f;
    h.nGfxDataBlockOffset = 0x00;
    h.pD2gfxWindowed = reinterpret_cast<const int *>(reinterpret_cast<BYTE *>(d2gfx) + 0x1d268);
    return true;
}
