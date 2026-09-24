// The game modules' imports of the cursor and window-geometry calls, redirected so the game sees its own
// client size while the real window is larger.
//
// The game gets a consistent picture: its client area is the game size, with its top-left corner where
// the real client's is. Screen coordinates it reads (GetCursorPos) or writes (SetCursorPos) are that
// origin plus game coordinates. ScreenToClient and ClientToScreen are plain offsets by that same
// origin, so they need no redirection; no game module imports ClipCursor. The renderer (this DLL) is
// never redirected.

#include <stdio.h>
#include <string.h>

#include "internal.h"
#include "present.h"

#include "../../common/log.h"

PresentUser32 g_user32;

namespace {

BOOL WINAPI Hook_GetCursorPos(LPPOINT pPt)
{
    BOOL bResult = g_user32.GetCursorPos(pPt);
    if (bResult)
        PresentWindow_ScreenToGame(pPt);
    return bResult;
}

BOOL WINAPI Hook_SetCursorPos(int nX, int nY)
{
    POINT pt = {nX, nY};
    PresentWindow_GameToScreen(&pt);
    return g_user32.SetCursorPos(pt.x, pt.y);
}

BOOL WINAPI Hook_GetClientRect(HWND hWnd, LPRECT pRect)
{
    if (pRect == NULL || !PresentWindow_IsMapped(hWnd))
        return g_user32.GetClientRect(hWnd, pRect);
    pRect->left = 0;
    pRect->top = 0;
    pRect->right = PresentWindow_GameWidth();
    pRect->bottom = PresentWindow_GameHeight();
    return TRUE;
}

// The window rectangle of a window whose client area is the game size: the real one, less the difference
// between the real client and the game's.
BOOL WINAPI Hook_GetWindowRect(HWND hWnd, LPRECT pRect)
{
    BOOL bResult = g_user32.GetWindowRect(hWnd, pRect);
    if (!bResult || !PresentWindow_IsMapped(hWnd))
        return bResult;
    RECT rcClient;
    if (g_user32.GetClientRect(hWnd, &rcClient)) {
        pRect->right -= rcClient.right - PresentWindow_GameWidth();
        pRect->bottom -= rcClient.bottom - PresentWindow_GameHeight();
    }
    return bResult;
}

typedef HMODULE(WINAPI *LoadLibraryAFn)(LPCSTR);
LoadLibraryAFn s_pfnLoadLibraryA;

// A game module that loads another gets that one redirected too.
HMODULE WINAPI Hook_LoadLibraryA(LPCSTR szName)
{
    HMODULE hModule = s_pfnLoadLibraryA(szName);
    present_patch_modules();
    return hModule;
}

struct ImportHook {
    const char *szName;
    void *pReal;
    void *pHook;
};

ImportHook s_aHooks[5];
int s_nHooks;

// The modules whose imports are redirected: the program and the game's own DLLs.
const char *const s_aszModules[] = {
    "D2gfx.dll", "D2Win.dll",  "D2Client.dll", "D2Launch.dll", "D2Multi.dll",
    "Storm.dll", "Fog.dll",    "D2Game.dll",   "binkw32.dll",  "SmackW32.dll",
};
HMODULE s_ahPatched[16];
int s_nPatched;

// Points every import slot of hModule that holds a hooked function at its hook.
void PatchModule(HMODULE hModule)
{
    BYTE *pBase = reinterpret_cast<BYTE *>(hModule);
    const IMAGE_DOS_HEADER *pDos = reinterpret_cast<const IMAGE_DOS_HEADER *>(pBase);
    if (pDos->e_magic != IMAGE_DOS_SIGNATURE)
        return;
    const IMAGE_NT_HEADERS32 *pNt = reinterpret_cast<const IMAGE_NT_HEADERS32 *>(pBase + pDos->e_lfanew);
    const IMAGE_DATA_DIRECTORY &dir = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (dir.VirtualAddress == 0)
        return;
    int nPatched = 0;
    for (const IMAGE_IMPORT_DESCRIPTOR *pImport =
             reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR *>(pBase + dir.VirtualAddress);
         pImport->Name != 0; pImport++) {
        for (void **ppSlot = reinterpret_cast<void **>(pBase + pImport->FirstThunk); *ppSlot != NULL; ppSlot++) {
            for (int i = 0; i < s_nHooks; i++) {
                if (*ppSlot != s_aHooks[i].pReal)
                    continue;
                DWORD dwOld;
                if (VirtualProtect(ppSlot, sizeof(*ppSlot), PAGE_READWRITE, &dwOld)) {
                    *ppSlot = s_aHooks[i].pHook;
                    VirtualProtect(ppSlot, sizeof(*ppSlot), dwOld, &dwOld);
                    nPatched++;
                }
            }
        }
    }
    char szPath[MAX_PATH];
    DWORD n = GetModuleFileNameA(hModule, szPath, MAX_PATH);
    const char *szName = n > 0 && n < MAX_PATH ? strrchr(szPath, '\\') : NULL;
    d2log("present: %s: %d imports redirected", szName ? szName + 1 : "?", nPatched);
}

bool IsPatched(HMODULE hModule)
{
    for (int i = 0; i < s_nPatched; i++)
        if (s_ahPatched[i] == hModule)
            return true;
    return false;
}

void TryPatch(HMODULE hModule)
{
    if (hModule == NULL || IsPatched(hModule) || s_nPatched >= (int)(sizeof(s_ahPatched) / sizeof(s_ahPatched[0])))
        return;
    s_ahPatched[s_nPatched++] = hModule;
    PatchModule(hModule);
}

// -opengl as a separate word on the command line.
bool HasOpenGLSwitch(void)
{
    const char *szCmdLine = GetCommandLineA();
    for (const char *p = szCmdLine; (p = strchr(p, '-')) != NULL; p++)
        if (_strnicmp(p + 1, "opengl", 6) == 0 && (p[7] == '\0' || p[7] == ' ' || p[7] == '"'))
            return true;
    return false;
}

// Per-monitor DPI awareness (V2, else V1), else system awareness: the window then gets real pixels.
void MakeDpiAware(void)
{
    typedef BOOL(WINAPI * SetContextFn)(HANDLE);
    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    SetContextFn pfnSetContext =
        reinterpret_cast<SetContextFn>(reinterpret_cast<void *>(GetProcAddress(hUser32, "SetProcessDpiAwarenessContext")));
    if (pfnSetContext) {
        if (pfnSetContext(reinterpret_cast<HANDLE>(static_cast<intptr_t>(-4))) ||
            pfnSetContext(reinterpret_cast<HANDLE>(static_cast<intptr_t>(-3)))) {
            d2log("present: per-monitor DPI aware");
            return;
        }
    }
    if (SetProcessDPIAware())
        d2log("present: system DPI aware");
    else
        d2log("present: DPI awareness unchanged (%lu)", GetLastError());
}

}  // namespace

