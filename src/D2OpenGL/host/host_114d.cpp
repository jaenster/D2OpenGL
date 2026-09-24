// The renderer's game dependencies in 1.14d Game.exe, which has every former DLL linked in and no
// relocations. Each address is matched to its Mac counterpart; each calling convention is taken from
// the Windows disassembly.

#include "host.h"

#include "../../common/log.h"

namespace {

template <typename Fn>
void bind(Fn &slot, uintptr_t address)
{
    slot = reinterpret_cast<Fn>(address);
}

template <typename Fn>
bool bind_import(Fn &slot, HMODULE module, const char *name)
{
    slot = reinterpret_cast<Fn>(reinterpret_cast<void *>(GetProcAddress(module, name)));
    if (!slot)
        d2log("host: %s missing", name);
    return slot != nullptr;
}

} // namespace

bool host_init_114d()
{
    HostApi &h = g_host;
    bind(h.SMemAlloc, 0x00413020);
    bind(h.SMemFree, 0x00412650);
    bind(h.SBmpDecodeImage, 0x006C6A80);
    bind(h.SFileLoadFileEx, 0x0041B680);
    bind(h.SFileOpenFileEx, 0x004192F0);

    bind(h.Halt, 0x00408A60);
    bind(h.LogWrite, 0x00410610);
    bind(h.AllocClientMemory, 0x0040B380);
    bind(h.FreeClientMemory, 0x0040B3C0);
    bind(h.GetUseDirectCommand, 0x00406D20);
    bind(h.BuildGammaRamp, 0x006BE190);

    bind(h.SetFont, 0x00502EF0);
    bind(h.DrawGameText, 0x00502320);
    bind(h.Utf8ToWide, 0x00526320);
    bind(h.SetGlobalVolume, 0x00515CE0);

    bind(h.GetHwnd, 0x004F59A0);
    bind(h.GetScreenSize, 0x004F59B0);
    bind(h.HideOSCursor, 0x004F5510);
    bind(h.ShowOSCursor, 0x004F5540);

    bind(h.DrawCelFrame, 0x006014C0);
    bind(h.GetDC6Width, 0x006018C0);
    bind(h.GetDC6Height, 0x006018F0);
    bind(h.GetDC6OffsetX, 0x00601920);
    bind(h.GetDC6OffsetY, 0x00601950);
    bind(h.SetPixelDataFreeCallback, 0x00601A30);
    bind(h.GetDC6BlockPixelData, 0x00601B10);
    bind(h.GetOrLoadSprite, 0x006001F0);
    bind(h.AllocTileEntry, 0x005FDEA0);
    bind(h.SetTileFreeCallback, 0x00604A30);

    // Fog InitializeSystemInfo 00407efb stores MEMORYSTATUS.dwTotalPhys here.
    bind(h.pdwTotalPhysicalMemory, 0x0074D898);

    HMODULE bink = GetModuleHandleA("binkw32.dll");
    if (!bink) {
        d2log("host: binkw32.dll not loaded");
        return false;
    }
    return bind_import(h.BinkOpen, bink, "_BinkOpen@8") &&
           bind_import(h.BinkClose, bink, "_BinkClose@4") &&
           bind_import(h.BinkSetSoundSystem, bink, "_BinkSetSoundSystem@8") &&
           bind_import(h.BinkCopyToBuffer, bink, "_BinkCopyToBuffer@28") &&
           bind_import(h.BinkDoFrame, bink, "_BinkDoFrame@4") &&
           bind_import(h.BinkNextFrame, bink, "_BinkNextFrame@4") &&
           bind_import(h.BinkWait, bink, "_BinkWait@4") &&
           bind_import(h.BinkOpenDirectSound, bink, "_BinkOpenDirectSound@4");
}
