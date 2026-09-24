#include "host_dllera.h"

#include "../../common/log.h"

namespace {

// The game's DLLs are normally loaded by now; loading one that is not only adds a reference.
HMODULE game_module(const char *name)
{
    HMODULE module = GetModuleHandleA(name);
    if (!module)
        module = LoadLibraryA(name);
    if (!module)
        d2log("host: %s not loaded (error %lu)", name, (unsigned long)GetLastError());
    return module;
}

void *resolve(const char *field, const HostImport &import)
{
    HMODULE module = game_module(import.module);
    if (!module)
        return nullptr;
    void *address = import.ordinal
                        ? reinterpret_cast<void *>(GetProcAddress(module, MAKEINTRESOURCEA(import.ordinal)))
                        : reinterpret_cast<BYTE *>(module) + import.rva;
    if (!address)
        d2log("host: %s (%s #%u) missing", field, import.module, import.ordinal);
    return address;
}

template <typename Fn>
bool bind(Fn &slot, const char *field, const HostImport &import)
{
    void *address = resolve(field, import);
    slot = reinterpret_cast<Fn>(address);
    return address != nullptr;
}

template <typename Fn>
bool bind_export(Fn &slot, HMODULE module, const char *name)
{
    slot = reinterpret_cast<Fn>(reinterpret_cast<void *>(GetProcAddress(module, name)));
    if (!slot)
        d2log("host: %s missing", name);
    return slot != nullptr;
}

} // namespace

bool host_bind_dllera(const HostImportTable &table)
{
    HostApi &h = g_host;
    bool ok = true;
    h.nGfxDataBlockOffset = 0x3C;
#define HOST_DLLERA_BIND(name)                                                                           \
    if (table.name.module)                                                                               \
        ok = bind(h.name, #name, table.name) && ok;
    HOST_DLLERA_ENTRIES(HOST_DLLERA_BIND)
#undef HOST_DLLERA_BIND

    // In OpenGL mode no other renderer DLL has loaded Bink yet.
    HMODULE bink = game_module("binkw32.dll");
    if (!bink)
        return false;
    return ok && bind_export(h.BinkOpen, bink, "_BinkOpen@8") &&
           bind_export(h.BinkClose, bink, "_BinkClose@4") &&
           bind_export(h.BinkSetSoundSystem, bink, "_BinkSetSoundSystem@8") &&
           bind_export(h.BinkCopyToBuffer, bink, "_BinkCopyToBuffer@28") &&
           bind_export(h.BinkDoFrame, bink, "_BinkDoFrame@4") &&
           bind_export(h.BinkNextFrame, bink, "_BinkNextFrame@4") &&
           bind_export(h.BinkWait, bink, "_BinkWait@4") &&
           bind_export(h.BinkOpenDirectSound, bink, "_BinkOpenDirectSound@4");
}