void PresentImports_Init(void)
{
    if (g_user32.GetCursorPos)
        return;
    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    g_user32.GetCursorPos = reinterpret_cast<BOOL(WINAPI *)(LPPOINT)>(
        reinterpret_cast<void *>(GetProcAddress(hUser32, "GetCursorPos")));
    g_user32.SetCursorPos = reinterpret_cast<BOOL(WINAPI *)(int, int)>(
        reinterpret_cast<void *>(GetProcAddress(hUser32, "SetCursorPos")));
    g_user32.GetClientRect = reinterpret_cast<BOOL(WINAPI *)(HWND, LPRECT)>(
        reinterpret_cast<void *>(GetProcAddress(hUser32, "GetClientRect")));
    g_user32.GetWindowRect = reinterpret_cast<BOOL(WINAPI *)(HWND, LPRECT)>(
        reinterpret_cast<void *>(GetProcAddress(hUser32, "GetWindowRect")));
    s_pfnLoadLibraryA =
        reinterpret_cast<LoadLibraryAFn>(reinterpret_cast<void *>(GetProcAddress(hKernel32, "LoadLibraryA")));

    const ImportHook aHooks[] = {
        {"GetCursorPos", reinterpret_cast<void *>(g_user32.GetCursorPos), reinterpret_cast<void *>(Hook_GetCursorPos)},
        {"SetCursorPos", reinterpret_cast<void *>(g_user32.SetCursorPos), reinterpret_cast<void *>(Hook_SetCursorPos)},
        {"GetClientRect", reinterpret_cast<void *>(g_user32.GetClientRect), reinterpret_cast<void *>(Hook_GetClientRect)},
        {"GetWindowRect", reinterpret_cast<void *>(g_user32.GetWindowRect), reinterpret_cast<void *>(Hook_GetWindowRect)},
        {"LoadLibraryA", reinterpret_cast<void *>(s_pfnLoadLibraryA), reinterpret_cast<void *>(Hook_LoadLibraryA)},
    };
    for (const ImportHook &hook : aHooks)
        if (hook.pReal)
            s_aHooks[s_nHooks++] = hook;
}

extern "C" void present_patch_modules(void)
{
    if (g_presentConfig.eScaling == SCALING_OFF || s_nHooks == 0)
        return;
    TryPatch(GetModuleHandleA(NULL));
    for (const char *szModule : s_aszModules)
        TryPatch(GetModuleHandleA(szModule));
}

extern "C" void present_install(HINSTANCE hSelf)
{
    static bool bInstalled;
    if (bInstalled)
        return;
    bInstalled = true;
    Present_LoadConfig(hSelf);
    const PresentConfig &cfg = g_presentConfig;
    static const char *const s_aszFilters[] = {"nearest", "sharp", "linear"};
    if (cfg.eScaling == SCALING_OFF) {
        d2log("present: Scaling=off");
        return;
    }
    if (!HasOpenGLSwitch()) {
        g_presentConfig.eScaling = SCALING_OFF;
        return;
    }
    char szScaling[16] = "auto";
    if (cfg.eScaling == SCALING_FIXED)
        snprintf(szScaling, sizeof(szScaling), "%d", cfg.nScale);
    d2log("present: Scaling=%s Fullscreen=%s Filter=%s Shader=%s (Scanlines %.2f Mask %.2f Glow %.2f)", szScaling,
          cfg.bBorderless ? "borderless" : "exclusive", s_aszFilters[cfg.eFilter],
          cfg.eShader == SHADER_CRT ? "crt" : "none", (double)cfg.fScanlines, (double)cfg.fMask,
          (double)cfg.fGlow);
    MakeDpiAware();
    PresentImports_Init();
    present_patch_modules();
}
