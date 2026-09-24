/* D2Loader plugin interface: D2Loader loads every DLL in its plugin\ folder and calls its "QueryInterface"
 * export, which must return this structure; a NULL return makes D2Loader unload the DLL.
 *
 * D2Loader loads its plugins before the game DLLs, so the install cannot happen yet. D2Loader's own imports of
 * LoadLibraryA are redirected instead: after each library it loads, the install is tried again, and it
 * succeeds as soon as D2gfx.dll is there, before D2gfx has chosen a renderer. */
#include <windows.h>

#include "present/present.h"

/* Returns TRUE once the renderer is installed. */
BOOL d2opengl_attach(void);

typedef HMODULE(WINAPI *LoadLibraryAFn)(LPCSTR name);
static LoadLibraryAFn g_real_load_library;

static HMODULE WINAPI load_library_hook(LPCSTR name)
{
    HMODULE module = g_real_load_library(name);
    if (d2opengl_attach())
        present_patch_modules();
    return module;
}

/* Points the main program's LoadLibraryA import at load_library_hook. */
static void hook_load_library(void)
{
    BYTE *base = (BYTE *)GetModuleHandleA(NULL);
    const IMAGE_NT_HEADERS32 *nt = (const IMAGE_NT_HEADERS32 *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    const IMAGE_DATA_DIRECTORY *dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dir->VirtualAddress)
        return;
    void *target = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    for (IMAGE_IMPORT_DESCRIPTOR *imp = (IMAGE_IMPORT_DESCRIPTOR *)(base + dir->VirtualAddress); imp->Name; imp++) {
        for (void **slot = (void **)(base + imp->FirstThunk); *slot; slot++) {
            if (*slot != target)
                continue;
            DWORD old;
            if (!VirtualProtect(slot, sizeof *slot, PAGE_READWRITE, &old))
                return;
            g_real_load_library = (LoadLibraryAFn)target;
            *slot = (void *)load_library_hook;
            VirtualProtect(slot, sizeof *slot, old, &old);
        }
    }
}

typedef DWORD(__stdcall *PluginEntryFunc)(DWORD reason, LPVOID data);

typedef struct PluginInterface {
    DWORD magic;
    DWORD version;
    LPCSTR description;
    PluginEntryFunc entry;
} PluginInterface;

#define PLUGIN_MAGICWORD 0x44320000u
#define PLUGIN_VERSION   0x01000911u
#define REASON_INIT      0x01u

static DWORD __stdcall plugin_entry(DWORD reason, LPVOID data)
{
    (void)data;
    if (reason == REASON_INIT && !d2opengl_attach())
        hook_load_library();
    return TRUE;
}

static const PluginInterface g_plugin = {
    PLUGIN_MAGICWORD, PLUGIN_VERSION, "D2OpenGL: OpenGL renderer (-opengl)", plugin_entry,
};

const PluginInterface *__stdcall d2loader_query_interface(void)
{
    return &g_plugin;
}
