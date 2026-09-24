#include <windows.h>
#include <stdio.h>

#include "build.h"

/* 1.14d is one Game.exe. The DLL-era builds are recognised by D2gfx.dll instead, because the program that
 * starts them varies (Game.exe, no-CD versions of it, D2Loader). */
static const struct {
    GameBuildId id;
    const char *name;
    const char *module;
    DWORD timestamp;
    DWORD image_size;
} g_builds[] = {
    {BUILD_114D, "1.14d", "Game.exe", 0x574DDFBCu, 0x005BA000u},
    {BUILD_113D, "1.13d", "D2gfx.dll", 0x4E9DE403u, 0x00021000u},
    {BUILD_113C, "1.13c", "D2gfx.dll", 0x4B95C154u, 0x00021000u},
    {BUILD_110F, "1.10f", "D2gfx.dll", 0x3F7CB7A2u, 0x00021000u},
};

static BOOL headers_match(const IMAGE_NT_HEADERS32 *nt, size_t i)
{
    return nt->FileHeader.TimeDateStamp == g_builds[i].timestamp &&
           nt->OptionalHeader.SizeOfImage == g_builds[i].image_size;
}

static const IMAGE_NT_HEADERS32 *loaded_headers(HMODULE module)
{
    const BYTE *base = (const BYTE *)module;
    return (const IMAGE_NT_HEADERS32 *)(base + ((const IMAGE_DOS_HEADER *)base)->e_lfanew);
}

static BOOL file_headers(const char *path, IMAGE_NT_HEADERS32 *nt)
{
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;
    IMAGE_DOS_HEADER dos;
    DWORD got;
    BOOL ok = ReadFile(file, &dos, sizeof dos, &got, NULL) && got == sizeof dos &&
              SetFilePointer(file, dos.e_lfanew, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER &&
              ReadFile(file, nt, sizeof *nt, &got, NULL) && got == sizeof *nt;
    CloseHandle(file);
    return ok;
}

GameBuildId build_identify(void)
{
    HMODULE exe = GetModuleHandleA(NULL);
    for (size_t i = 0; i < sizeof g_builds / sizeof g_builds[0]; i++) {
        /* 1.14d's Game.exe has no relocations and runs at its preferred base. */
        if (g_builds[i].id == BUILD_114D) {
            if ((DWORD_PTR)exe == 0x00400000 && headers_match(loaded_headers(exe), i))
                return BUILD_114D;
            continue;
        }
        HMODULE module = GetModuleHandleA(g_builds[i].module);
        if (module && headers_match(loaded_headers(module), i))
            return g_builds[i].id;
    }
    return BUILD_UNKNOWN;
}

GameBuildId build_identify_folder(const char *folder)
{
    for (size_t i = 0; i < sizeof g_builds / sizeof g_builds[0]; i++) {
        char path[MAX_PATH];
        IMAGE_NT_HEADERS32 nt;
        snprintf(path, sizeof path, "%s\\%s", folder, g_builds[i].module);
        if (file_headers(path, &nt) && headers_match(&nt, i))
            return g_builds[i].id;
    }
    return BUILD_UNKNOWN;
}

const char *build_name(GameBuildId id)
{
    for (size_t i = 0; i < sizeof g_builds / sizeof g_builds[0]; i++)
        if (g_builds[i].id == id)
            return g_builds[i].name;
    return "unknown build";
}
