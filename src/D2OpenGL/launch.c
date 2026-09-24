/* Starts Game.exe from D2OpenGL.dll's folder with this DLL loaded before the game's own code runs:
 *
 *   rundll32.exe D2OpenGL.dll,Play -w
 *
 * (on 64-bit Windows the 32-bit rundll32 in SysWOW64). The game is created suspended; its first thread
 * starts in RtlUserThreadStart with the entry point in EAX. EAX is pointed at a small stub instead,
 * which loads this DLL and then jumps to the real entry point, so the loader has finished and nothing
 * in Game.exe is changed. -opengl is added to the command line. */
#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../common/build.h"
#include "../common/log.h"

typedef struct LaunchStub {
    BYTE code[32];
    char dll_path[MAX_PATH];
} LaunchStub;

static void build_stub(LaunchStub *stub, DWORD remote, DWORD load_library, DWORD entry)
{
    BYTE *p = stub->code;
    DWORD path = remote + offsetof(LaunchStub, dll_path);
    *p++ = 0x60;                                    /* pushad             */
    *p++ = 0x68; memcpy(p, &path, 4); p += 4;       /* push dll_path      */
    *p++ = 0xB8; memcpy(p, &load_library, 4); p += 4; /* mov eax, LoadLibraryA */
    *p++ = 0xFF; *p++ = 0xD0;                       /* call eax           */
    *p++ = 0x61;                                    /* popad              */
    *p++ = 0x68; memcpy(p, &entry, 4); p += 4;      /* push entry         */
    *p++ = 0xC3;                                    /* ret                */
}

static BOOL own_folder(HINSTANCE self, char *dll_path, char *folder)
{
    DWORD n = GetModuleFileNameA(self, dll_path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        return FALSE;
    strcpy(folder, dll_path);
    char *slash = strrchr(folder, '\\');
    if (!slash)
        return FALSE;
    *slash = '\0';
    return TRUE;
}

static void fail(const char *what)
{
    char text[256];
    snprintf(text, sizeof text, "D2OpenGL could not start Diablo II: %s (error %lu).", what,
             (unsigned long)GetLastError());
    MessageBoxA(NULL, text, "D2OpenGL", MB_OK | MB_ICONERROR);
}

extern HINSTANCE g_self;

void CALLBACK Play(HWND hwnd, HINSTANCE inst, LPSTR args, int show)
{
    (void)hwnd;
    (void)inst;
    (void)show;
    char dll_path[MAX_PATH], folder[MAX_PATH], exe[MAX_PATH + 16], cmdline[2048];
    if (!own_folder(g_self, dll_path, folder))
        return fail("cannot find its own folder");
    snprintf(exe, sizeof exe, "%s\\Game.exe", folder);
    /* 1.10f's Game.exe cannot be patched to pick OpenGL; with -3dfx its client draws with the 3D-card
     * paths, and the DLL switches D2gfx itself to OpenGL. */
    const char *extra = build_identify_folder(folder) == BUILD_110F ? " -3dfx" : "";
    snprintf(cmdline, sizeof cmdline, "\"%s\" -opengl%s %s", exe, extra, args ? args : "");

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    PROCESS_INFORMATION pi;
    if (!CreateProcessA(exe, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, folder, &si, &pi))
        return fail("Game.exe did not start");

    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_INTEGER;
    LaunchStub stub = {0};
    void *remote = VirtualAllocEx(pi.hProcess, NULL, sizeof stub, MEM_COMMIT | MEM_RESERVE,
                                  PAGE_EXECUTE_READWRITE);
    DWORD load_library = (DWORD)(DWORD_PTR)(void *)GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                                                  "LoadLibraryA");
    BOOL ok = remote && load_library && GetThreadContext(pi.hThread, &ctx);
    if (ok) {
        /* kernel32 sits at the same address in every process of this boot session. */
        build_stub(&stub, (DWORD)(DWORD_PTR)remote, load_library, ctx.Eax);
        strcpy(stub.dll_path, dll_path);
        ok = WriteProcessMemory(pi.hProcess, remote, &stub, sizeof stub, NULL);
        ctx.Eax = (DWORD)(DWORD_PTR)remote;
        ok = ok && FlushInstructionCache(pi.hProcess, remote, sizeof stub) &&
             SetThreadContext(pi.hThread, &ctx);
    }
    if (!ok) {
        fail("could not load D2OpenGL.dll into the game");
        TerminateProcess(pi.hProcess, 1);
    } else {
        ResumeThread(pi.hThread);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}
