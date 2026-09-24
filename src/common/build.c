#include <windows.h>

#include "build.h"

static const struct {
    GameBuildId id;
    const char *name;
    DWORD timestamp;
    DWORD image_size;
} g_builds[] = {
    {BUILD_114D, "1.14d Game.exe", 0x574DDFBCu, 0x005BA000u},
};

GameBuildId build_identify(void)
{
    const BYTE *base = (const BYTE *)GetModuleHandleA(NULL);
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)base;
    const IMAGE_NT_HEADERS32 *nt = (const IMAGE_NT_HEADERS32 *)(base + dos->e_lfanew);
    /* 1.14d has no relocations; it only ever runs at its preferred base. */
    if ((DWORD_PTR)base != 0x00400000)
        return BUILD_UNKNOWN;
    for (size_t i = 0; i < sizeof g_builds / sizeof g_builds[0]; i++)
        if (g_builds[i].timestamp == nt->FileHeader.TimeDateStamp &&
            g_builds[i].image_size == nt->OptionalHeader.SizeOfImage)
            return g_builds[i].id;
    return BUILD_UNKNOWN;
}

const char *build_name(GameBuildId id)
{
    for (size_t i = 0; i < sizeof g_builds / sizeof g_builds[0]; i++)
        if (g_builds[i].id == id)
            return g_builds[i].name;
    return "unknown build";
}
