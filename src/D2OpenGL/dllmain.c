/* D2OpenGL.dll: the renderer. In 1.14d Game.exe it installs itself when loaded (see launch.c for how it
 * gets loaded). It also exports renderer_query as ordinal 10000, the entry through which DLL-era D2gfx
 * asks a renderer DLL for its 54-slot table; the table is only handed out for a build with host bindings. */
#include <windows.h>

#include "../common/build.h"
#include "../common/log.h"
#include "install/install.h"
#include "renderer.h"
#include "present/present.h"

HINSTANCE g_self;

/* platform/platform.cpp: runs the functions the renderer registered with platform_atexit. */
void platform_run_atexit(void);

void *const *renderer_query(void)
{
    if (!renderer_host_init()) {
        d2log("renderer: no game bindings, table withheld");
        return NULL;
    }
    return renderer_table();
}

/* Installs the renderer into the running game, once; TRUE when that has happened. Called when this DLL loads
 * and again from the D2Loader plugin path, in case the game DLLs were not loaded yet. */
BOOL d2opengl_attach(void)
{
    static BOOL attached;
    if (attached)
        return TRUE;
    GameBuildId build = build_identify();
    if (build == BUILD_UNKNOWN)
        return FALSE;
    attached = TRUE;
    d2log_reset();
    d2log("D2OpenGL: host is %s", build_name(build));
    present_install(g_self);
    switch (build) {
    case BUILD_114D: {
        void *const *table = renderer_query();
        if (table)
            d2log("D2OpenGL: install %s", install_114d(table) ? "ok" : "FAILED");
        break;
    }
    case BUILD_110F:
    case BUILD_113C:
    case BUILD_113D: {
        /* D2gfx loads this DLL by name later and fetches the table through ordinal 10000. */
        static char self_path[MAX_PATH];
        DWORD n = GetModuleFileNameA(g_self, self_path, MAX_PATH);
        BOOL ok = n > 0 && n < MAX_PATH && install_dllera(build, self_path);
        d2log("D2OpenGL: install %s", ok ? "ok" : "FAILED");
        break;
    }
    default:
        break;
    }
    return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = inst;
        DisableThreadLibraryCalls(inst);
        d2opengl_attach();
    } else if (reason == DLL_PROCESS_DETACH && reserved == NULL) {
        /* FreeLibrary only: at process exit Storm's heap may already be gone. */
        platform_run_atexit();
    }
    return TRUE;
}
