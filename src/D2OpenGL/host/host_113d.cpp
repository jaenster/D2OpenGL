// The renderer's game dependencies in 1.13d, by module and ordinal (or RVA for the two D2gfx cursor
// functions, which 1.13d does not export). Each entry was matched to its 1.14d counterpart by code,
// halt lines and calling convention.

#include "host_dllera.h"

bool host_init_113d()
{
    static const HostImportTable table = {
        /* SMemAlloc */ {"Storm.dll", 401, 0},
        /* SMemFree */ {"Storm.dll", 403, 0},
        /* SBmpDecodeImage */ {"Storm.dll", 321, 0},
        /* SFileLoadFileEx */ {"Storm.dll", 281, 0},
        /* SFileOpenFileEx */ {"Storm.dll", 268, 0},
        /* Halt */ {"Fog.dll", 10024, 0},
        /* LogWrite */ {"Fog.dll", 10029, 0},
        /* AllocClientMemory */ {"Fog.dll", 10042, 0},
        /* FreeClientMemory */ {"Fog.dll", 10043, 0},
        /* GetUseDirectCommand */ {"Fog.dll", 10117, 0},
        /* BuildGammaRamp */ {"Fog.dll", 10198, 0},
        /* SetFont */ {"D2Win.dll", 10047, 0},
        /* DrawGameText */ {"D2Win.dll", 10076, 0},
        /* Utf8ToWide */ {"D2Lang.dll", 10051, 0},
        /* SetGlobalVolume */ {"D2sound.dll", 10056, 0},
        /* GetHwnd */ {"D2gfx.dll", 10007, 0},
        /* GetScreenSize */ {"D2gfx.dll", 10023, 0},
        /* HideOSCursor */ {"D2gfx.dll", 0, 0xb1b0},
        /* ShowOSCursor */ {"D2gfx.dll", 0, 0xb180},
        /* DrawCelFrame */ {"D2CMP.dll", 10001, 0},
        /* GetDC6Width */ {"D2CMP.dll", 10092, 0},
        /* GetDC6Height */ {"D2CMP.dll", 10033, 0},
        /* GetDC6OffsetX */ {"D2CMP.dll", 10030, 0},
        /* GetDC6OffsetY */ {"D2CMP.dll", 10007, 0},
        /* SetPixelDataFreeCallback */ {"D2CMP.dll", 10073, 0},
        /* GetDC6BlockPixelData */ {"D2CMP.dll", 10014, 0},
        /* GetOrLoadSprite */ {"D2CMP.dll", 10025, 0},
        /* AllocTileEntry */ {"D2CMP.dll", 10084, 0},
        /* SetTileFreeCallback */ {"D2CMP.dll", 10103, 0},
        // Fog's system info (Fog#10022 returns it); +0xc is MEMORYSTATUS.dwTotalPhys.
        /* pdwTotalPhysicalMemory */ {"Fog.dll", 0, 0x4af38},
    };
    return host_bind_dllera(table);
}
