/* The DLL-era games (1.13d, 1.13c, 1.10f). Everything happens in D2gfx.dll, so it works whichever program
 * started the game: Game.exe, a no-CD Game.exe or D2Loader. D2gfx is loaded but has not chosen a renderer when
 * this DLL loads. With -opengl on the command line:
 *   - D2gfx's renderer DLL name for mode 5 (OpenGL), which is NULL, becomes this DLL's path; D2gfx then loads
 *     it and fetches the table through ordinal 10000;
 *   - the stock 1.13 Game.exe picks the mode from the 3dfx, window and d3d ini bytes only, although it parses
 *     -opengl into +7. Its choice is detoured so that opengl picks mode 5, which the rest of the game then
 *     agrees with (the client draws its tiles for a 3D card only in modes 4 and up);
 *   - where Game.exe cannot be patched (1.10f's is encrypted until it runs; other launchers), D2gfx's
 *     initialise function is made to use mode 5 whatever mode it is asked for instead. The windowed flag is a
 *     separate argument and still applies. */
#include <windows.h>
#include <string.h>

#include "../../common/build.h"
#include "../../common/log.h"
#include "install.h"

/* 1.13c and 1.13d: the initialise function (stdcall hInstance, WndProc, mode, windowed) starts with
 * "sub esp,28h / mov eax,[esp+30h]". Those 7 bytes become a jump to a stub that stores 5 in the mode
 * argument and replays them. */
void *d2gfx_init_resume;

extern const char d2gfx_init_mode5[];
__asm__(".globl _d2gfx_init_mode5\n"
        "_d2gfx_init_mode5:\n"
        "    movl $5, 0xc(%esp)\n"
        "    subl $0x28, %esp\n"
        "    movl 0x30(%esp), %eax\n"
        "    jmp  *_d2gfx_init_resume\n");

/* 1.13c: the stub replays the "push ebx" that follows the patched "mov al,[edi+6] / test al,al". */
void *mode_select_113c_resume_3dfx = (void *)0x0040766C; /* mov ebx,4 */
void *mode_select_113c_resume_next = (void *)0x00407682; /* the window / d3d tests */
void *mode_select_113c_resume_done = (void *)0x0040769D; /* mode chosen, in EBX */

extern const char select_mode_113c[];
__asm__(".globl _select_mode_113c\n"
        "_select_mode_113c:\n"
        "    pushl %ebx\n"
        "    cmpb $0, 0x7(%edi)\n"
        "    jne  1f\n"
        "    cmpb $0, 0x6(%edi)\n"
        "    je   2f\n"
        "    jmp  *_mode_select_113c_resume_3dfx\n"
        "2:  jmp  *_mode_select_113c_resume_next\n"
        "1:  movl $5, %ebx\n"
        "    jmp  *_mode_select_113c_resume_done\n");

/* 1.13d: the patched bytes include the "push ebx", which the stub replays. */
void *mode_select_113d_resume_3dfx = (void *)0x004082EC; /* mov ebx,4 */
void *mode_select_113d_resume_next = (void *)0x00408302; /* the window / d3d tests */
void *mode_select_113d_resume_done = (void *)0x0040831D; /* mode chosen, in EBX */

extern const char select_mode_113d[];
__asm__(".globl _select_mode_113d\n"
        "_select_mode_113d:\n"
        "    pushl %ebx\n"
        "    cmpb $0, 0x7(%edi)\n"
        "    jne  1f\n"
        "    cmpb $0, 0x6(%edi)\n"
        "    je   2f\n"
        "    jmp  *_mode_select_113d_resume_3dfx\n"
        "2:  jmp  *_mode_select_113d_resume_next\n"
        "1:  movl $5, %ebx\n"
        "    jmp  *_mode_select_113d_resume_done\n");

/* A Game.exe mode-selection detour; only applied when the bytes at the site are the expected ones. */
typedef struct GameExeDetour {
    DWORD site;
    BYTE original[6];
    SIZE_T length;
    const char *stub;
} GameExeDetour;

