/* D2OpenGL.dll: the renderer. In 1.14d Game.exe it installs itself when loaded (see launch.c for how it
 * gets loaded). It also exports renderer_query as ordinal 10000, the entry through which DLL-era D2gfx
 * asks a renderer DLL for its 54-slot table; the table is only handed out for a build with host bindings. */
#include <windows.h>

#include "../common/build.h"
#include "../common/log.h"
#include "install/install.h"
#include "renderer.h"

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

static void attach_to_game(void)
{
    GameBuildId build = build_identify();
    if (build == BUILD_UNKNOWN)
        return;
    d2log_reset();
    d2log("D2OpenGL: host is %s", build_name(build));
    if (build != BUILD_114D)
        return;
    void *const *table = renderer_query();
    if (table)
        d2log("D2OpenGL: install %s", install_114d(table) ? "ok" : "FAILED");
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = inst;
        DisableThreadLibraryCalls(inst);
        attach_to_game();
    } else if (reason == DLL_PROCESS_DETACH && reserved == NULL) {
        /* FreeLibrary only: at process exit Storm's heap may already be gone. */
        platform_run_atexit();
    }
    return TRUE;
}
