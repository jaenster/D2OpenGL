// The renderer's game dependencies in 1.13c, by module and ordinal (or RVA for what 1.13c does not export).
// Each entry was matched to its 1.14d counterpart by code, halt lines and calling convention.

#include "host_dllera.h"

bool host_init_113c()
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
        /* SetFont */ {"D2Win.dll", 10184, 0},
        /* DrawGameText */ {"D2Win.dll", 10150, 0},
        /* Utf8ToWide */ {"D2Lang.dll", 10051, 0},
        /* SetGlobalVolume */ {"D2sound.dll", 10061, 0},
        /* GetHwnd */ {"D2gfx.dll", 10048, 0},
        /* GetScreenSize */ {"D2gfx.dll", 10080, 0},
        /* HideOSCursor */ {"D2gfx.dll", 0, 0x80a0},
        /* ShowOSCursor */ {"D2gfx.dll", 0, 0x8070},
        /* DrawCelFrame */ {"D2CMP.dll", 10015, 0},
        /* GetDC6Width */ {"D2CMP.dll", 10060, 0},
        /* GetDC6Height */ {"D2CMP.dll", 10000, 0},
        /* GetDC6OffsetX */ {"D2CMP.dll", 10002, 0},
        /* GetDC6OffsetY */ {"D2CMP.dll", 10069, 0},
        /* SetPixelDataFreeCallback */ {"D2CMP.dll", 10067, 0},
        /* GetDC6BlockPixelData */ {"D2CMP.dll", 10098, 0},
        /* GetOrLoadSprite */ {"D2CMP.dll", 10005, 0},
        /* AllocTileEntry */ {"D2CMP.dll", 10106, 0},
        /* SetTileFreeCallback */ {"D2CMP.dll", 10087, 0},
        // Fog's system info; +0xc is MEMORYSTATUS.dwTotalPhys.
        /* pdwTotalPhysicalMemory */ {"Fog.dll", 0, 0x32f08},
    };
    return host_bind_dllera(table);
}