static const GameExeDetour g_detour_113c = {0x00407664u, {0x8A, 0x47, 0x06, 0x84, 0xC0}, 5, select_mode_113c};
static const GameExeDetour g_detour_113d = {0x004082E4u, {0x8A, 0x47, 0x06, 0x84, 0xC0, 0x53}, 6, select_mode_113d};

typedef struct DllEraInstall {
    GameBuildId build;
    DWORD name_slot_rva; /* D2gfx's renderer DLL name for mode 5 */
    DWORD patch_rva;
    BYTE original[7];
    const BYTE *replacement; /* NULL: jump to d2gfx_init_mode5 */
    const GameExeDetour *game_exe;
} DllEraInstall;

/* 1.10f reads the mode into ESI once ("mov esi,[esp+80h]"); it becomes "mov esi,5". */
static const BYTE g_110f_mode5[7] = {0xBE, 0x05, 0x00, 0x00, 0x00, 0x90, 0x90};

static const DllEraInstall g_installs[] = {
    {BUILD_113D, 0x10C30u, 0xABA0u, {0x83, 0xEC, 0x28, 0x8B, 0x44, 0x24, 0x30}, NULL, &g_detour_113d},
    {BUILD_113C, 0x10C2Cu, 0xB580u, {0x83, 0xEC, 0x28, 0x8B, 0x44, 0x24, 0x30}, NULL, &g_detour_113c},
    {BUILD_110F, 0xE1E0u, 0x3756u, {0x8B, 0xB4, 0x24, 0x80, 0x00, 0x00, 0x00}, g_110f_mode5, NULL},
};

static BOOL has_switch(const char *cmdline, const char *name)
{
    size_t n = strlen(name);
    for (const char *p = cmdline; (p = strchr(p, '-')) != NULL; p++)
        if (_strnicmp(p + 1, name, n) == 0 && (p[n + 1] == '\0' || p[n + 1] == ' ' || p[n + 1] == '"'))
            return TRUE;
    return FALSE;
}

BOOL install_dllera(GameBuildId build, const char *self_path)
{
    const DllEraInstall *install = NULL;
    for (size_t i = 0; i < sizeof g_installs / sizeof g_installs[0]; i++)
        if (g_installs[i].build == build)
            install = &g_installs[i];
    if (!install)
        return FALSE;
    if (!has_switch(GetCommandLineA(), "opengl")) {
        d2log("install: no -opengl, left alone");
        return TRUE;
    }

    DWORD base = (DWORD)(DWORD_PTR)GetModuleHandleA("D2gfx.dll");
    static const void *const empty = NULL;
    if (!install_patch(base + install->name_slot_rva, &empty, &self_path, sizeof self_path))
        return FALSE;

    const GameExeDetour *detour = install->game_exe;
    if (detour && (DWORD_PTR)GetModuleHandleA(NULL) == 0x00400000 &&
        memcmp((const void *)(DWORD_PTR)detour->site, detour->original, detour->length) == 0) {
        BYTE jump[6];
        DWORD rel = (DWORD)(DWORD_PTR)detour->stub - (detour->site + 5);
        jump[0] = 0xE9;
        memcpy(jump + 1, &rel, 4);
        jump[5] = 0x90;
        d2log("install: Game.exe picks mode 5");
        return install_patch(detour->site, detour->original, jump, detour->length);
    }

    BYTE patch[7];
    if (install->replacement) {
        memcpy(patch, install->replacement, sizeof patch);
    } else {
        DWORD site = base + install->patch_rva;
        DWORD rel = (DWORD)(DWORD_PTR)d2gfx_init_mode5 - (site + 5);
        d2gfx_init_resume = (void *)(DWORD_PTR)(site + 7);
        patch[0] = 0xE9;
        memcpy(patch + 1, &rel, 4);
        patch[5] = patch[6] = 0x90;
    }
    d2log("install: D2gfx forced to mode 5");
    return install_patch(base + install->patch_rva, install->original, patch, sizeof patch);
}
